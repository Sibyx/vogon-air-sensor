# VogonAirFirmware

VogonAirFirmware is a simple air pollution sensor firmware for ESP32, developed using ESP-IDF. It reads data from
DHT22 (temperature and humidity sensor) and SDS011 (particulate matter sensor), and sends the measurements to a 
server using MQTT and WiFi for further processing.

**Work in progress**

## Features

- Reads temperature and humidity data from DHT22 sensor
- Reads particulate matter (PM2.5 and PM10) data from SDS011 sensor
- Sends sensor data to a server via MQTT
- Connects to WiFi for internet connectivity
- Easy configuration and setup

## Hardware Requirements

- ESP32 development board
- DHT22 temperature and humidity sensor
- SDS011 particulate matter sensor
- Breadboard and jumper wires for connections
- Power supply (5V USB or battery)

## Software Requirements

- [ESP-IDF](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/index.html) (Espressif IoT Development Framework)
- MQTT broker (e.g., Mosquitto)
- MQTT client for testing (e.g., MQTT.fx, MQTT Explorer)

## Installation

Follow the [official ESP-IDF setup guide](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/index.html) 
to install ESP-IDF and set up your development environment.

**Clone the Repository**

```sh
git clone https://github.com/Sibyx/vogon-air-sensor
cd vogon-air-sensor
```

**Configure the Project**

Use `idf.py menuconfig` to configure your project settings, such as WiFi credentials and MQTT server details.

**Build and Flash the Firmware**

```shell
idf.py build
idf.py flash
```

## Configuration

Configuration is done via the `menuconfig` tool. Key configuration options include:

## MQTT Topics and messages

By default, the firmware publishes sensor data to the following MQTT topic: `vogonair/:mac_address/raw`.

The compatible server configuration is in repository 
[vogon-veggie-collector](https://github.com/Sibyx/vogon-veggie-collector).

---
Feel free to contribute to this project by creating issues, submitting pull requests, or suggesting improvements. 
Happy hacking!