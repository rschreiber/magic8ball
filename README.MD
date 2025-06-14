# Magic8Ball Project

This project implements a Magic 8 Ball using the following components:

- **NodeMCUv2 ESP-8266** main board
- **GY-521 breakout board** with MPU6050 accelerometer
- **1.3" LCD screen** (SH1106 driver)

## Features

- Shake detection using the MPU6050 accelerometer
- Random Magic 8 Ball responses displayed on the SH1106 LCD

## Hardware Connections

| Component      | NodeMCU Pin | Notes                |
|----------------|-------------|----------------------|
| GY-521 (MPU6050) | SDA (D2)    | I2C Data             |
|                | SCL (D1)    | I2C Clock            |
| SH1106 LCD     | SDA (D2)    | Shared I2C Bus       |
|                | SCL (D1)    | Shared I2C Bus       |
| Power          | 3V3, GND    | To both modules      |

## Getting Started

1. Install [PlatformIO](https://platformio.org/) and ensure `pio` is available at `~\.platformio\penv\Scripts\pio`.
2. Clone this repository.
3. Connect the hardware as described above.
4. Build and upload the firmware:
    ```
    ~\.platformio\penv\Scripts\pio run --target upload
    ```
5. Shake the device to get a Magic 8 Ball response on the LCD.

## Libraries Used

- `Adafruit_MPU6050` or equivalent for MPU6050
- `U8g2` or `Adafruit_SH1106` for the LCD

## License

CC0 1.0 Universal
