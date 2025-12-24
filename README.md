# esp-pd-heater

## Flashing (esptool)

Install:

```sh
python -m pip install --upgrade esptool
```

Option 1 (recommended): use the provided args file:

```sh
python -m esptool --chip esp32c3 --port /dev/ttyUSB0 --baud 460800 write_flash -z @flash_project_args
```

Option 2 (simplest): flash the merged image:

```sh
python -m esptool --chip esp32c3 --port /dev/ttyUSB0 --baud 460800 write_flash -z 0x0 merged-firmware.bin
```

Notes:

- Serial port examples: Linux `/dev/ttyUSB0` or `/dev/ttyACM0`, Windows `COM5`.

## LCD Variant Auto-Select (HSD / BOE)

This project supports two ST7735S-compatible LCD variants (HSD and BOE) without hardware detection.

Behavior:

- First boot after a full erase defaults to **HSD**.
- Until you **start heating** from the UI, the firmware will **toggle** the LCD variant on every boot: `HSD -> BOE -> HSD -> ...`.
- Once you **start heating** (UI heating switch ON), the current LCD variant is **locked** and will no longer toggle on reboot.

How to use:

- If the screen looks inverted/shifted, reboot the device (power cycle) **without starting heating** to try the other variant.
- When the screen looks correct, start heating once to lock the current variant.
- The selection/lock state is stored in NVS (namespace `lcd`, keys `variant` and `locked`) and is only cleared by a full erase.

Reset (full erase):

```sh
idf.py erase-flash
```

Or with esptool:

```sh
python -m esptool --chip esp32c3 --port /dev/ttyUSB0 erase_flash
```
