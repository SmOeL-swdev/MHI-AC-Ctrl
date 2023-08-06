# MHI-AC-Ctrl for Home Assistant with better Ext. Sensors
This version/fork of MHI-AC-Ctrl is specificly setup for MQTT in combination with Home-assistant.
Reads and writes data (e.g. power, mode, fan status etc.) from/to a Mitsubishi Heavy Industries (MHI) air conditioner (AC) via SPI controlled by MQTT. The AC is the SPI master and the ESP8266 is the SPI slave.
Repo is cloned and forked from previous builds and Platform IO enabled. This makes all dependencies and build flags static in the project and more reproduceable on other machines.

<img src="/images/HA-MHI-CTRL_Settings.png" width=650 align="left" />

# Attention:
:warning: You have to open the indoor unit to have access to the SPI. Opening of the indoor unit should be done by 
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

In a previous version the HW-MISO pin seems to have crosstalk, this is solved by setting the MISO to GPIO16
This Software based SPI is reliable and the performance of the ESP8266 is sufficient for this use case.
In case of problems please check the [Troubleshooting guide](Troubleshooting.md).

## Setting up Home-Assistant configuration 
:construction_worker: Future improvement of the manual how this needs to be set-up :construction_worker:

# Enhancement
If you are interested to have a deeper look on the SPI protocol or want to trace the SPI signals, please check [MHI-AC-Trace](https://github.com/absalom-muc/MHI-AC-Trace). But this is not needed for the standard user of MHI-AC-Ctrl. :construction_worker: I probably will have a look into this for future improvement.

# License
This project is licensed under the MIT License - see the LICENSE.md file for details

# Acknowledgments
The coding of the [SPI protocol](https://github.com/absalom-muc/MHI-AC-Trace/blob/main/SPI.md). [rjdekker's MHI2MQTT](https://github.com/rjdekker/MHI2MQTT) has made some great first steps in decoding the MHI protocol! Unfortunately rjdekker is no longer active on GitHub. He used an Arduino plus an ESP8266 for his project.
Also thank you very much on the authors and contributors of [MQTT client](https://github.com/knolleary/pubsubclient), [ArduinoOTA](https://github.com/esp8266/Arduino/tree/master/libraries/ArduinoOTA). Thank to all support on the absalom-muc version where this repo is forked from ((https://github.com/absalom-muc/MHI-AC-Ctrl)

