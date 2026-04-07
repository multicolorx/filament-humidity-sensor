# Hand-Soldering BOM

This BOM is grouped for bench assembly rather than purchasing automation. It focuses on package size, soldering difficulty, and the order that will make assembly easier.

## Assembly Order

1. Solder the smallest and flattest SMD parts first: `R1-R7`, `C1-C14`, `Y1`.
2. Solder the small ICs next: `IC2`, `Q1`, `IC3`, `IC4`.
3. Solder the MCU `IC1` after the surrounding passives are done and cleaned.
4. Finish with through-hole connectors: `J1-J5`.
5. Connect the battery last and power up with current limiting the first time.

## BOM

| Qty | References | Part / Function | Value | Package / Footprint | Hand soldering note |
| --- | --- | --- | --- | --- | --- |
| 11 | `C1-C5`, `C7-C8`, `C10`, `C12-C14` | Ceramic capacitors | `100 nF` / `0.1 uF` | `0805` | Easy. Good starter parts for alignment and paste amount. |
| 1 | `C11` | Ceramic capacitor | `1 uF` | `0805` | Easy. |
| 2 | `C6`, `C9` | Crystal load capacitors | `20 pF` | `0805` | Easy, but keep flux residue low around the crystal network. |
| 3 | `R1`, `R5`, `R6` | Resistors | `10 k` | `0805` | Easy. |
| 3 | `R2-R4` | Resistors | `1 k` | `0805` | Easy. |
| 1 | `R7` | Resistor | `100 k` | `0805` | Easy. |
| 1 | `Y1` | 32.768 kHz crystal | `ABS07-32.768KHZ-T` | `ABS07` | Moderate. Small part, but still manageable with iron or hot air. |
| 1 | `IC2` | Battery protection IC | `DW01A-G` | `SOT-23-6` | Moderate. Fine-tip iron is enough. |
| 1 | `Q1` | Dual MOSFET for battery protection | `UMW8205A` | `SOT-23-6` | Moderate. Similar difficulty to `IC2`. |
| 1 | `IC3` | 3.3 V LDO regulator | `TCR2LF33,LM_CT` | Small SMD regulator package | Moderate. Watch orientation carefully. |
| 1 | `IC4` | Temperature / humidity sensor | `SHT40-CD1B-R3` | `SHT40CD1BR3` | Hard. Best done with paste + hot air because of the compact no-lead package. |
| 1 | `IC1` | MCU | `STM32L053R8T6` | `LQFP-64` | Moderate to hard. Drag soldering works well with flux. |
| 2 | `J1`, `J2` | Battery wire / terminal connection points | `Conn_01x01` | `PinHeader_1x01_P2.54mm_Vertical` | Very easy. Through-hole. |
| 2 | `J3`, `J4` | LCD headers | `Conn_01x05` | `PinHeader_1x05_P2.00mm_Vertical` | Very easy. Through-hole. |
| 1 | `J5` | SWD programming header | `Conn_01x05` | `PinHeader_1x05_P2.54mm_Vertical` | Very easy. Through-hole. |
| 1 | `BT1` | Battery cell in schematic only | `Battery_Cell` | No PCB footprint assigned | Off-board item. Treat this as the external battery source, not a PCB-mounted part. |

## Notes

- The board is mostly friendly to hand assembly because the passives are `0805` and the connectors are through-hole.
- The two parts most likely to slow you down are `IC4` (`SHT40`) and `IC1` (`STM32L053R8T6`).
- If you are ordering extras, it is worth buying spare `IC4`, `IC2`, and `Q1`.
- Clean flux especially well around `IC4`, `Y1`, and the MCU pins before first bring-up.
