# MHI-AC-Ctrl for Home Assistant with better Temp./Hum. Sensors
This version/fork of MHI-AC-Ctrl is specificly setup for MQTT in combination with Home-assistant.
Reads and writes data (e.g. power, mode, fan status etc.) from/to a Mitsubishi Heavy Industries (MHI) air conditioner (AC) via SPI controlled by MQTT. The AC is the SPI master and the ESP8266 is the SPI slave.
Repo is cloned and forked from previous builds and Platform IO enabled. This makes all dependencies and build flags static in the project and more reproduceable on other machines.

**The current version does no longer require manual integration of yaml files in HA. It also creates a control panel where vanes can be set with logical names.**

<img src="images/HA-MHI-DashBoardControl.png" width=650 align="left" />

# Attention:
**warning:** You have to open the indoor unit to have access to the SPI. Opening of the indoor unit should be done by 
a qualified professional because faulty handling may cause leakage of water, electric shock or fire! :warning: 

# Prerequisites:
For use of the program you have to connect your ESP8266 (I use a ESP12-E module) with custom soldered board and level-shifters) via a
cable connector to your air conditioner. This has to be a split device (separated indoor and outdoor unit).
I assume that all AC units of the type "SRK xx ZS-S" / "SRC xx ZS-S" are supported. I use the indoor unit SRK 50 ZS-W and the outdoor unit SRC 50 ZS-W. Users reported that additionally the following models are supported:

- SRF xx ZJX-S1
- SRF xx ZMX-S
- SRF xx ZMXA-S
- SRF xx ZF-W
- SRK xx ZJ-S
- SRK xx ZM-S
- SRK xx ZS-S
- SRK xx ZJX-S
- SRK xx ZJX-S1
- SRK xx ZRA-W
- SRK xx ZSA-W
- SRK xx ZSX-S
- SRK xx ZSX-W
- SRK xx ZS-W
- SRR xx ZM-S
- SRK xx ZMXA-S

Unsupported models:

- SRK xx ZSPR-S
- SRK71ZEA-S1
 
If you find out that also other models are supported that are not listed here, please give feedback so that I can expand the list.

# Installing:

## Hardware (:construction_worker: Expirimental version SmOeL):
The ESP8266 is powered from the AC via LDO (12V -> 5V) converter :warning: this is right now not efficient. Maybe I will create schematic and layout for this HW-version--TBD 
The ESP8266 SPI signals SCL (SPI clock), MOSI (Master Out Slave In) and MISO (Master In Slave Out) are connected via a voltage level shifter 5V <-> 3.3V with the AC. Direct connection of the signals without a level shifter could damage your ESP8266!
More details are described in [Hardware.md](Hardware.md).

## Software Development Environment:
The program uses the following libraries (These are set in lib_deps of the pio.ini 
 - [MQTT client library](https://github.com/knolleary/pubsubclient) - don't use v2.8.0! (because of this [issue](https://github.com/knolleary/pubsubclient/issues/747)). Better use v2.7.0:
 - Webserver is running for OTA.
 
Clone the project and run 'pio run'
You could also use the recently updated version in the [src folder](src) but with the risk that it is more unstable. The stability of the program is better when it is compiled for a CPU frequency of 160MHz.
The configuration options are described in [SW-Configuration.md](SW-Configuration.md).

In a previous version and/or certain ESP chip/module versions, the HW-MISO pin seems to have crosstalk, this is solved by setting the MISO to GPIO16
This Software based SPI is reliable and the performance of the ESP8266 is sufficient for this use case.
In case of problems please check the [Troubleshooting guide](Troubleshooting.md).

## Setting up Home-Assistant configuration 
This project provides example YAML configurations for Home Assistant's MQTT integration.  
The files in [`ha_config/`](ha_config/) are structured for HA's `!include_dir_merge_list` pattern — one file per entity type, one folder per MQTT platform.

### Folder structure
```
ha_config/
├── airco/
│   └── MHI_AC_General.yaml    # Climate entity (modes, fan, vertical vanes)
├── select/
│   └── MHI_AC_VanesLR.yaml    # Horizontal vanes select (positions 1-7 + Swing)
└── switch/
    └── MHI_AC_3Dauto.yaml     # 3D Auto mode switch (On/Off)
```

### configuration.yaml integration
Copy the folders to your HA config directory (e.g. as `mqtt/airco/`, `mqtt/select/`, `mqtt/switch/`) and add:

```yaml
mqtt:
  climate: !include_dir_merge_list ./mqtt/airco
  select: !include_dir_merge_list ./mqtt/select
  switch: !include_dir_merge_list ./mqtt/switch
  # ... your other entity types ...
```

### VanesLR & 3D Auto (extended features)
The horizontal vanes (VanesLR) and 3D Auto entities require the extended 33-byte SPI frame.  
To enable in the firmware, uncomment the following line in [`src/support.h`](src/support.h):

```c
#define USE_EXTENDED_FRAME_SIZE true
```

| Entity | Type | MQTT State Topic | MQTT Command Topic | Values |
|--------|------|------------------|--------------------|--------|
| Vertical Vanes | climate swing_mode | `MHI-AC/Vanes` | `MHI-AC/set/Vanes` | 1, 2, 3, 4, Swing |
| Horizontal Vanes (LR) | select | `MHI-AC/VanesLR` | `MHI-AC/set/VanesLR` | 1–7, Swing |
| 3D Auto | switch | `MHI-AC/3Dauto` | `MHI-AC/set/3Dauto` | On, Off |

> **Note:** HA's MQTT Climate entity supports only one swing axis natively (`swing_mode`), which is used for the vertical vanes (Up/Down). The horizontal vanes are exposed as a separate `select` entity.

# Enhancement
If you are interested to have a deeper look on the SPI protocol or want to trace the SPI signals, please check [MHI-AC-Trace](https://github.com/absalom-muc/MHI-AC-Trace). But this is not needed for the standard user of MHI-AC-Ctrl. :construction_worker: I probably will have a look into this for future improvement.

# License
This project is licensed under the MIT License - see the LICENSE.md file for details

# Acknowledgments
The coding of the [SPI protocol](https://github.com/absalom-muc/MHI-AC-Trace/blob/main/SPI.md). [rjdekker's MHI2MQTT](https://github.com/rjdekker/MHI2MQTT) has made some great first steps in decoding the MHI protocol! Unfortunately rjdekker is no longer active on GitHub. He used an Arduino plus an ESP8266 for his project.
Also thank you very much on the authors and contributors of [MQTT client](https://github.com/knolleary/pubsubclient), [ArduinoOTA](https://github.com/esp8266/Arduino/tree/master/libraries/ArduinoOTA). Thank to all support on the absalom-muc version where this repo is forked from ((https://github.com/absalom-muc/MHI-AC-Ctrl)

