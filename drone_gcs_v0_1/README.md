# Drone GCS v0.3.5 — STM32F411

Ubuntu desktop Ground Control Station (PySide6) and portable STM32 firmware protocol module.

## What's new in v0.3.5

Headers, parameter names, values and units are centred in both sensor tables.

## What's new in v0.3.4

Altitude is displayed in **centimetres** on both the dedicated instrument and the barometer table (1 m = 100 cm); the moving scale uses 50 cm per tick. UART, CSV and internal altitude calculations continue to use metres, including playback and pressure-derived height, so existing firmware and recordings retain their original units. Sensor readings now use read-only tables with headers, grid lines and alternating row shading: IMU has Parameter / X / Y / Z / Unit columns; Barometer has Parameter / Value / Unit columns. Missing measurements show `--`.

## What's new in v0.3.3

Sensor readings now occupy two separate panels: **MPU6050 / IMU** groups acceleration and gyro by X/Y/Z; **BMP280 / Barometer** groups pressure, temperature and altitude. Values retain their units and missing measurements display `--`. The altitude row mirrors the dedicated altitude instrument, including the current pressure-derived relative altitude when available. This layout change does not change the telemetry or CSV field list.

## What's new in v0.3.0

A projected 3D quadrotor now sits alongside the attitude plot. It follows the latest received roll, pitch and yaw in UART, simulation and CSV replay; no smoothing or synthetic rotation is added. Cyan arms and a yellow body arrow mark the nose. The panel shows the current connection/ARM/replay state and dims to a neutral preview when telemetry is stale or disconnected. Rotor blades are static and do not represent motor RPM. The rendering uses Qt's painter and requires no OpenGL or new packages.

Angles are degrees, using body X forward, Y right, Z down and world north/east/down: positive roll lowers the right side, positive pitch raises the nose, and positive yaw turns clockwise from N. Rotation is `Rz(yaw) * Ry(pitch) * Rx(roll)`. The fixed N/E grid is the zero-yaw reference, not a GPS position or a verified magnetic heading. Firmware must send angles with this convention for the model to match physical orientation. The separate altitude instrument remains the height display; the 3D model visualizes orientation only.

## What's new in v0.2.4

The circular attitude view keeps its horizon through the centre and rotates only with roll. The internal pitch ladder and vertical motion have been removed. Only roll and pitch angles appear beneath the face; pitch is a numeric reading, and heading remains available in telemetry, the plot and CSV. Altitude continues to use its separate instrument.

## What's new in v0.2.3

Attitude and altitude now have separate instruments. The circular horizon shows roll, pitch and heading; the altitude panel shows a large metre readout and a moving vertical scale (5 m per tick), including negative heights. Both update from UART, simulation or CSV playback and show no data when sensors become stale or the source is disconnected. The altitude value uses the reference provided by firmware; the GUI does not infer ground clearance. Raw IMU and barometer values remain in their own panel, and CSV still includes all 12 telemetry fields.

## What's new in v0.2.2

The artificial horizon now uses a circular face with a bezel, bank-angle marks and a moving roll pointer. Roll, pitch and heading readings remain visible below the instrument.

## What's new in v0.2.1

Each motor now has a synchronized slider and numeric PWM input (1000–2000 µs, numeric step 1 µs), plus **Send PWM** to resend the current value, including the initial 1000 µs. Slider changes send immediately; typing commits on Enter or focus loss. Per-channel labels show the last value sent over UART or a simulation value. These labels do not measure the pin waveform or acknowledge physical output. Controls require the bench checkbox, a live UART link and the requested ARM acknowledgement; simulation supports local testing.

For physical pulse tests, follow [the PWM measurement instructions](docs/INTEGRATION.md#per-motor-pwm-measurement).

## What's new in v0.2

- Artificial horizon with roll, pitch and heading; telemetry values include units.
- UART statistics: telemetry frequency over a rolling five-second window, valid packets, malformed packets, received bytes, packet age and sensor age. These are receive statistics, not a packet-loss estimate.
- CSV playback: open recordings, play/pause, seek by sample and replay the attitude display and plots. Compatible with complete v0.1 CSV recordings.
- Motor sliders stay locked until a requested ARM is acknowledged. Disarm, interlock removal, disconnect and timeout reset the sliders and revoke motor authorization. Late status packets cannot restore that authorization.
- Failed UART writes close the connection without recursive DISARM attempts. Malformed packets cannot keep the link alive; oversized lines are discarded through the next newline.
- Firmware requires a valid heartbeat before ARM, publishes status every 500 ms and reports its E-stop latch. Repeated ARM requests disable existing outputs before rearming.

## Try the new interface

1. Launch the app, then select **Start simulation** to see the horizon and live telemetry.
2. Select **Start CSV logging**, choose a file, then **Stop CSV logging** after collecting samples.
3. Select **Open CSV**. Loading a valid recording disconnects UART, stops simulation and closes logging. Motor controls are disabled during replay.
4. Use **Play/Pause** and the slider to inspect samples. **Close replay** returns to the disconnected state. Connecting UART or starting simulation also exits replay.

CSV files require `timestamp` (Unix seconds) plus all 12 telemetry columns shown below. Values must be finite numbers and timestamps must be nondecreasing. Invalid recordings report an error without interrupting the current data source. New logs use a monotonic clock offset from the initial wall-clock time so clock corrections cannot reverse their timestamps. Switching data sources or disconnecting UART closes the current log.

The UART link times out after one second without a valid status or telemetry packet and requests DISARM. ARM acknowledgements also time out after one second. Stale sensor data clears the attitude and altitude instruments even if status packets continue; raw IMU/barometer readings remain the last received values, with sensor age shown above them. After an E-stop, reset the MCU and reconnect UART; simulation clears its latch when restarted. Always verify the firmware's actual state.

## Quick start (Ubuntu)

```bash
sudo apt update
sudo apt install -y python3-venv python3-pip
cd drone_gcs_v0_1
python3 -m venv .venv
source .venv/bin/activate
pip install -r requirements.txt
python -m gcs.main
```

Use **Start simulation** without hardware. For real UART select `/dev/ttyUSB0` or `/dev/ttyACM0`, baud **115200, 8N1**, then Connect UART. If permission is denied:

```bash
sudo usermod -aG dialout "$USER"
```

Log out and back in. Do **not** use `sudo` to launch the GUI.

## Project structure

```
drone_gcs_v0_1/
├── gcs/
│   ├── __init__.py
│   ├── main.py          # PySide6 GUI, simulator, UART, plots, logging, replay
│   ├── attitude.py      # Artificial horizon widget
│   ├── altitude.py      # Independent altitude readout and moving scale
│   ├── drone_model.py   # Projected 3D quadrotor driven by received Euler angles
│   ├── replay.py        # CSV validation and loading
│   └── protocol.py      # JSON Lines framing and packet validation
├── firmware/
│   ├── gcs_link.h       # MCU API and board hooks
│   └── gcs_link.c       # Parser, heartbeat and safety state
├── docs/
│   └── INTEGRATION.md   # STM32 port instructions and safety tests
├── tests/              # Python GUI/protocol/replay tests and host C harness
├── requirements.txt
└── README.md
```

## UART protocol

One compact JSON object per line (`\n`), 115200 8N1.

PC -> MCU:

```
{"type":"heartbeat","seq":1}
{"type":"arm","bench":true}
{"type":"motor","id":1,"us":1000}
{"type":"disarm"}
{"type":"estop"}
```

MCU -> PC:

```
{"type":"status","armed":false,"estop":false,"reason":"disarmed"}
{"type":"telemetry","roll":1.2,"pitch":-0.5,"yaw":3.0,"ax":0,"ay":0,"az":9.81,"gx":0,"gy":0,"gz":0,"pressure":101325,"temperature":26,"altitude":0}
```

**Firmware parser accepts only the exact compact command layout generated by this GUI.** It is intentionally a narrow parser, not a general-purpose JSON implementation. Do not reformat/reorder fields without updating it.

Firmware also publishes `status` with `reason="periodic"` every 500 ms when `gcs_poll` is scheduled. The new GUI accepts v0.1 status packets without `estop`; the v0.2 firmware's added status field is ignored by the old GUI. Telemetry fields and command layouts are unchanged. Maximum host receive frame: 512 bytes including newline; maximum firmware receive command: 255 bytes excluding newline (254 with CRLF).

## Automated checks

```bash
QT_QPA_PLATFORM=offscreen .venv/bin/python -m unittest discover -s tests -v
```

The C harness requires `gcc` and is skipped if it is unavailable. It verifies framing recovery, startup/heartbeat gating, motor limits, physical interlock, timeout, E-stop and timer wrap using mock board hooks. GUI checks use an offscreen Qt backend and simulated UART failures. These checks do not validate real PWM timing, wiring or ESC stop behavior.

## Scope and limitations

- Simulation is software-only. It is not a physics model and does not validate flight dynamics.
- The circular horizon shows roll, while the 3D quadrotor uses received roll/pitch/yaw. Neither display validates sensor fusion accuracy.
- Firmware `gcs_link.c` is a portable integration module, **not** a standalone flashable STM32 application. Implement board hooks and connect to existing UART/PWM/sensor drivers.
- PWM is a *request*; no motor outputs are enabled in this package. Real outputs remain disabled until board hooks are explicitly implemented.
- Bench-only motor testing: **remove all propellers**; use a physical bench enable jumper/switch.
- No flight-ready motor control, PID, sensor fusion, or wireless failsafe in this release.
- UART status ACK reflects firmware's arm state, but does not certify ESC output behavior.
- E-stop is software, not a substitute for an independent physical power cutoff.
