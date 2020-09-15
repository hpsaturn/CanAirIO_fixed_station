# CanAirIO fixed station

Reduced and improved version for CanAirIO fixed stations. The current firmware is in development, and it should be support `ESP32` and `ESP8266` devices. The main code is a abstraction with custom parameters of WifiManager library for those microcontrollers.

# Firmware

For compiling and upload the current firmware, please firts install [PlatformIO](https://platformio.org/install) or include the project in your `Arduino IDE`. Also you need the [Git](https://git-scm.com/downloads) software. Please check that the commands `pio` and `git` works fine in your OS.

## Compiling and Installing

First, please clone this repo:
```python
git clone git@github.com:hpsaturn/CanAirIO_fixed_station.git
```

Connect your device to USB cable , enter to `CanAirIO` directory and run:

```python
pio run --target upload
```

You should have something like this:
```python
Building .pio/build/WEMOS/firmware.bin
Checking size .pio/build/WEMOS/firmware.elf
Advanced Memory Usage is available via "PlatformIO Home > Project Inspect"
RAM:   [=         ]  12.3% (used 40432 bytes from 327680 bytes)
Flash: [======    ]  64.9% (used 850800 bytes from 1310720 bytes)
esptool.py v2.6
===================== [SUCCESS] Took 8.44 seconds =======================

Environment    Status    Duration
-------------  --------  ------------
WEMOS          SUCCESS   00:00:08.441
TTGO_T7        IGNORED
nodemcuv2      IGNORED
===================== 1 succeeded in 00:00:08.441 ========================
```

Only in the first time, please upload the config file too:


```python
pio run --target uploadfs
```

# Usage

After the firmware installation you should be have a `CanAirIO Config Me` open wifi network, please connect and sign in on it for configure your CanAirIO device.