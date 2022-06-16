# ESP8266 SimpleHome Temperature Sensor

## About This Project
This project is a little operating system for the ESP8266 that uses the [SimpleHome API](https://github.com/Domi04151309/HomeApp/wiki/SimpleHome-API).
It lets you see the current temperature and humidity of your room at a glance.
You need the app [Home App](https://github.com/Domi04151309/HomeApp) for this project.

## Parts
- Breadboard
- Wires
- NodeMCU ESP8266
- DHT22
- 10k&#8486; Resistor

## Schematic
![Schematic](https://raw.githubusercontent.com/Domi04151309/ESP8266SimpleHomeTemperature/main/schematic.png)

## Setup

### Fresh Install
Just install the sketch on your ESP8266 and wait for a WiFi called `ESP8266 SimpleHome`.
The password is `00000000`. You can then open `http://192.168.1.1/` in your browser to connect the device to your home network.
After your device has connected to your home network you can use the app to find it automatically.
Add the device to your list and connect to it.
You should see temperature and humidity now.

### Changed WiFi
If you changed your WiFi name or password and the device is unable to connect, it will open its own access point.
You can then continue like it's a fresh install.

### Additional Info
The device is ready as soon as the onboard LED turns off.

### Modified Libraries
The `libs` folder includes modified versions of libraries to improve efficiency.

## Legal Notice
Copyright (C) 2021 Domi04151309

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
