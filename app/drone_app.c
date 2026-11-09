#include "uart.h"
#include "task_list.h"
#include "drone_config.h"
#include "task_gcs.h"

/* =========================================================
 * App Run
 * ========================================================= */

void drone_app_run(void)
{
    uart_printf("Starting Drone App\r\n");
    task_list_init();
#if DRONE_GCS_ENABLED
    task_gcs_init();
#endif
    while (1)
    {
#if DRONE_GCS_ENABLED
        task_gcs_run();
#endif
        task_list_run();
    }
}
