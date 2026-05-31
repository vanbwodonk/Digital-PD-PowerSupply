# USB PD Sink Request Voltage Project

This project implements a USB Power Delivery sink that requests fixed voltages based on button input.

## Features
- USB PD sink functionality using ch32fun framework
- Button input on PC0 to cycle through voltage levels (5V, 9V, 12V, 15V, 20V)
- USB CDC serial communication for debugging
- PlatformIO build system

## Hardware
- CH32X035G8U6 (WQFN-28) MCU
- USB Type-C connector for PD communication
- Button connected to PC0 with internal pull-up

## Usage
1. Connect to USB CDC serial port
2. Press button to cycle through voltage levels
3. Monitor serial output for voltage requests

## Build
```sh
pio run -t upload
```

## Flash
```sh
wchisp flash firmware.bin
```