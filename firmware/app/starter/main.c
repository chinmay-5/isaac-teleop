#include <stdio.h>

#include "tx_api.h"

#include "board_init.h"
#include "cmsis_utils.h"
#include "screen.h"
#include "sntp_client.h"
#include "wwd_networking.h"

#include "cloud_config.h"

#include "sensor.h"
#include "ssd1306.h"

#define ECLIPSETX_THREAD_STACK_SIZE 4096
#define ECLIPSETX_THREAD_PRIORITY   4

TX_THREAD eclipsetx_thread;
TX_THREAD eclipsetx_thread2;
ULONG eclipsetx_thread_stack[ECLIPSETX_THREAD_STACK_SIZE / sizeof(ULONG)];
ULONG eclipsetx_thread_stack2[ECLIPSETX_THREAD_STACK_SIZE / sizeof(ULONG)];

#define IMU_THREAD_STACK_SIZE 2048
#define IMU_THREAD_PRIORITY   5

TX_THREAD imu_thread;
ULONG imu_thread_stack[IMU_THREAD_STACK_SIZE / sizeof(ULONG)];

static void eclipsetx_thread_entry(ULONG parameter)
{
    UINT status;

    printf("Starting Eclipse ThreadX thread\r\n\r\n");

    // Initialize the network
    if ((status = wwd_network_init(WIFI_SSID, WIFI_PASSWORD, WIFI_MODE)))
    {
        printf("ERROR: Failed to initialize the network (0x%08x)\r\n", status);
    }

     wwd_network_connect();
}

static void imu_thread_entry(ULONG parameter)
{
    printf("Starting IMU OLED thread\r\n");
    while (1)
    {
        lsm6dsl_data_t imu = lsm6dsl_data_read();

        printf("ACC mg: X=%d Y=%d Z=%d\r\n",
            (int)imu.acceleration_mg[0],
            (int)imu.acceleration_mg[1],
            (int)imu.acceleration_mg[2]);
        
        printf("GYR mdps: X=%d Y=%d Z=%d\r\n",
            (int)imu.angular_rate_mdps[0],
            (int)imu.angular_rate_mdps[1],
            (int)imu.angular_rate_mdps[2]);

        char left[16];
        char right[16];

        ssd1306_Fill(Black);

        /* Column headings */
        ssd1306_SetCursor(0, 0);
        ssd1306_WriteString("ACC mg", Font_7x10, White);

        ssd1306_SetCursor(66, 0);
        ssd1306_WriteString("GYR mdps", Font_7x10, White);

        /* X axis */
        snprintf(left, sizeof(left), "X:%4d",
                (int)imu.acceleration_mg[0]);
        snprintf(right, sizeof(right), "X:%4d",
                (int)imu.angular_rate_mdps[0]);

        ssd1306_SetCursor(0, 16);
        ssd1306_WriteString(left, Font_7x10, White);
        ssd1306_SetCursor(66, 16);
        ssd1306_WriteString(right, Font_7x10, White);

        /* Y axis */
        snprintf(left, sizeof(left), "Y:%4d",
                (int)imu.acceleration_mg[1]);
        snprintf(right, sizeof(right), "Y:%4d",
                (int)imu.angular_rate_mdps[1]);

        ssd1306_SetCursor(0, 30);
        ssd1306_WriteString(left, Font_7x10, White);
        ssd1306_SetCursor(66, 30);
        ssd1306_WriteString(right, Font_7x10, White);

        /* Z axis */
        snprintf(left, sizeof(left), "Z:%4d",
                (int)imu.acceleration_mg[2]);
        snprintf(right, sizeof(right), "Z:%4d",
                (int)imu.angular_rate_mdps[2]);

        ssd1306_SetCursor(0, 44);
        ssd1306_WriteString(left, Font_7x10, White);
        ssd1306_SetCursor(66, 44);
        ssd1306_WriteString(right, Font_7x10, White);

        ssd1306_UpdateScreen();

        tx_thread_sleep(TX_TIMER_TICKS_PER_SECOND / 5);
    }
}


void tx_application_define(void* first_unused_memory)
{
    systick_interval_set(TX_TIMER_TICKS_PER_SECOND);

    // Create ThreadX thread
    UINT status = tx_thread_create(&eclipsetx_thread,
        "Eclipse ThreadX Thread",
        eclipsetx_thread_entry,
        0,
        eclipsetx_thread_stack,
        ECLIPSETX_THREAD_STACK_SIZE,
        ECLIPSETX_THREAD_PRIORITY,
        ECLIPSETX_THREAD_PRIORITY,
        TX_NO_TIME_SLICE,
        TX_AUTO_START);

    if (status != TX_SUCCESS)
    {
        printf("ERROR: Eclipse ThreadX thread creation failed\r\n");
    }

    status = tx_thread_create(&imu_thread,
        "IMU OLED Thread",
        imu_thread_entry,
        0,
        imu_thread_stack,
        IMU_THREAD_STACK_SIZE,
        IMU_THREAD_PRIORITY,
        IMU_THREAD_PRIORITY,
        TX_NO_TIME_SLICE,
        TX_AUTO_START);

    if (status != TX_SUCCESS)
    {
        printf("ERROR: IMU thread creation failed\r\n");
    }
}

int main(void)
{
    // Initialize the board
    board_init();

    // Enter the ThreadX kernel
    tx_kernel_enter();

    return 0;
}
