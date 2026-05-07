# esp-pd-heater

[English](README.md) | 简体中文

## 使用 ESP Flash Download Tool 烧录

本项目目标芯片为 `ESP32-C3`，`ESP Flash Download Tool` 可从乐鑫官方页面下载：

- https://docs.espressif.com/projects/esp-test-tools/zh_CN/latest/esp32/production_stage/tools/flash_download_tool.html

### 方法一：单文件烧录

烧录打包后的单文件合并镜像

1. 打开 `ESP Flash Download Tool`
2. 选择 `ChipType = ESP32-C3`
3. 选择 `WorkMode = Develop`
4. 选择 `LoadMode = UART`
5. 添加 `merged-firmware.bin`，地址填 `0x0`
6. 设置 `SPI SPEED = 80MHz`、`SPI MODE = DIO`、`FLASH SIZE = 4MB`
7. 选择正确的 `COM` 口和波特率
8. 点击 `START`

### 方法二：多文件烧录

按 `flash_project_args` 中列出的地址烧录打包后的各个文件

1. 打开 `dist/flash_project_args`
2. 第一行是通用烧录参数：

```text
--flash_mode dio --flash_freq 80m --flash_size 4MB
```

3. 将 `flash_project_args` 后续每一行的 `地址 + 文件名` 填入 `Download Path Config`，并勾选每一行
4. 设置 `ChipType = ESP32-C3`、`WorkMode = Develop`、`LoadMode = UART`
5. 设置 `SPI SPEED = 80MHz`、`SPI MODE = DIO`、`FLASH SIZE = 4MB`
6. 选择正确的 `COM` 口和波特率
7. 点击 `START`

## 使用 esptool 烧录

安装：

```sh
python -m pip install --upgrade esptool
```

方法一：使用提供的参数文件

```sh
python -m esptool --chip esp32c3 --port /dev/ttyUSB0 --baud 460800 write_flash -z @flash_project_args
```

方法二：烧录合并镜像

```sh
python -m esptool --chip esp32c3 --port /dev/ttyUSB0 --baud 460800 write_flash -z 0x0 merged-firmware.bin
```

串口示例：

- Linux：`/dev/ttyUSB0` 或 `/dev/ttyACM0`
- Windows：`COM5`

## LCD 型号自动切换

本项目支持两种 ST7735 的 LCD 版本：`HSD` 和 `BOE`

行为说明：

- 完整擦除后首次启动默认使用 `HSD`
- 在 UI 中未启动加热前，每次上电都会在两种 LCD 版本之间切换：`HSD -> BOE -> HSD -> ...`
- 只要在界面中成功启动过一次加热，当前 LCD 版本就会被锁定，后续重启不再切换

使用方法：

- 如果画面倒置或偏移，不要启动加热，直接断电重启一次
- 当屏幕显示正常后，启动一次加热以锁定当前 LCD 版本
- 该选择保存在 NVS 中，命名空间是 `lcd`，键为 `variant` 和 `locked`

完整擦除：

```sh
idf.py erase-flash
```

或使用 esptool：

```sh
python -m esptool --chip esp32c3 --port /dev/ttyUSB0 erase_flash
```
