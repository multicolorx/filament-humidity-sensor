# Filament Humidity Sensor

Battery-powered filament storage humidity monitor based on `STM32L053` + `SHT40`, with segmented LCD drive and Li-ion power input.

## Overview

This repository contains the hardware design files for a compact humidity sensor intended for filament storage monitoring.

The board is built around:
- `STM32L053R8T6` low-power MCU (with LCD segment peripheral)
- `SHT40-CD1B-R3` digital humidity/temperature sensor (`I2C`)
- Single-cell Li-ion input (`18650` compatible)
- Onboard battery protection (`DW01A` + `UMW8205A`)
- `TCR2LF33` 3.3 V LDO regulator
- Segmented LCD interface via two 1x5 headers

## Repository Layout

- `filament-humidity-sensor/filament-humidity-sensor.kicad_sch` schematic
- `filament-humidity-sensor/filament-humidity-sensor.kicad_pcb` PCB layout
- `filament-humidity-sensor/filament-humidity-sensor.kicad_pro` KiCad project
- `datasheets/` component datasheets used in the design

## Hardware Interfaces

### Battery Input

- `J1` (`1x01`, 2.54 mm): `BAT+`
- `J2` (`1x01`, 2.54 mm): `BAT-`

### SWD Programming Header (`J5`, 1x05, 2.54 mm)

1. `+3V3`
2. `SWDIO`
3. `GND`
4. `SWCLK`
5. `NRST`

Use an ST-Link compatible probe for programming/debugging.

### LCD Headers

`J3` (`1x05`, 2.00 mm)
1. `LCD_SEG5`
2. `LCD_COM0`
3. `LCD_COM1`
4. `LCD_COM2`
5. `LCD_COM3`

`J4` (`1x05`, 2.00 mm)
1. `LCD_SEG0`
2. `LCD_SEG1`
3. `LCD_SEG2`
4. `LCD_SEG3`
5. `LCD_SEG4`

## Getting Started

1. Open `filament-humidity-sensor/filament-humidity-sensor.kicad_pro` in KiCad 9.
2. Inspect schematic and run ERC.
3. Open PCB and run DRC.
4. Generate Gerber files.
5. Generate drill files.
6. Generate pick-and-place files (if assembly is planned).

## Firmware

This repository currently contains hardware design files only. Add firmware in a separate folder (for example `firmware/`) and document flashing/build steps here.

## Notes

- The design uses a single-cell Li-ion source; follow proper battery safety practices.
- Verify connector orientation and pin 1 markings before first power-up.
- Review all datasheets in `datasheets/` before ordering parts or assembling prototypes.

## License

This project is licensed under `GPL-3.0` (see `LICENSE`).
