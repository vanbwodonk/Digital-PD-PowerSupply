# Digital USB-PD Power Supply

Low Cost USB Power Delivery adjustable power supply using CH32X035 RISC-V MCU.

- **MCU:** CH32X035G8U6 (WQFN-28), 48MHz RISC-V, USB PD PHY, USB 2.0 FS
- **Power:** USB Type-C PD (sink) → adjustable output voltage/current
- **Peripherals:** Rotary encoder control, OLED display, INA219 current sensing, USB CDC serial
- **Firmware:** C with ch32v003fun framework, PlatformIO build
- **Hardware:** Custom WQFN-28 dev board (KiCad design in `Hardware/`)
