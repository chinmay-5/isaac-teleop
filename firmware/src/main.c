#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/display.h>
#include <zephyr/display/cfb.h>
#include <zephyr/sys/printk.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#define DISPLAY_NODE  DT_CHOSEN(zephyr_display)

static const struct device *imu = DEVICE_DT_GET_ONE(st_lsm6dsl);
static const struct device *display = DEVICE_DT_GET(DISPLAY_NODE);


/*
 * Convert Zephyr sensor_value to a short decimal string.
 */
static void format_sensor_value(char *buf,
                                size_t len,
                                const struct sensor_value *v)
{
    int32_t val1 = v->val1;
    int32_t val2 = v->val2;

    if (val1 < 0) {
        val2 = -val2;
    }

    snprintf(buf,
             len,
             "%ld.%02ld",
             (long)val1,
             (long)(abs(val2) / 10000));
}


int main(void)
{
    struct sensor_value accel[3];
    struct sensor_value gyro[3];

    int ret;

    printk("\n");
    printk("=============================\n");
    printk(" AZ3166 IMU + OLED\n");
    printk("=============================\n");


    /* -------------------------------------------------
     * IMU
     * ------------------------------------------------- */

    if (!device_is_ready(imu)) {
        printk("ERROR: IMU not ready\n");
        return 0;
    }

    printk("IMU ready: %s\n", imu->name);


    /*
     * Configure accelerometer sampling frequency
     * at runtime.
     *
     * 104 Hz.
     */
    struct sensor_value odr = {
        .val1 = 104,
        .val2 = 0,
    };

    ret = sensor_attr_set(
        imu,
        SENSOR_CHAN_ACCEL_XYZ,
        SENSOR_ATTR_SAMPLING_FREQUENCY,
        &odr
    );

    if (ret < 0) {
        printk("ERROR: accel ODR configuration failed: %d\n", ret);
        return 0;
    }

    printk("Accel ODR: 104 Hz\n");


    /*
     * Configure gyroscope sampling frequency
     * at runtime.
     */
    ret = sensor_attr_set(
        imu,
        SENSOR_CHAN_GYRO_XYZ,
        SENSOR_ATTR_SAMPLING_FREQUENCY,
        &odr
    );

    if (ret < 0) {
        printk("ERROR: gyro ODR configuration failed: %d\n", ret);
        return 0;
    }

    printk("Gyro ODR: 104 Hz\n");


    /* -------------------------------------------------
     * OLED
     * ------------------------------------------------- */

    if (!device_is_ready(display)) {
        printk("ERROR: OLED not ready\n");
        return 0;
    }

    printk("OLED ready: %s\n", display->name);


    ret = cfb_framebuffer_init(display);

    if (ret < 0) {
        printk("ERROR: CFB initialization failed: %d\n", ret);
        return 0;
    }

    cfb_framebuffer_set_font(display, 0);

    display_blanking_off(display);

    cfb_framebuffer_clear(display, true);

    cfb_print(display, "AZ3166 IMU", 0, 0);
    cfb_print(display, "Starting...", 0, 16);

    ret = cfb_framebuffer_finalize(display);

    if (ret < 0) {
        printk("ERROR: OLED finalize failed: %d\n", ret);
        return 0;
    }

    k_sleep(K_SECONDS(1));


    /* -------------------------------------------------
     * Main loop
     * ------------------------------------------------- */

    while (1) {

        /* ---------------------------------------------
         * Fetch IMU sample
         * --------------------------------------------- */

        ret = sensor_sample_fetch(imu);

        if (ret < 0) {
            printk("sensor fetch failed: %d\n", ret);
            k_sleep(K_MSEC(100));
            continue;
        }


        /* ---------------------------------------------
         * Read accelerometer
         * --------------------------------------------- */

        ret = sensor_channel_get(
            imu,
            SENSOR_CHAN_ACCEL_XYZ,
            accel
        );

        if (ret < 0) {
            printk("accelerometer read failed: %d\n", ret);
            k_sleep(K_MSEC(100));
            continue;
        }


        /* ---------------------------------------------
         * Read gyroscope
         * --------------------------------------------- */

        ret = sensor_channel_get(
            imu,
            SENSOR_CHAN_GYRO_XYZ,
            gyro
        );

        if (ret < 0) {
            printk("gyroscope read failed: %d\n", ret);
            k_sleep(K_MSEC(100));
            continue;
        }


        /* ---------------------------------------------
         * Serial output
         * --------------------------------------------- */

        printk(
            "ACC: %d.%06d %d.%06d %d.%06d | "
            "GYR: %d.%06d %d.%06d %d.%06d\n",

            accel[0].val1,
            accel[0].val2,

            accel[1].val1,
            accel[1].val2,

            accel[2].val1,
            accel[2].val2,

            gyro[0].val1,
            gyro[0].val2,

            gyro[1].val1,
            gyro[1].val2,

            gyro[2].val1,
            gyro[2].val2
        );


        /* ---------------------------------------------
         * Format OLED values
         * --------------------------------------------- */

        char ax[10];
        char ay[10];
        char az[10];

        char gx[10];
        char gy[10];
        char gz[10];

        format_sensor_value(ax, sizeof(ax), &accel[0]);
        format_sensor_value(ay, sizeof(ay), &accel[1]);
        format_sensor_value(az, sizeof(az), &accel[2]);

        format_sensor_value(gx, sizeof(gx), &gyro[0]);
        format_sensor_value(gy, sizeof(gy), &gyro[1]);
        format_sensor_value(gz, sizeof(gz), &gyro[2]);


        /* ---------------------------------------------
         * OLED
         * --------------------------------------------- */

        cfb_framebuffer_clear(display, false);

        /*
         * Keep the two columns separated.
         *
         * Left:  ACC
         * Right: GYR
         */
        cfb_print(display, "ACC", 0, 0);
        cfb_print(display, "GYR", 70, 0);

        char left[16];
        char right[16];


        /* ACC */

        snprintf(left, sizeof(left), "X%s", ax);
        cfb_print(display, left, 0, 16);

        snprintf(left, sizeof(left), "Y%s", ay);
        cfb_print(display, left, 0, 30);

        snprintf(left, sizeof(left), "Z%s", az);
        cfb_print(display, left, 0, 44);


        /* GYR */

        snprintf(right, sizeof(right), "X%s", gx);
        cfb_print(display, right, 70, 16);

        snprintf(right, sizeof(right), "Y%s", gy);
        cfb_print(display, right, 70, 30);

        snprintf(right, sizeof(right), "Z%s", gz);
        cfb_print(display, right, 70, 44);

        cfb_framebuffer_finalize(display);


        /*
         * Application update rate:
         * 10 Hz
         */
        k_sleep(K_MSEC(100));
    }

    return 0;
}