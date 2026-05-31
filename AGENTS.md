# Digital PD Power Supply — CH32X035 Project Notes

Digital USB-PD adjustable power supply using CH32X035G8U6 (WQFN-28).
Base project: [ch32fun](https://github.com/cnlohr/ch32fun) / [ch32v003fun](https://github.com/cnlohr/ch32v003fun).
Inspiration: [wagiminator/CH32X035-USB-PD-Adapter](https://github.com/wagiminator/CH32X035-USB-PD-Adapter).

---

## WQFN-28 Pinout (CH32X035G8U6)

Board uses **WQFN-28** (not UFQFPN20). Package drawing: `WQFN-28-1EP_4x4mm_P0.4mm_EP2.7x2.7mm`.

| Pin | Name | Available | Notes |
|-----|------|-----------|-------|
| 1 | PC15/CC2 | Yes | USB PD CC2 |
| 2 | VDD | — | 3.3V supply |
| 3 | PC0 | Yes | GPIO, J5-3 |
| 4 | PC3 | Yes | GPIO, J5-4 |
| 5 | PA0 | Yes | TIM2_CH1 / EXTI0, J5-5 |
| 6 | PA1 | Yes | TIM2_CH2 / EXTI1, J5-6 |
| 7 | PA2 | Yes | J5-7 |
| 8 | PA3 | Yes | J5-8 |
| 9 | PA4 | Yes | J5-9 |
| 10 | PA5 | Yes | J5-10 |
| 11 | PA6 | Yes | J5-11 |
| 12 | PA7 | Yes | J5-12 |
| 13 | PB0 | Yes | J3-1 |
| 14 | PB3 | Yes | J3-2 |
| 15 | PB4 | Yes | J3-3 |
| 16 | PB1/PB5 | Yes | **PB1+PB5 shorted internally** — J3-4 |
| 17 | PB6 | Yes | J3-5 |
| 18 | PB7 | Yes | J3-6 |
| 19 | PB8 | Yes | J3-7 |
| 20 | PB9 | Yes | J3-8 |
| 21 | PB10 | Yes | J3-9 |
| 22 | PB11 | Yes | J3-10 |
| 23 | PB12 | Yes | J3-11 |
| 24 | PC19/DCK | Yes | SWCLK (SDI debug), J2-4 |
| 25 | PC18/DIO | Yes | SWDIO (SDI debug), J2-3 |
| 26 | PC11/PC16/UDM | Yes | USB D- |
| 27 | PC10/PC17/UDP | Yes | USB D+ |
| 28 | PC14/CC1 | Yes | USB PD CC1 |
| 29 | GND (EP) | — | Exposed pad, ground |

**Not available on WQFN-28:** PA8-PA15, PB2, PB13-PB15, PC1-PC2, PC4-PC9, PC12-PC13, PC16-PC23.

---

## Pin Critical Facts

### PC18/PC19 — SDI Debug Lock (pins 25/24)
- **CAN be used as GPIO** by writing `AFIO->PCFR1 = (AFIO->PCFR1 & ~AFIO_PCFR1_SWJ_CFG) | AFIO_PCFR1_SWJ_CFG_DISABLE;`
- Use **GPIOC->CFGXR** (not CFGLR/CFGHR) for config: each nibble = `CNF[1:0] | MODE[1:0]`. Value `0x3` = Out_PP 50MHz.
- Use **GPIOC->BSXR** for set/reset: bits 0-7 = set, bits 16-23 = reset. PC18 = bit 2/18, PC19 = bit 3/19.
- `funDigitalWrite`/`funPinMode` macros only support pins 0-15; use CFGXR/BSXR directly for pins 16+.
- CFGXR config for extended pins PC16-PC23: each nibble = `MODE[1:0] | CNF[3:2]`. On WQFN-28, PC16/PC17 are internal aliases — CFGXR bits [3:0] (PC16) and [7:4] (PC17) still work through shared internal routing with PC11/PC10.
- **I2C1 on PC18/PC19**: remap via `AFIO_PCFR1_I2C1_REMAP` (bits [4:2] = 011). See "Hardware I2C Library" section.

### PC10/PC11 — USB Pins (pins 27/26)
- PC10/UDP = D+, PC11/UDM = D-.
- PC16 and PC17 are **internal aliases** for these USB signals — not separate physical pins on WQFN-28.
- `USB_init()` in usbcdc_handler.c writes CFGXR bits for PC16/PC17 — works correctly because they share internal routing with PC10/PC11.
- Bootloader D+ pull-low via PC17 BSXR works (confirmed on WQFN-28).
- Configured with `AFIO->CTLR` for USB PHY and pull-up.

### PB1/PB5 — WQFN-28 Internal Short (pin 16)
- PB1 and PB5 are **shorted together inside the chip** on CH32X035G8U6.
- Both IOs cannot be used for independent output functions simultaneously.
- Treat as one pin with two names. The schematic labels this pin "PB1".

### PA0/PA1 — Encoder Inputs (pins 5/6)
- TIM2_CH1 (PA0) / TIM2_CH2 (PA1) — default no remap needed.
- Also usable as EXTI0/EXTI1 for software quadrature decoding.

---

## PlatformIO Build System

All peripheral tests use **PlatformIO** with the `ch32v` platform and `ch32v003fun` framework.

### platformio.ini template
```ini
[env:genericCH32X035G8U6]
platform = ch32v
framework = ch32v003fun
monitor_speed = 115200
board = genericCH32X035G8U6
lib_deps = ../../ExtraLibs/usbcdc_libs
```

### Build & flash
```sh
pio run -t upload
pio device monitor -b 115200
```

### Including USB CDC in any project
```c
#include "../usbcdc_libs/usbcdc_internal.h"
CDC_init();
while (!CDC_connected());
CDC_write_buf("hello\r\n", 7);
```

No extra `ADDITIONAL_C_FILES` needed — PlatformIO handles it via `lib_deps`.

---

## USB CDC Serial Library — `ExtraLibs/usbcdc_libs/`

A C port of [CH32X035_USBSerial](https://github.com/jobitjoseph/CH32X035_USBSerial) (Arduino library), converted from C++ to pure C.

### Files
```
ExtraLibs/usbcdc_libs/
├── usbcdc_cdc.c         # CDC endpoint handlers, read/write/flush
├── usbcdc_config.h      # VID/PID (0x16C0/0x27DD), strings, UID serial
├── usbcdc_descr.c       # USB descriptors, string generation from UID
├── usbcdc_handler.c     # USB init, ISR, EP0 setup/control
├── usbcdc_internal.h    # Shared declarations, register compat defines
├── usbcdc_usb.h         # USB descriptor structs
└── usbfs_compat.h       # USBFS register bit defines
```

### Key API
```c
void CDC_init(void);                    // Initialize USB + CDC
uint8_t CDC_available(void);            // Bytes available to read
uint8_t CDC_connected(void);            // DTR set (host opened port)
char CDC_read(void);                    // Read one byte (blocks)
void CDC_write(char c);                 // Write one byte (flushes)
void CDC_write_buf(const char *d, int l); // Write buffer (non-blocking, overwrites pending)
```

### `CDC_write()` vs `CDC_write_buf()`
- `CDC_write()` is blocking — spins on `CDC_writeBusyFlag`. Used for interactive echo.
- `CDC_write_buf()` is **non-blocking** — directly overwrites the EP2 buffer and re-arms TX. Used for periodic output.
- Both set TX to ACK and rely on the host sending an IN token. If the host doesn't poll (picocom vs minicom), `CDC_write()` blocks forever. `CDC_write_buf()` always overwrites regardless.

### USB Pin Config Note (WQFN-28)
`USB_init()` in `usbcdc_handler.c` configures PC16/PC17 via CFGXR — this works on WQFN-28 because:
- PC16/PC17 are internal aliases for the USB D-/D+ signals.
- CFGXR bits [3:0] = PC16 → routes to PC11/UDM internally.
- CFGXR bits [7:4] = PC17 → routes to PC10/UDP internally.
- `AFIO->CTLR` with `USB_IOEN` enables the USB PHY regardless of CFGXR state.

---

## Hardware I2C Library — `lib_i2c.h`

**Author:** UniTheCat (MIT License)
**Used by:** `i2c-ina219/`, `i2c-oled-remap/`
**Source:** `Peripheral-test/i2c-ina219/src/lib_i2c.h`

Bit-bang-free hardware I2C master for CH32X03x/V30x. Uses the peripheral I2C engine with register-level control (no HAL).

### API
```c
// Initialize I2C peripheral
void i2c_init(I2C_TypeDef* I2Cx, u32 PCLK, u32 i2cSpeed_Hz);

// Master send: start + address(w) + data + stop
u8 i2c_sendBytes(I2C_TypeDef* I2Cx, u8 i2cAddress, u8* buffer, u8 len);
u8 i2c_sendByte(I2C_TypeDef* I2Cx, u8 i2cAddress, u8 data);

// Master receive: start + address(r) + read data + stop
u8 i2c_readBytes(I2C_TypeDef* I2Cx, u8 i2cAddress, u8* buffer, u8 len);

// Combined: write register address, then restart + read back data
u8 i2c_readRegTx_buffer(I2C_TypeDef* I2Cx, u8 i2cAddress,
                        u8 *tx_buf, u8 tx_len, u8 *rx_buf, u8 rx_len);
u8 i2c_readReg_buffer(I2C_TypeDef* I2Cx, u8 i2cAddress, u8 reg,
                      u8 *rx_buf, u8 rx_len);

// Scan I2C bus (calls callback for each found address)
void i2c_scan(I2C_TypeDef* I2Cx, void (*onPingFound)(u8 address));

// Slave mode
void i2c_slave_init(I2C_TypeDef* I2Cx, u16 self_addr, u32 PCLK, u32 i2cSpeed_Hz);
```

### I2C1 Remap to PC18/PC19 (UFQFPN20 and WQFN-28)
Default I2C1 pins (PA10/PA11) are not available in either package. Use remap 3:

```c
// 1. Disable SDI debug on PC18/PC19
AFIO->PCFR1 = (AFIO->PCFR1 & ~AFIO_PCFR1_SWJ_CFG) | AFIO_PCFR1_SWJ_CFG_DISABLE;
// 2. Set I2C1 remap: PC18=SDA, PC19=SCL
AFIO->PCFR1 = (AFIO->PCFR1 & ~AFIO_PCFR1_I2C1_REMAP) | I2C1_REMAP_VAL;
// 3. Configure PC18/PC19 as AF_OD 50MHz (0xF per nibble)
GPIOC->CFGXR = (GPIOC->CFGXR & ~(0xF << 8))  | (0xF << 8);   // PC18 = bits 8-11
GPIOC->CFGXR = (GPIOC->CFGXR & ~(0xF << 12)) | (0xF << 12);  // PC19 = bits 12-15
// 4. Initialize I2C
i2c_init(I2C1, 48000000, 100000);
```

---

## INA219 Current/Voltage/Power Sensor — `ina219.h`

**Used by:** `Peripheral-test/i2c-ina219/`

High-side DC current sensor with I2C interface, 0.1Ω shunt, ±320mA range.

### Register Map
| Addr | Name | Description |
|------|------|-------------|
| 0x00 | CONFIG | Configuration register |
| 0x01 | SHUNT | Shunt voltage (raw, ±320mV) |
| 0x02 | VOLTAGE | Bus voltage (LSB = 4mV, shift right 1) |
| 0x03 | POWER | Power (LSB = 128mW with CALIB=4096) |
| 0x04 | CURRENT | Current (LSB = 320µA with CALIB=4096) |
| 0x05 | CALIB | Calibration register |

### API
```c
void INA_init(I2C_TypeDef* I2Cx);              // Configure: 32V, ±320mA, 128 samples
uint16_t INA_readVoltage(I2C_TypeDef* I2Cx);   // Returns mV × 10 (divide by 10 for volts)
int16_t INA_readCurrent(I2C_TypeDef* I2Cx);    // Returns mA × 320 (divide by 320 for amps)
uint16_t INA_read(I2C_TypeDef* I2Cx, uint8_t reg);
void INA_write(I2C_TypeDef* I2Cx, uint8_t reg, uint16_t value);
```

### Conversion formulas (with CALIB=4096, 0.1Ω shunt)
```
Voltage (V) = raw_bus_register >> 1 × 4mV / 1000
            = raw_voltage / 10.0

Current (A) = raw_current / 320.0

Power (W) = raw_power / 128.0
```

### Default config (0b0010011111111111 = 0x27FF)
| Field | Bits | Value | Meaning |
|-------|------|-------|---------|
| BRNG | 13 | 1 | ±32V bus range |
| PG | 12:11 | 11 | ±320mV shunt (gain /8) |
| BADC | 10:7 | 1111 | 128 samples, 8.244ms |
| SADC | 6:3 | 1111 | 128 samples, 8.244ms |
| Mode | 2:0 | 111 | Continuous shunt+bus |

---

## USB PD Sink Library — `usbpd.h`

**Source:** `Peripheral-test/usbpd_sink_request/src/usbpd.h`

Single-header USB PD sink library for CH32X035. Uses the built-in USBPD peripheral on PC14(CC1)/PC15(CC2). Supports negotiation with USB PD sources and dynamic reconfiguration via PDO selection.

### Usage

```c
#define USBPD_IMPLEMENTATION   // in exactly ONE .c file
#include "usbpd.h"

USBPD_Init(eUSBPD_VCC_5V0);
while (eUSBPD_BUSY == USBPD_SinkNegotiate());
// Negotiated — now request desired voltage
```

### API

| Function | Description |
|----------|-------------|
| `USBPD_Init(vcc)` | Initialize USBPD peripheral on PC14/CC1 + PC15/CC2 |
| `USBPD_SinkNegotiate()` | Run state machine; returns `eUSBPD_OK` when PS is ready |
| `USBPD_SelectPDO(index, voltageIn100mV)` | Select a PDO (PPS: set target voltage in 100mV units) |
| `USBPD_GetCapabilities(&caps)` | Get source capabilities and PDO count |
| `USBPD_IsPPS(pdo)` | True if PDO is a PPS APDO |
| `USBPD_GetState()` | Current state machine state |
| `USBPD_GetVersion()` | Negotiated PD spec revision |
| `USBPD_Reset()` | Reset PD state machine |
| `USBPD_StateToStr(state)` | Debug string for state |
| `USBPD_ResultToStr(result)` | Debug string for result |

### Result Codes

```c
eUSBPD_OK, eUSBPD_BUSY, eUSBPD_ERROR,
eUSBPD_ERROR_ARGS, eUSBPD_ERROR_NOT_SUPPORTED, eUSBPD_ERROR_TIMEOUT
```

### State Machine

```
eSTATE_IDLE → eSTATE_CABLE_DETECT → eSTATE_SOURCE_CAP
  → eSTATE_WAIT_ACCEPT → eSTATE_WAIT_PS_RDY → eSTATE_PS_RDY
```

### PDO Types (union `USBPD_SourcePDO_t`)

| Type | Struct | Fields |
|------|--------|--------|
| Fixed Supply | `USBPD_SourceFixedSupplyPDO_t` | VoltageIn50mV, MaxCurrentIn10mA, PeakCurrent, flags |
| Variable Supply | `USBPD_VariablePDO_t` | Min/MaxVoltageIn50mV, MaxCurrentIn10mA |
| Battery Supply | `USBPD_BatteryPDO_t` | Min/MaxVoltageIn50mV, MaxPowerIn250mW |
| PPS APDO | `USBPD_SPR_PPS_APDO_t` | Min/MaxVoltageIn100mV, MaxCurrentIn50mA |
| EPR AVS APDO | `USBPD_EPR_AVS_APDO_t` | Min/MaxVoltageIn100mV, PDPIn1W |
| SPR AVS APDO | `USBPD_SPR_AVS_APDO_t` | MaxCurrent15To20V, MaxCurrent9to15V |

### Format Macros

Printf-ready macros for logging PDO contents:

```
FIXED_SUPPLY_FMT / FIXED_SUPPLY_FMT_ARGS(pdo)
VARIABLE_SUPPLY_FMT / VARIABLE_SUPPLY_FMT_ARGS(pdo)
BATTERY_SUPPLY_FMT / BATTERY_SUPPLY_FMT_ARGS(pdo)
SPR_PPS_FMT / SPR_PPS_FMT_ARGS(pdo)
EPR_AVS_FMT / EPR_AVS_FMT_ARGS(pdo)
SPR_AVS_FMT / SPR_AVS_FMT_ARGS(pdo)
```

Example:
```c
LOG(FIXED_SUPPLY_FMT, FIXED_SUPPLY_FMT_ARGS(pdo));
```

### Capabilities Message

```c
USBPD_SPR_CapabilitiesMessage_t caps;
size_t count = USBPD_GetCapabilities(&caps);
for (size_t i = 0; i < count; i++) {
  USBPD_SourcePDO_t* pdo = &caps.Source[i];
  // check pdo->Header.PDOType
}
```

Up to 7 PDOs in a single source capabilities message.

### `USBPD_SelectPDO()` Behavior

- **Non-PPS PDOs** (Fixed, Variable, Battery): `voltageIn100mV` ignored — selects by index, requests max current.
- **PPS PDOs**: clamps `voltageIn100mV` to PDO min/max, builds PPS RDO with requested voltage and max current.

### Pin Config (PC14/PC15)

- PC14 = CC1, PC15 = CC2 — configured as open-drain outputs.
- CC comparator threshold set to 0.66V.
- `USBPD_Init()` enables `RCC_USBPD`, sets `AFIO->CTLR` with `USBPD_IN_HVT` and optionally `USBPD_PHY_V33` for 3.3V VCC.

### Compile Option

In `funconfig.h`:
```c
#define FUNCONF_USBPD_NO_STR 1  // disable USBPD_StateToStr/ResultToStr to save flash
```

### Convenience Helpers (from `usbpd_sink_request.c`)

```c
// Match a fixed/variable PDO by exact voltage
bool USBPD_RequestVoltage(uint32_t target_mV) {
  USBPD_SPR_CapabilitiesMessage_t* caps;
  size_t count = USBPD_GetCapabilities(&caps);
  for (size_t i = 0; i < count; i++) {
    USBPD_SourcePDO_t* pdo = &caps->Source[i];
    switch (pdo->Header.PDOType) {
      case eUSBPD_PDO_FIXED:
        if (pdo->FixedSupply.VoltageIn50mV * 50 == target_mV)
          return USBPD_SelectPDO(i, 0) == eUSBPD_OK;
        break;
      case eUSBPD_PDO_VARIABLE:
        if (target_mV >= pdo->VariableSupply.MinVoltageIn50mV * 50 &&
            target_mV <= pdo->VariableSupply.MaxVoltageIn50mV * 50)
          return USBPD_SelectPDO(i, 0) == eUSBPD_OK;
        break;
      default: break;
    }
  }
  return false;
}

// Match a PPS APDO by voltage range
bool USBPD_RequestPPSVoltage(uint32_t target_mV) {
  USBPD_SPR_CapabilitiesMessage_t* caps;
  size_t count = USBPD_GetCapabilities(&caps);
  for (size_t i = 0; i < count; i++) {
    USBPD_SourcePDO_t* pdo = &caps->Source[i];
    if (USBPD_IsPPS(pdo)) {
      uint32_t v = target_mV / 100;
      if (v >= pdo->SPR_PPS.MinVoltageIn100mV &&
          v <= pdo->SPR_PPS.MaxVoltageIn100mV)
        return USBPD_SelectPDO(i, v) == eUSBPD_OK;
    }
  }
  return false;
}
```

### PD Sink Negotiation Pattern

```c
USBPD_Init(eUSBPD_VCC_5V0);
while (eUSBPD_BUSY == USBPD_SinkNegotiate());
// Now USBPD_SelectPDO() can be called at any time
USBPD_RequestVoltage(9000);  // request 9V
```

No re-negotiation needed — `USBPD_SelectPDO()` sends the request immediately via hardware.

---

## Projects

All projects under `Peripheral-test/`. Build with `pio run -t upload`.

| Project | What it does | Status |
|---------|-------------|--------|
| `Peripheral-test/tim2_encoder/` | TIM2 encoder mode on PA0(CH1)/PA1(CH2), USB CDC debug | Experimenting |
| `Peripheral-test/exti_encoder/` | EXTI-based quadrature decoder on PA0/PA1, USB CDC debug | Experimenting |
| `Peripheral-test/i2c-oled-remap/` | I2C1 remap to PC18/PC19, SSD1306 OLED | Working |
| `Peripheral-test/i2c-ina219/` | I2C1 remap to PC18/PC19, INA219 current/voltage sensor | Working |
| `Peripheral-test/usbpd_sink_request/` | USB PD sink, negotiates and cycles 5V/9V/12V/15V/20V | Working |

---

## Bootloader Entry

**Status: WORKING** on CH32X035G8U6 (WQFN-28). Uses wagiminator `BOOT_now()` method with D+ pull-low before reset.

### Working sequence (from `jump_bootloader/` — reference only, not in this repo)

```c
#define NVIC_RESETSYS  ((uint32_t)0x00000080)  // bit 7, NOT bit 3!

void BOOT_now(void) {
  FLASH->KEYR = FLASH_KEY1;
  FLASH->KEYR = FLASH_KEY2;
  FLASH->BOOT_MODEKEYR = FLASH_KEY1;
  FLASH->BOOT_MODEKEYR = FLASH_KEY2;
  FLASH->STATR |= FLASH_STATR_BOOT_MODE;   // 0x4000 (bit 14)
  FLASH->CTLR  |= FLASH_CTLR_LOCK;
  RCC->RSTSCKR |= RCC_RMVF;
  NVIC->CFGR = NVIC_RESETSYS | NVIC_KEY3;  // system reset
}

// Before calling BOOT_now():
// 1. Pull D+ (PC17 alias, routes to PC10/UDP) LOW for ~200ms
//    GPIOC->CFGXR |= (0x3 << 4);     // PC17 = Out_PP 50MHz
//    GPIOC->BSXR = 1 << 17;          // PC17 = low (BSXR bits 16-23 = reset)
//    Delay_Ms(200);
// 2. Release D+ back to input pull-up
//    GPIOC->CFGXR &= ~(0xF << 4);
//    GPIOC->CFGXR |= (8 << 4);       // PC17 = input pull-up
//    GPIOC->BSXR = 1 << 1;           // PC17 = high (set)
// 3. Call BOOT_now()
```

### Critical details
- `FLASH_STATR_BOOT_MODE` = **0x4000** (bit 14), NOT 0x400 (bit 10).
- `NVIC_RESETSYS` = **0x80** (bit 7), NOT 0x08 (bit 3).
- D+ must be pulled low then released before the reset.
- On WQFN-28, PC17 is an alias for PC10/UDP — BSXR bit 1 (set) / bit 17 (reset) works correctly.
- After reset, chip enumerates as WCH USB bootloader.
- Flash with: `wchisp flash firmware.bin`.

### Manual trigger (1200 baud)
```sh
python -c "import serial; serial.Serial('/dev/ttyACM0', 1200).close()"
# Then flash with: wchisp flash firmware.bin
```

---

## TIM2 Encoder Mode — `Peripheral-test/tim2_encoder/`

CH32X035 timers support quadrature encoder mode via the `SMS` bits in `SMCFGR`:

| SMS | Mode | Count edges |
|-----|------|------------|
| 001 | Encoder 1 | TI1 edges, TI2 = direction |
| 010 | Encoder 2 | TI2 edges, TI1 = direction |
| 011 | Encoder 3 | Both TI1 + TI2 edges (4x resolution) |

### Default TIM2 pins (no remap)
- TIM2_CH1 = **PA0** (pin 5, available on WQFN-28)
- TIM2_CH2 = **PA1** (pin 6, available on WQFN-28)

### Register setup
```c
RCC->APB1PCENR |= RCC_APB1Periph_TIM2;
funPinMode(PA0, GPIO_CFGLR_IN_PUPD);
funPinMode(PA1, GPIO_CFGLR_IN_PUPD);
TIM2->CTLR1 = 0;
TIM2->CHCTLR1 = (TIM2->CHCTLR1 & ~TIM_CC1S) | 0x0001 |
                (TIM2->CHCTLR1 & ~TIM_CC2S) | 0x0100;
TIM2->CHCTLR1 |= (0xF << 4) | (0xF << 12);  // IC1F=IC2F=0xF filter
TIM2->SMCFGR = (TIM2->SMCFGR & ~TIM_SMS) | 3;  // encoder mode 3
TIM2->ATRLR = 0xFFFF;
TIM2->CNT = 0x8000;    // center for signed reading
TIM2->CTLR1 |= TIM_CEN;
// Read: (int16_t)(TIM2->CNT - 0x8000)
```

### Gotchas
- `funPinMode` for CH32X03x uses a **macro** that shifts `mode` directly into CFGLR. Use `GPIO_CFGLR_IN_PUPD = 8` (clean 4-bit value), NOT `GPIO_Mode_IPU = 0x48` (extra bits bleed into adjacent pin nibbles).
- ch32fun's `mini_vsnprintf` does NOT support the `+` format flag. Use `%d` not `%+d`.
- Add IC1F/IC2F input filter (0xF = max) for mechanical encoder debounce. For heavy bounce, also use `IC1PSC` prescaler.

---

## EXTI Encoder — `Peripheral-test/exti_encoder/`

Software quadrature decoder using EXTI interrupts on PA0/PA1.

### How it works
- EXTI0 = PA0, EXTI1 = PA1 (default PORTA mapping, no AFIO config needed).
- Both rising + falling edges enabled → ISR fires 4x per full cycle.
- 4-state lookup table decodes direction from (old_state << 2) | new_state.
- Same resolution as TIM encoder mode 3, but uses CPU cycles per edge (~1-2µs at 48MHz).
- Pros: any 2 GPIO pins, software filtering possible.
- Cons: CPU overhead.

### Register setup
```c
funPinMode(PA0, GPIO_CFGLR_IN_PUPD);
funPinMode(PA1, GPIO_CFGLR_IN_PUPD);
EXTI->RTENR |= EXTI_INTENR_MR0 | EXTI_INTENR_MR1;  // rising
EXTI->FTENR |= EXTI_INTENR_MR0 | EXTI_INTENR_MR1;  // falling
EXTI->INTENR |= EXTI_INTENR_MR0 | EXTI_INTENR_MR1;  // enable irq
NVIC_SetPriority(EXTI7_0_IRQn, 0);
NVIC_EnableIRQ(EXTI7_0_IRQn);
```

### ISR
```c
void EXTI7_0_IRQHandler(void) __attribute__((interrupt));
void EXTI7_0_IRQHandler(void) {
  uint32_t cur = GPIOA->INDR & 3;
  uint32_t idx = (last_enc << 2) | cur;
  encoder_pos += enc_table[idx & 0xF];
  last_enc = cur;
  EXTI->INTFR = 0xFF;  // clear lines 0-7
}
```

### Lookup table
```
[prev][cur] → step: {0,1,-1,0, -1,0,0,1, 1,0,0,-1, 0,-1,1,0}
```

---

## WQFN-28 vs UFQFPN20 Comparison

| Feature | WQFN-28 (G8U6) | UFQFPN20 (F8U6) |
|---------|----------------|-----------------|
| Pins | 28 + EP | 20 |
| Package | 4×4mm | 3×3mm |
| PA0-PA2 | Yes | Yes |
| PA3-PA7 | **Yes** | No |
| PA9-PA14 | No | **Yes** |
| PB0-PB1 | Yes | Yes |
| PB3-PB12 | **Yes** (PB1+PB5 shared) | No (only PB10-12) |
| PC0-PC3 | PC0, PC3 only | PC0-PC3 |
| PC10/PC11 | **USB D+/D-** | No |
| PC14/PC15 | **CC1/CC2** | No |
| PC16/PC17 | Internal aliases only | **Separate pins, USB D-/D+** |
| PC18/PC19 | SDI + GPIO | SDI + GPIO |
| USB PD | CC1+CC2 both | CC1 only (PC14) |
| Default I2C1 (PA10/PA11) | No | **Yes** |
| I2C via PC18/PC19 remap | Required | Required |
