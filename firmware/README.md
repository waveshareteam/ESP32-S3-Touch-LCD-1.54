# Factory firmware

Each BIN is a complete merged image for its listed model (ESP32-S3, 16 MB Flash). Version dates are supplied by the hardware maintainer. Select the matching model; the touch and non-touch images are separate.

| File | Model / purpose | Flash offset |
| --- | --- | --- |
| [ESP32-S3-Touch-LCD-1.54_FactoryOnly_20260319.bin](ESP32-S3-Touch-LCD-1.54_FactoryOnly_20260319.bin) | ESP32-S3-Touch-LCD-1.54 factory firmware | `0x0` |
| [ESP32-S3-LCD-1.54_FactoryOnly_20260319.bin](ESP32-S3-LCD-1.54_FactoryOnly_20260319.bin) | ESP32-S3-LCD-1.54 factory firmware | `0x0` |

## Flashing

Erase Flash, then write the complete BIN from `0x0`. Replace `COM5` with the device port.

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

Erasing clears existing settings and user data. Historical example binaries and SD-card resources remain in [`../Firmware/`](../Firmware/). These new images are registered by model in `manifest.json`.
