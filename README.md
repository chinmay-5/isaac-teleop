# Isaac Teleop

Robotics teleoperation project built around the **MXChip AZ3166 IoT DevKit** and **Zephyr RTOS**.

## Current Status

The current firmware milestone is intentionally simple:

- Read accelerometer data from the onboard **LSM6DSL**
- Read gyroscope data from the onboard **LSM6DSL**
- Configure IMU sampling rate at runtime
- Display IMU data on the onboard **128×64 SSD1306 OLED**
- Print IMU data over the serial console

ROS 2 and Isaac Sim integration will be added in later milestones.

## Hardware

### MXChip AZ3166 IoT DevKit

The board provides:

- STM32F412 MCU
- LSM6DSL 6-axis IMU
  - 3-axis accelerometer
  - 3-axis gyroscope
- SSD1306 128×64 OLED
- I²C bus for onboard sensors/display
- USB connection for flashing and serial output

### I²C Devices

The onboard devices used by the current firmware are:

| Device | Address | Function |
|---|---:|---|
| LSM6DSL | `0x6A` | Accelerometer + Gyroscope |
| SSD1306 | `0x3C` | OLED display |

The LSM6DSL is exposed through the Zephyr devicetree as the `accel0` alias.

## Software

- Zephyr RTOS
- Zephyr Sensor API
- Zephyr Character Framebuffer (CFB)
- Zephyr SSD1306 display driver
- Zephyr LSM6DSL sensor driver
- West
- CMake
- Python/UV virtual environment

## Project Structure

```text
isaac-teleop/
├── firmware/
│   ├── app/
│   │   ├── CMakeLists.txt
│   │   ├── prj.conf
│   │   └── src/
│   │       └── main.c
│   │
│   └── zephyr/
│
├── pyproject.toml
└── README.md
```

## Zephyr Configuration

The current application enables:

```text
CONFIG_SERIAL=y
CONFIG_CONSOLE=y
CONFIG_UART_CONSOLE=y
CONFIG_PRINTK=y

CONFIG_I2C=y
CONFIG_SENSOR=y
CONFIG_LSM6DSL=y

CONFIG_DISPLAY=y
CONFIG_CHARACTER_FRAMEBUFFER=y

CONFIG_HEAP_MEM_POOL_SIZE=4096
```

The accelerometer and gyroscope sampling frequency are configured **at runtime** using the Zephyr Sensor API rather than through Kconfig.

Current sampling rate:

```text
104 Hz
```

Runtime configuration uses:

```c
struct sensor_value odr = {
    .val1 = 104,
    .val2 = 0,
};

sensor_attr_set(
    imu,
    SENSOR_CHAN_ACCEL_XYZ,
    SENSOR_ATTR_SAMPLING_FREQUENCY,
    &odr
);

sensor_attr_set(
    imu,
    SENSOR_CHAN_GYRO_XYZ,
    SENSOR_ATTR_SAMPLING_FREQUENCY,
    &odr
);
```

## Reading the IMU

The application uses Zephyr's generic sensor API:

```c
sensor_sample_fetch(imu);
```

followed by:

```c
sensor_channel_get(
    imu,
    SENSOR_CHAN_ACCEL_XYZ,
    accel
);

sensor_channel_get(
    imu,
    SENSOR_CHAN_GYRO_XYZ,
    gyro
);
```

No application-level raw I²C register access is required.

## OLED Display

The onboard SSD1306 is already defined by the AZ3166 board's Zephyr devicetree.

The display is accessed through:

```c
DT_CHOSEN(zephyr_display)
```

and the Zephyr Character Framebuffer API.

The current display layout shows acceleration and gyroscope values in two columns:

```text
ACC                 GYR

X 0.12              X 0.01
Y -0.04             Y -0.00
Z 9.78              Z 0.00
```

Values are displayed with two decimal places.

## Serial Output

The same measurements are printed through the Zephyr console at:

```text
115200 baud
```

Example:

```text
ACC: 0.123000 -0.042000 9.781000 | GYR: 0.001000 -0.003000 0.002000
```

## Building

Activate the UV virtual environment:

```bash
source .venv/bin/activate
```

Build the firmware:

```bash
cd firmware/app
west build
```

For a configuration change, use a pristine build:

```bash
west build -p always
```

## Flashing

Connect the AZ3166 through USB and run:

```bash
west flash
```

## Serial Console

Open the serial console:

```bash
screen /dev/ttyACM0 115200
```

To exit `screen`:

```text
Ctrl-A
K
Y
```

## Current Data Flow

```text
          MXChip AZ3166
                │
        ┌───────┴───────┐
        │               │
     LSM6DSL         SSD1306
        │               ▲
        │               │
        ▼               │
   Zephyr Sensor    Zephyr CFB
       API              │
        │               │
        └───────┬───────┘
                │
             main.c
                │
        ┌───────┴───────┐
        │               │
      USB/Serial       OLED
```