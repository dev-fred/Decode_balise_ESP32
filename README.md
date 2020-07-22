# Decode_balise_ESP32

Decodes the beacon frame emitted by a GPS tracker : project https://github.com/khancyr/TTGO_T_BEAM and send result to a serial console:
len=137 ID: ILLEGAL_DRONE_APPELEZ_POLICE17 LAT: 28.78126 LON: -3.03463 ALT ABS: 40 HAUTEUR: 48 LAT DEP: 28.78134 LON DEP: -3.03450 VITESSE HOR: 0 DIR: 1

The code is derivated from https://github.com/f5soh/decode_balise, an ESP8266 version.
The sketch shall be compiled with Arduino IDE configured with the proper ESP32 plugin (https://github.com/espressif/arduino-esp32).
The code is based on https://github.com/ESP-EOS/ESP32-WiFi-Sniffer and  https://github.com/justcallmekoko/ESP32Marauder/search?q=beaconSnifferCallback&unscoped_q=beaconSnifferCallback

## Hardware
<img src="Capture.PNG" width = "300">

# Decode_balise_ESP32_BT

Rate is reduced to 1 message every 4s.

Adds bluetooth output to connect an Android smartphone console with https://play.google.com/store/apps/details?id=de.kai_morich.serial_bluetooth_terminal&hl=en_US

<img src="Screenshot.png" width = "300">

#### Settings -> Terminal 
* Font size : 10 
* Buffer size : Unlimited

#### Setting -> Misc.
* Macro buttons : None
* Keep screen on when connected : ON
