# Tesla 1
A simple pulse generator using a ESP8266 8-pin module. Designed to work as an interrupter for my Tesla coil.

![scope shot](pics/scope.png)

Uses the I2S peripheral to achieve precise timing. Smallest pulse-width is 1 us.

![UI](pics/ui.png)

Has a minimal but very responsive user interface based on web-sockets.

# Building
Needs platform.io installed. Set your WIFI credentials in `platform.ini`.

```code
    pio run -t uploadfs -t upload -t monitor
```
