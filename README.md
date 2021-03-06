# Project Probe

This is a control system serves to build an independent probe, which based on Raspberry Pi + ESP32. Pi controller is directly accessed, and one of the ESP32 connected to Pi is used as BLE server. Another inside the probe is the client and data collector. 

## Hardware

- ESP32 Wrover E x 2
- 3.7 V battery 200mAh x 2
- Raspberry Pi 4B

## Features

- BLE for data communication 
- OTA update through WIFI
- 12bit ADC with up to 400kHz sampling rate
- Deep sleep in idle time, with ~10uA power consuming