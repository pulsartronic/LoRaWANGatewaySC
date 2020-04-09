# LoRa WAN Single Channel Gateway for WeMos ESP8266 Arduino

Instructions on how to make it work can be found here https://pulsartronic.com/blog/electronics/lorawan


# Contribute
An open source project is a forever Work In Progress. Feel free to be constructive.

RFM default configuration is in libraries/RFM/RFM.h you should change it based on your hardware
before upload, otherwise you can change it through the web interface

LoRaWAN default configuration is in library/WAN/WAN.h

# TODO
There are many todo's, the biggest is: DOWNLINKS are not scheduled, they are sent immediately as
soon as the gateway receives them. It would be a nice contribution :) it is in libraries/WAN/WAN.cpp
around the line 270

# Credits

AES for microcontrollers (Arduino & Raspberry pi)  
https://github.com/spaniakos/AES

JSON library for Arduino and embedded C++. Simple and efficient.  
https://github.com/bblanchon/ArduinoJson

An Arduino library for sending and receiving data using LoRa radios.
https://github.com/sandeepmistry/arduino-LoRa

WebSocket Server and Client for Arduino based on RFC6455  
https://github.com/Links2004/arduinoWebSockets

Base64 encoder/decoder for arduino  
https://github.com/Densaugeo/base64_arduino


