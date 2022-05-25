# cryptoTickerLedMatrix
Display current Crypto Prices on a 32 * 64 Pixel Matrix

## Hardware


## Software

### certificates
You will need to upload a File wit ca certificates to the Spiff of the nodemcu. You can generate them with this script: https://github.com/esp8266/Arduino/blob/master/libraries/ESP8266WiFi/examples/BearSSL_CertStore/certs-from-mozilla.py  or use the certs.ar file in the tools directory for convenience.

To upload the certs.ar file to the nodemcu you can use this tool:
https://github.com/esp8266/arduino-esp8266fs-plugin

### Wifimanager
This project uses https://github.com/tzapu/WiFiManager  Wifimanager. It will generate an AP whith the SSID "cryptoLED" and the password "crpassword". Connect to it and setup your own configuration.
