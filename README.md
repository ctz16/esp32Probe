# Project Probe

This is a control system serves to build an independent probe, which based on the framework Raspberry Pi + ESP32. Pi controller is directly accessed, and one of the ESP32 connected to Pi is used as BLE server. Another ESP32 in the probe is the client and data collector. 



## Hardware

- ESP32 Wrover-E x 2
- 3.7 V battery 200mAh x 2
- Raspberry Pi 4B

## Features

- BLE for data communication 
- OTA update through WIFI
- 12bit on-chip ADC with up to 400kHz sampling rate (i2s)
- Deep sleep in idle time, with ~10uA power consuming
- shake to wakeup the probe through bma400 accelerometer