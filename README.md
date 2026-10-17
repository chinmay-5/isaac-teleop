# Tele-Link

A real-time IMU telemetry system using the MXChip AZ3166 IoT DevKit, Eclipse ThreadX, UDP over Wi-Fi, and Python-based visualization.

The firmware reads accelerometer and gyroscope measurements from the onboard LSM6DSL IMU. Measurements are printed to the console, displayed on the onboard SSD1306 OLED, and transmitted over UDP to a PC for real-time visualization.

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
- Python
- Matplotlib
- Ubuntu 22.04 under WSL on Windows 11

## Repository Structure

```text
tele-link/
├── README.md
├── firmware/
│   ├── CMakeLists.txt
│   ├── app/
│   │   ├── CMakeLists.txt
│   │   ├── common/
│   │   └── starter/
│   ├── cmake/
│   │   ├── arm-gcc-cortex-m4.cmake
│   │   ├── arm-gcc-cortex-toolchain.cmake
│   │   ├── FindCMSIS.cmake
│   │   ├── FindSTM32HAL.cmake
│   │   └── utilities.cmake
│   ├── deps/
│   │   ├── lib/
│   │   └── src/
│   ├── lib/
│   │   ├── mxchip_bsp/
│   │   ├── netxduo/
│   │   ├── stm32cubef4/
│   │   ├── threadx/
│   │   └── wiced_sdk/
│   └── scripts/
│       ├── build.ps1
│       ├── build.sh
│       ├── deploy.ps1
│       └── deploy.sh
├── pyproject.toml
├── imu_receiver.py
└── imu_visualizer.py
```

## Key Files and Directories

| Path | Purpose |
|---|---|
| `firmware/app/starter/main.c` | Main firmware application and ThreadX thread setup |
| `firmware/app/common/` | Shared application and networking code |
| `firmware/lib/mxchip_bsp/` | Board support package and peripheral drivers |
| `firmware/lib/threadx/` | ThreadX RTOS |
| `firmware/lib/netxduo/` | NetX Duo networking stack |
| `firmware/scripts/build.sh` | Linux/WSL firmware build script |
| `firmware/scripts/deploy.sh` | Firmware deployment script |
| `imu_receiver.py` | Minimal UDP receiver for sensor packets |
| `imu_visualizer.py` | Real-time accelerometer and gyroscope plots |

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

The application prints the acceleration and gyroscope measurements as integer values to the console.

Example output:

```text
ACC mg: X=318 Y=-173 Z=977
GYR mdps: X=120 Y=-85 Z=30
```

Values are illustrative; actual measurements depend on the board's orientation and movement.

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

The display refreshes approximately five times per second.

### ThreadX

The application uses ThreadX threads to execute its tasks. The IMU thread reads sensor measurements, prints them to the console, updates the OLED, and sends UDP packets.

### Wi-Fi and UDP Telemetry

The firmware uses the existing MXChip Wi-Fi networking code and NetX Duo to connect to the wireless network and transmit IMU measurements to a PC.

The current UDP configuration is:

| Parameter | Value |
|---|---|
| Transport | UDP over Wi-Fi |
| Destination IP | `192.168.0.82` |
| Destination port | `5005` |
| Transmission interval | Approximately 200 ms |
| Approximate transmission rate | 5 packets per second |

Each UDP packet contains seven comma-separated fields:

```text
timestamp_ticks,acc_x,acc_y,acc_z,gyr_x,gyr_y,gyr_z
```

Example:

```text
1234,318,-173,977,120,-85,30
```

The timestamp is based on the ThreadX tick counter returned by `tx_time_get()`. Its conversion to seconds depends on the configured ThreadX tick rate.

The packet contains three accelerometer measurements in mg and three gyroscope measurements in mdps.

UDP does not guarantee packet delivery or ordering.

## Python Visualization

The Python visualizer receives the UDP packets and displays the sensor measurements as real-time plots using Matplotlib.

It shows two graphs:

- **Accelerometer:** X, Y, and Z acceleration in mg.
- **Gyroscope:** X, Y, and Z angular velocity in mdps.

The plots maintain a sliding window of the latest 50 samples and update approximately every 40 ms. The receiver runs in a background thread while Matplotlib updates the plots.

### Python Environment

The Python project uses `uv` and `pyproject.toml` to manage its virtual environment and dependencies.

Install `uv` if it is not already installed, then create the environment from the project root:

```bash
uv sync
```

Run the visualizer:

```bash
uv run python imu_visualizer.py
```

The visualizer listens on `0.0.0.0:5005`, allowing it to receive UDP packets sent to the PC on port `5005`.

The minimal receiver can be run separately:

```bash
uv run python imu_receiver.py
```

The PC and board must be able to communicate over the network. Windows Firewall must permit incoming UDP traffic on port `5005`.

## Development Environment

Firmware development is performed in Ubuntu 22.04 under WSL on Windows 11.

Install the required firmware build tools:

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

## Firmware Build Instructions

Navigate to the firmware directory:

```bash
cd ~/tele-link/firmware
```

Build the default `starter` configuration:

```bash
./scripts/build.sh
```

The build script configures the project with CMake and Ninja and compiles the firmware. The build directory is `firmware/build/`, and the deployment script expects the generated `.bin` file under `firmware/build/app/`.

To clean the existing build directory and reconfigure:

```bash
./scripts/build.sh starter rebuild
```

To clean the build directory before building again:

```bash
./scripts/build.sh starter clean
```

Inspect generated firmware binaries:

```bash
find build/app -maxdepth 1 -type f -name '*.bin' -print
```

## Firmware Deployment

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

Deploy the firmware:

```bash
cd ~/tele-link/firmware
./scripts/deploy.sh /mnt/e
```

The deployment script copies a `.bin` file from `build/app/` to the specified destination.

The script defaults to `/Volumes/AZ3166`, which is a macOS path. Under WSL, pass the actual mount path explicitly.

If multiple `.bin` files exist in the build directory, inspect them before deploying because the script selects the first matching file.

## Wi-Fi Configuration

The starter application's Wi-Fi configuration is located at:

```text
firmware/app/starter/cloud_config.h
```

Keep Wi-Fi credentials private and do not commit real passwords to the repository.

## Current Status

- [x] ThreadX-based firmware
- [x] LSM6DSL accelerometer readings
- [x] LSM6DSL gyroscope readings
- [x] Console output of sensor measurements
- [x] OLED display of acceleration and angular velocity
- [x] Wi-Fi connection
- [x] UDP transmission of IMU measurements to a PC
- [x] Python UDP receiver
- [x] Real-time accelerometer and gyroscope visualization
- [x] Firmware build and deployment scripts
