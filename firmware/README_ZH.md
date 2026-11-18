# 出厂固件

每个 BIN 均为对应型号的完整合并镜像（ESP32-S3、16 MB Flash），版本日期由硬件维护者提供。请按型号选择，触摸与非触摸型号分别使用各自的镜像。

| 文件 | 适用型号及用途 | 烧录偏移 |
| --- | --- | --- |
| [ESP32-S3-Touch-LCD-1.54_FactoryOnly_20260319.bin](ESP32-S3-Touch-LCD-1.54_FactoryOnly_20260319.bin) | ESP32-S3-Touch-LCD-1.54 出厂固件 | `0x0` |
| [ESP32-S3-LCD-1.54_FactoryOnly_20260319.bin](ESP32-S3-LCD-1.54_FactoryOnly_20260319.bin) | ESP32-S3-LCD-1.54 出厂固件 | `0x0` |

## 烧录

先整片擦除 Flash，再从 `0x0` 烧录完整 BIN。将 `COM5` 替换为实际串口。

### ESP32-S3-Touch-LCD-1.54

```shell
python -m esptool --chip esp32s3 --port COM5 erase-flash
python -m esptool --chip esp32s3 --port COM5 write-flash 0x0 ESP32-S3-Touch-LCD-1.54_FactoryOnly_20260319.bin
```

### ESP32-S3-LCD-1.54

```shell
python -m esptool --chip esp32s3 --port COM5 erase-flash
python -m esptool --chip esp32s3 --port COM5 write-flash 0x0 ESP32-S3-LCD-1.54_FactoryOnly_20260319.bin
```

整片擦除会清除已有设置及用户数据。历史示例镜像和 SD 卡资源保留在 [`../Firmware/`](../Firmware/)；本次镜像由 `manifest.json` 按型号分别声明。
