# LoRa WAN Single Channel Gateway for WeMos ESP8266 Arduino

Instructions on how to make it work can be found here https://www.hackster.io/pulsartronic/lorawan-gateway-esp8266-rfm95-arduino-4914a8  
IMPORTANT: I don't know why, but the latest version of the ESP8266 library (v3.0.2 at this time) does not correctly deliver the .html. It seems to work just fine with v2.7.4

# Contribute
An open source project is a forever Work In Progress. Feel free to be constructive.

RFM default configuration is in:
```sh
libraries/RFM/RFM.h
```
You should change it based on your hardware before uploading it to your board, although you can later change it through the web interface.
LoRaWAN default configuration is in:
```sh
libraries/WAN/WAN.h
```

# TODO
There are many todo's, the biggest is: CAD (Channel Activity Detection) is not supported yet, if you want
to implement it, it would be fantastic, if you don't know how, you can open an issue asking for implementation
in the following repo: https://github.com/sandeepmistry/arduino-LoRa

Another todo: DOWNLINKS that arrive too late are emitted anyways,
TTN sometimes takes too long to send a downlink resulting in many TOO_LATE errors.
See protocol: https://github.com/Lora-net/packet_forwarder/blob/master/PROTOCOL.TXT
If you want to change this behaviour, you can find it in
```
libraries/WAN/WAN.cpp
```
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


