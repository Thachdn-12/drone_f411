#include "bmp280.h"
#include "spi.h"
#include "gpio.h"
#include "systick.h"

#define BMP280_CS_PIN 4U
#define BMP280_REG_ID 0xD0U
#define BMP280_REG_RESET 0xE0U
#define BMP280_REG_STATUS 0xF3U
#define BMP280_REG_CTRL 0xF4U
#define BMP280_REG_CONFIG 0xF5U
#define BMP280_REG_CALIB 0x88U
#define BMP280_REG_DATA 0xF7U

typedef struct
{
    uint16_t t1;
    int16_t t2, t3;
    uint16_t p1;
    int16_t p2, p3, p4, p5, p6, p7, p8, p9;
} bmp280_calibration_t;

static bmp280_calibration_t calibration;
static int ready;

static int read_regs(uint8_t reg, uint8_t *data, uint32_t len)
{
    uint8_t command = reg | 0x80U;
    gpio_write(GPIOA, BMP280_CS_PIN, 0);
    int result = spi1_transfer(&command, 0, 1);
    if (result == 0)
        result = spi1_transfer(0, data, len);
    gpio_write(GPIOA, BMP280_CS_PIN, 1);
    return result;
}

static int write_reg(uint8_t reg, uint8_t value)
{
    uint8_t command[2] = {reg & 0x7FU, value};
    gpio_write(GPIOA, BMP280_CS_PIN, 0);
    int result = spi1_transfer(command, 0, sizeof command);
    gpio_write(GPIOA, BMP280_CS_PIN, 1);
    return result;
}

static uint16_t read_u16(const uint8_t *bytes)
{
    return (uint16_t)((uint16_t)bytes[0] | ((uint16_t)bytes[1] << 8));
}

static int16_t read_s16(const uint8_t *bytes)
{
    uint16_t value = read_u16(bytes);
    return (int16_t)((value & 0x8000U) ? (int32_t)value - 65536 : value);
}

bmp280_status_t bmp280_init(void)
{
    ready = 0;
    gpio_clock_enable(GPIOA);
    (void)RCC_AHB1ENR;
    /* Preload CS high before enabling the output to avoid a low glitch. */
    gpio_write(GPIOA, BMP280_CS_PIN, 1);
    gpio_output_type(GPIOA, BMP280_CS_PIN, GPIO_PUSH_PULL);
    gpio_pull(GPIOA, BMP280_CS_PIN, GPIO_PULL_UP);
    gpio_speed(GPIOA, BMP280_CS_PIN, GPIO_FAST_SPEED);
    gpio_mode(GPIOA, BMP280_CS_PIN, GPIO_OUTPUT);
    delay_ms(3);

    uint8_t id;
    if (read_regs(BMP280_REG_ID, &id, 1) != 0)
        return BMP280_ERR_IO;
    if (id != 0x58U)
        return BMP280_ERR_ID;
    if (write_reg(BMP280_REG_RESET, 0xB6U) != 0)
        return BMP280_ERR_IO;
    delay_ms(3);

    uint32_t start = systick_get_tick();
    uint8_t status;
    uint32_t attempts = 0;
    do
    {
        if (read_regs(BMP280_REG_STATUS, &status, 1) != 0)
            return BMP280_ERR_IO;
        if (!(status & 1U)) /* NVM calibration copy has finished. */
            break;
        if ((uint32_t)(systick_get_tick() - start) >= 20U || ++attempts >= 1000U)
            return BMP280_ERR_TIMEOUT;
    } while (1);

    uint8_t bytes[24];
    if (read_regs(BMP280_REG_CALIB, bytes, sizeof bytes) != 0)
        return BMP280_ERR_IO;
    unsigned all_ff = 1;
    for (unsigned i = 0; i < sizeof bytes; ++i)
        all_ff &= bytes[i] == 0xFFU;
    calibration.t1 = read_u16(bytes);
    calibration.t2 = read_s16(bytes + 2);
    calibration.t3 = read_s16(bytes + 4);
    calibration.p1 = read_u16(bytes + 6);
    calibration.p2 = read_s16(bytes + 8);
    calibration.p3 = read_s16(bytes + 10);
    calibration.p4 = read_s16(bytes + 12);
    calibration.p5 = read_s16(bytes + 14);
    calibration.p6 = read_s16(bytes + 16);
    calibration.p7 = read_s16(bytes + 18);
    calibration.p8 = read_s16(bytes + 20);
    calibration.p9 = read_s16(bytes + 22);
    if (all_ff || calibration.p1 == 0U)
        return BMP280_ERR_CALIBRATION;

    /* Reset leaves the sensor asleep; config must be written before normal
     * mode. spi3w_en remains 0 for separate MOSI/MISO wires. */
    if (write_reg(BMP280_REG_CONFIG, 0x08U) != 0 ||
        write_reg(BMP280_REG_CTRL, 0x57U) != 0)
        return BMP280_ERR_IO;
    delay_ms(50); /* First x2/x16 conversion: maximum ~43.2 ms. */
    ready = 1;
    return BMP280_OK;
}

bmp280_status_t bmp280_read(bmp280_sample_t *sample)
{
    if (sample == 0)
        return BMP280_ERR_ARGUMENT;
    if (!ready)
        return BMP280_ERR_NOT_READY;
    uint8_t bytes[6];
    if (read_regs(BMP280_REG_DATA, bytes, sizeof bytes) != 0)
        return BMP280_ERR_IO;
    uint32_t adc_p = ((uint32_t)bytes[0] << 12) | ((uint32_t)bytes[1] << 4) | (bytes[2] >> 4);
    uint32_t adc_t = ((uint32_t)bytes[3] << 12) | ((uint32_t)bytes[4] << 4) | (bytes[5] >> 4);
    if (adc_p == 0x80000U || adc_t == 0x80000U)
        return BMP280_ERR_DATA; /* Skipped or not yet available. */

    /* Bosch BST-BMP280-DS001, section 8.1, evaluated with the M4F's
     * single-precision FPU. Keep t_fine local to this coherent burst. */
    float v1 = ((float)adc_t / 16384.0f - (float)calibration.t1 / 1024.0f) * calibration.t2;
    float delta = (float)adc_t / 131072.0f - (float)calibration.t1 / 8192.0f;
    float v2 = delta * delta * calibration.t3;
    float fine = v1 + v2;
    float centi_c = fine / 51.2f;

    v1 = fine / 2.0f - 64000.0f;
    v2 = v1 * v1 * calibration.p6 / 32768.0f;
    v2 += v1 * calibration.p5 * 2.0f;
    v2 = v2 / 4.0f + (float)calibration.p4 * 65536.0f;
    v1 = ((float)calibration.p3 * v1 * v1 / 524288.0f + (float)calibration.p2 * v1) / 524288.0f;
    v1 = (1.0f + v1 / 32768.0f) * calibration.p1;
    if (v1 == 0.0f)
        return BMP280_ERR_DATA;
    float pressure = (1048576.0f - (float)adc_p - v2 / 4096.0f) * 6250.0f / v1;
    v1 = (float)calibration.p9 * pressure * pressure / 2147483648.0f;
    v2 = pressure * calibration.p8 / 32768.0f;
    pressure += (v1 + v2 + (float)calibration.p7) / 16.0f;

    /* Reject invalid compensation (including NaN) before integer conversion.
     * These broad bounds include the sensor's entire specified range. */
    if (!(centi_c >= -5000.0f && centi_c <= 10000.0f &&
          pressure > 0.0f && pressure <= 200000.0f))
        return BMP280_ERR_DATA;
    sample->temperature_centi_c = (int32_t)(centi_c + (centi_c < 0.0f ? -0.5f : 0.5f));
    sample->pressure_pa = (uint32_t)(pressure + 0.5f);
    return BMP280_OK;
}
