# Magic 8-Ball Wiring Guide

## Components Used
- NodeMCU v2 ESP8266 main board
- GY-521 breakout board with MPU6050 accelerometer
- 1.3" LCD screen SH1106
- HW-247A DFPlayer Mini sound module
- MicroSD card with sound files
- Speaker or headphones

## Wiring Connections

### SH1106 OLED Display (I2C)
- **VCC** → 3.3V on NodeMCU
- **GND** → GND on NodeMCU
- **SCL** → D1 (GPIO5) on NodeMCU
- **SDA** → D2 (GPIO4) on NodeMCU

### MPU6050 Accelerometer (I2C - shared with display)
- **VCC** → 3.3V on NodeMCU
- **GND** → GND on NodeMCU
- **SCL** → D1 (GPIO5) on NodeMCU
- **SDA** → D2 (GPIO4) on NodeMCU

### HW-247A DFPlayer Mini Sound Module
- **VCC** → 5V on NodeMCU (Important: Use 5V for reliable operation)
- **GND** → GND on NodeMCU
- **RX** → D0 (GPIO16) on NodeMCU
- **TX** → D3 (GPIO0) on NodeMCU
- **SPK_1** → Speaker positive
- **SPK_2** → Speaker negative
- **ADKEY_1** → Not used
- **ADKEY_2** → Not used
- **USB** → Not used
- **SD** → Insert microSD card with your sound files

**Note**: After extensive testing, D0/D3 pins were found to work reliably with the DFPlayer Mini. Other pin combinations (D5/D6, D7/D8) may not initialize properly.

## SD Card Setup

1. Format a microSD card as FAT32
2. Copy your sound files to the root directory of the SD card
3. Name the files as:
   - `0001.mp3` → whoosh.mp3
   - `0002.mp3` → arcade.mp3

**Important**: The DFPlayer Mini requires specific file naming:
- Files must be numbered sequentially starting from 0001
- Files must have the .mp3 extension
- Files should be in the root directory of the SD card

## Pin Summary
```
NodeMCU Pin | Component           | Wire Color Suggestion
------------|--------------------|-----------------------
3.3V        | OLED VCC, MPU VCC  | Red
5V          | DFPlayer VCC       | Red (thick)
GND         | All GND pins       | Black
D1 (GPIO5)  | OLED SCL, MPU SCL  | Yellow
D2 (GPIO4)  | OLED SDA, MPU SDA  | Blue
D0 (GPIO16) | DFPlayer RX        | Green
D3 (GPIO0)  | DFPlayer TX        | White
```

**Important Pin Notes:**
- D0/D3 combination is the only tested working configuration for DFPlayer Mini
- D3 (GPIO0) is also used during boot sequence, but works fine for DFPlayer TX
- Avoid using D1/D2 for DFPlayer as they're reserved for I2C devices

## Testing

1. **OLED Display**: Should show "Magic 8-Ball" welcome screen on startup
2. **MPU6050**: Shake detection should trigger responses (check serial monitor)
3. **DFPlayer**: Should play random sounds when responses are given
4. **SD Card**: Check serial monitor for "SD card detected with X files" message

## Troubleshooting

### OLED Display Issues
- Check I2C address (0x3C is common for SH1106)
- Verify power connections (3.3V, not 5V)
- Check SDA/SCL connections

### MPU6050 Issues
- Default I2C address is 0x68, but code uses alternate 0x69
- If shake detection isn't working, try pressing the built-in button (D3)

### DFPlayer Issues
- **Pin Configuration**: Use D0 (GPIO16) for RX and D3 (GPIO0) for TX - other combinations may fail
- Check SD card formatting (FAT32)
- Verify file naming (0001.mp3, 0002.mp3)
- Check serial monitor for DFPlayer initialization messages
- Try different volume levels in code
- **Power is critical**: DFPlayer Mini requires stable 5V for reliable operation
- If initialization fails, try power cycling the entire device
- Look for LED flashing on DFPlayer during initialization (indicates communication)

### Audio Issues
- Check speaker connections
- Adjust volume in code (0-30 range)
- Try different audio file formats/bitrates
- Ensure SD card is properly inserted

## Power Considerations

- The NodeMCU can be powered via USB or Vin pin
- DFPlayer Mini works best with 5V but can operate on 3.3V
- Total current draw is approximately 150-300mA depending on volume
- Use a quality USB cable or external power supply for best results
