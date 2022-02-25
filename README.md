[![PlatformIO](https://github.com/hpsaturn/CanAirIO_fixed_station/workflows/PlatformIO/badge.svg)](https://github.com/hpsaturn/CanAirIO_fixed_station/actions/) ![ViewCount](https://views.whatilearened.today/views/github/hpsaturn/CanAirIO_fixed_station.svg)

# CanAirIO Captive Portal Version

Reduced version for a possible CanAirIO device for fixed stations. The current firmware is in **development**, and it should be support `ESP32` and `ESP8266` devices. The main code is a abstraction with custom parameters of WifiManager library for those microcontrollers. Also it use the [canairio_sensorlib](https://github.com/kike-canaries/canairio_sensorlib) library for handled multiple sensors and **influxdb** for save the output data, you also can use the CanAirIO test server or your custom instance.

## Firmware

For compiling and upload the current firmware, please firts install [PlatformIO](https://platformio.org/install) or include the project in your `Arduino IDE`. Also you need the [Git](https://git-scm.com/downloads) software. Please check that the commands `pio` and `git` works fine in your OS.

## Compiling and Installing

First, please clone this repo:

```python
git clone git@github.com:hpsaturn/CanAirIO_fixed_station.git
```

Connect your device to USB cable , enter to `CanAirIO` directory and run:

### NODEMCUV2 (default)

```python
pio run --target upload
```

You should have something like this:

```python
Building .pio/build/NODEMCUV2/firmware.bin
RAM:   [=         ]  12.3% (used 40432 bytes from 327680 bytes)
Flash: [======    ]  64.9% (used 850800 bytes from 1310720 bytes)
===================== [SUCCESS] Took 8.44 seconds ======================

Environment    Status    Duration
-------------  --------  ------------
TTGO_T7        SUCCESS   00:00:08.441
WEMOS          IGNORED
nodemcuv2      IGNORED
===================== 1 succeeded in 00:00:08.441 ======================
```

for others boards please select your environment:

### ESP8266

The current environments are WEMOS, TTGO_T7, nodemcuv2 and esp12e, but please review the `platformio.ini` file for more.
You can compile and upload the variant for example with:

```python
pio run -e ESP12E --target upload
```

## Upload the config data

Please edit the **config.json** file on `data` directory and set the parameters for your fixed station and only in the first time, please upload the config file with:

```python
pio run --target uploadfs
```

The config json file is like this:

```json
{
  "influxdb":"canairio",
  "hostname":"influxdb.canair.io",
  "port":"8086",
  "geohash":"9q8yv",
  "stime":"10",
  "stype":"0"
}
```

## Usage

After the firmware installation you will should be have a `CanAirIO Config` wifi network (Hostpot), please connect and sign in on it for configure your CanAirIO device. The current config passw is `CanAirIO`.

---
