# esp-pd-heater

English | [简体中文](README.zh-CN.md)

## Flashing With ESP Flash Download Tool

This project targets `ESP32-C3`. You can download `ESP Flash Download Tool` from Espressif's official page here:

- https://docs.espressif.com/projects/esp-test-tools/en/latest/esp32/production_stage/tools/flash_download_tool.html

### Option 1: Single File

Flash the packaged merged image as a single file.

1. Open `ESP Flash Download Tool`.
2. Select `ChipType = ESP32-C3`.
3. Select `WorkMode = Develop`.
4. Select `LoadMode = UART`.
5. Add `merged-firmware.bin` at address `0x0`.
6. Set `SPI SPEED = 80MHz`, `SPI MODE = DIO`, `FLASH SIZE = 4MB`.
7. Select the correct `COM` port and baud rate.
8. Click `START`.

### Option 2: Multi-File

Flash the packaged files with the addresses listed in `flash_project_args`.

1. Open `dist/flash_project_args`.
2. Use the first line as the common flash settings:

```text
--flash_mode dio --flash_freq 80m --flash_size 4MB
```

3. Add each remaining `address + file` row from `flash_project_args` into `Download Path Config` and check every row.
4. Set `ChipType = ESP32-C3`, `WorkMode = Develop`, `LoadMode = UART`.
5. Set `SPI SPEED = 80MHz`, `SPI MODE = DIO`, `FLASH SIZE = 4MB`.
6. Select the correct `COM` port and baud rate.
7. Click `START`.

## Flashing With esptool

Install:

```sh
python -m pip install --upgrade esptool
```

Option 1: use the provided args file.

```sh
python -m esptool --chip esp32c3 --port /dev/ttyUSB0 --baud 460800 write_flash -z @flash_project_args
```

Option 2: flash the merged image.

```sh
python -m esptool --chip esp32c3 --port /dev/ttyUSB0 --baud 460800 write_flash -z 0x0 merged-firmware.bin
```

Serial port examples:

- Linux: `/dev/ttyUSB0` or `/dev/ttyACM0`
- Windows: `COM5`

## LCD Variant Auto-Select

This project supports two ST7735S-compatible LCD variants, `HSD` and `BOE`, without hardware detection.

Behavior:

- After a full erase, the first boot defaults to `HSD`.
- Until heating is started from the UI, the firmware toggles the LCD variant on every boot: `HSD -> BOE -> HSD -> ...`
- After heating is started once, the current LCD variant is locked and no longer toggles on reboot.

How to use:

- If the screen looks inverted or shifted, power-cycle the device without starting heating.
- When the screen looks correct, start heating once to lock the current variant.
- The selection is stored in NVS under namespace `lcd`, keys `variant` and `locked`.

Full erase:

```sh
idf.py erase-flash
```

Or with esptool:

```sh
python -m esptool --chip esp32c3 --port /dev/ttyUSB0 erase_flash
```
