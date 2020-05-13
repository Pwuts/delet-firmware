# Big Fat Delete button firmware

This repository contains the firmware I wrote for the protoboard in my big translucent <kbd>Del</kbd> key.
The thing now contains four 8mm WS2811 RGB LEDs and a Wemos D1 mini.

Demo video:  
[![demo video thumbnail](https://img.youtube.com/vi/wkcr0zEhDnk/0.jpg)](https://www.youtube.com/watch?v=wkcr0zEhDnk)

## Manual
* press the button to cycle between modes
* long press (>3 sec) the button to show free heap size
* long press (10 sec) to start WiFi portal

## What I want it to do
* be decoration
* use animations to indicate events on MQTT
* be an input device (e.g. as push-to-talk button)

## What it currently does
* be decoration
* WiFi configuration portal
* OTA update

## TODO
* add MQTT functionality
* get angled USB micro-B cable
* add USB input device functionality
