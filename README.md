# CanAirIO_fixed_station

Reduced and improved version for CanAirIO fixed stations

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
Building .pio/build/ttgo-display/firmware.bin
RAM:   [==        ]  19.7% (used 64684 bytes from 327680 bytes)
Flash: [========= ]  86.4% (used 1698140 bytes from 1966080 bytes)
esptool.py v2.6
============== [SUCCESS] Took 33.86 seconds ==================

Environment       Status    Duration
----------------  --------  ------------
ttgo-display      SUCCESS   00:00:33.861
ttgo-display-ota  IGNORED
============= 1 succeeded in 00:00:33.861 =====================
```

Only in the first time, please upload the config file too:


```python
pio run --target uploadfs
```

# Usage

After the firmware installation you should be have a `CanAirIO Config Me` open wifi network, please connect and sign in on it for configure your CanAirIO device.