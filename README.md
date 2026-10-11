# Isaac Teleop

Firmware for the MXChip AZ3166 IoT DevKit, developed as the hardware interface for the Isaac Teleop project.

The firmware uses Eclipse ThreadX and reads accelerometer and gyroscope measurements from the onboard LSM6DSL IMU. The measurements are printed to the console and displayed on the onboard SSD1306 OLED.

## Hardware

- **Development board:** MXChip AZ3166 IoT DevKit
- **Microcontroller:** STM32F412
- **IMU:** ST LSM6DSL accelerometer and gyroscope
- **Display:** SSD1306 OLED, 128 × 64 pixels
- **Connectivity:** Onboard Wi-Fi

## Software Stack

- Eclipse ThreadX
- NetX Duo
- MXChip board support package (BSP)
- STM32CubeF4
- ARM GNU Embedded Toolchain
- CMake
- Ninja
- Ubuntu 22.04 under WSL on Windows 11

## Repository Structure

```text
isaac-teleop/
├── README.md
└── firmware/
    ├── CMakeLists.txt
    ├── app/
    │   ├── CMakeLists.txt
    │   ├── common/
    │   └── starter/
    ├── cmake/
    │   ├── arm-gcc-cortex-m4.cmake
    │   ├── arm-gcc-cortex-toolchain.cmake
    │   ├── FindCMSIS.cmake
    │   ├── FindSTM32HAL.cmake
    │   └── utilities.cmake
    ├── deps/
    │   ├── lib/
    │   └── src/
    ├── lib/
    │   ├── mxchip_bsp/
    │   ├── netxduo/
    │   ├── stm32cubef4/
    │   ├── threadx/
    │   └── wiced_sdk/
    └── scripts/
        ├── build.ps1
        ├── build.sh
        ├── deploy.ps1
        └── deploy.sh
```

### Key Files and Directories

| Path | Purpose |
|---|---|
| `firmware/app/starter/main.c` | Main application and ThreadX thread setup |
| `firmware/app/common/` | Shared application code |
| `firmware/lib/mxchip_bsp/` | Board support package and peripheral drivers |
| `firmware/lib/threadx/` | ThreadX RTOS |
| `firmware/lib/netxduo/` | NetX Duo networking stack |
| `firmware/scripts/build.sh` | Linux/WSL build script |
| `firmware/scripts/deploy.sh` | Firmware deployment script |

## Implemented Functionality

### IMU Reading

The firmware reads acceleration and angular velocity from the onboard LSM6DSL using the MXChip sensor driver.

The sensor API defines the following data structure:

```c
typedef struct {
    float acceleration_mg[3];
    float angular_rate_mdps[3];
    float temperature_degC;
} lsm6dsl_data_t;
```

Sensor readings are retrieved using:

```c
lsm6dsl_data_t imu = lsm6dsl_data_read();
```

The application currently prints the acceleration and gyroscope measurements as integer values to the console.

Example output:

```text
ACC mg: X=318 Y=-173 Z=977
GYR mdps: X=120 Y=-85 Z=30
```

The values shown are illustrative; actual measurements change with the board's orientation and movement.

### Measurement Units

| Measurement | Unit | Description |
|---|---|---|
| Acceleration X, Y, Z | mg | Milligravity |
| Angular velocity X, Y, Z | mdps | Millidegrees per second |
| Sensor temperature | °C | Degrees Celsius, available through the sensor data structure |

### OLED Display

The firmware displays accelerometer and gyroscope measurements on the onboard SSD1306 OLED, which has a resolution of 128 × 64 pixels.

The current layout uses two columns:

```text
ACC mg          GYR mdps
X:  318         X:  120
Y: -173         Y:  -85
Z:  977         Z:   30
```

The display is refreshed periodically by the IMU thread, approximately five times per second.

### ThreadX

The application uses ThreadX threads to execute its tasks. The current application includes an IMU task for reading sensor data, printing measurements, and refreshing the OLED.

### Networking Support

The project includes NetX Duo and the existing MXChip Wi-Fi networking code. The starter application contains the network initialization and connection functions.

## Development Environment

Development is performed in Ubuntu 22.04 under WSL on Windows 11.

Install the required build tools:

```bash
sudo apt update
sudo apt install -y cmake ninja-build gcc-arm-none-eabi
```

Verify the installation:

```bash
arm-none-eabi-gcc --version
cmake --version
ninja --version
```

## Build Instructions

Navigate to the firmware directory:

```bash
cd ~/isaac-teleop/firmware
```

Build the default `starter` configuration:

```bash
./scripts/build.sh
```

The build script configures the project with CMake and Ninja and compiles the firmware. The build directory is `firmware/build/`, and the deployment script expects the generated `.bin` file under `firmware/build/app/`.

To clean the existing build directory and reconfigure the project:

```bash
./scripts/build.sh starter rebuild
```

To clean the build directory before building again:

```bash
./scripts/build.sh starter clean
```

Inspect the generated firmware binaries:

```bash
find build/app -maxdepth 1 -type f -name '*.bin' -print
```

## Deployment

Connect the AZ3166 to the PC over USB and identify the drive letter assigned to its storage volume in Windows.

For example, if the board appears as drive `E:`, mount it in WSL:

```bash
sudo mkdir -p /mnt/e
sudo mount -t drvfs E: /mnt/e
```

Check that the destination is accessible:

```bash
ls -la /mnt/e
```

Deploy the built firmware:

```bash
cd ~/isaac-teleop/firmware
./scripts/deploy.sh /mnt/e
```

The deployment script copies a `.bin` file from `build/app/` to the specified destination.

The script defaults to `/Volumes/AZ3166`, which is a macOS path. Under WSL, pass the actual mount path explicitly.

If multiple `.bin` files exist in the build directory, check which one is intended before deploying, since the current script selects the first matching file.

## Wi-Fi Configuration

The starter application's Wi-Fi configuration is located at:

```text
firmware/app/starter/cloud_config.h
```

Keep Wi-Fi credentials private and do not commit real passwords to the repository.

## Current Status

- [x] ThreadX-based firmware project structure
- [x] LSM6DSL accelerometer readings
- [x] LSM6DSL gyroscope readings
- [x] Console output of sensor measurements
- [x] OLED display of acceleration and angular velocity
- [x] Existing Wi-Fi networking support in the starter application
- [x] Build and deployment scripts for the firmware
