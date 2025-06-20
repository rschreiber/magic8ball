# Magic8Ball Project

This project implements a Magic 8 Ball using the following components:

- **NodeMCUv2 ESP-8266** main board
- **GY-521 breakout board** with MPU6050 accelerometer
- **1.3" LCD screen** (SH1106 driver)
- **HW-247A DFPlayer Mini** sound module
- **MicroSD card** with sound files

## Features

- Shake detection using the MPU6050 accelerometer
- Random Magic 8 Ball responses displayed on the SH1106 LCD
- Random sound effects (whoosh.mp3 or arcade.mp3) played when giving responses
- Animated display with shake effects and fade-in responses
- Fallback button input if accelerometer is not working

## Hardware Connections

| Component         | NodeMCU Pin | Notes                    |
|-------------------|-------------|--------------------------|
| GY-521 (MPU6050)  | SDA (D2)    | I2C Data                 |
|                   | SCL (D1)    | I2C Clock                |
| SH1106 LCD        | SDA (D2)    | Shared I2C Bus           |
|                   | SCL (D1)    | Shared I2C Bus           |
| HW-247A DFPlayer  | RX (D0)     | Serial communication     |
|                   | TX (D3)     | Serial communication     |
|                   | SPK_1/SPK_2 | Speaker connections      |
| Power             | 3V3/5V, GND | To all modules           |

See [WIRING.md](WIRING.md) for detailed connection instructions.

## SD Card Setup

1. Format a microSD card as FAT32
2. Copy your sound files to the root directory:
   - Rename `whoosh.mp3` to `0001.mp3`
   - Rename `arcade.mp3` to `0002.mp3`
3. Insert the SD card into the DFPlayer Mini module

## Getting Started

1. Install [PlatformIO](https://platformio.org/) and ensure `pio` is available at `~\.platformio\penv\Scripts\pio`.
2. Clone this repository.
3. Connect the hardware as described in [WIRING.md](WIRING.md).
4. Prepare the SD card with sound files as described above.
5. Build and upload the firmware:
    ```
    ~\.platformio\penv\Scripts\pio run --target upload
    ```
6. Shake the device to get a Magic 8 Ball response on the LCD with sound effects.

## Libraries Used

- `U8g2` for the SH1106 OLED display
- `DFRobotDFPlayerMini` for sound playback
- Custom MPU6050 implementation for shake detection

## License

CC0 1.0 Universal
