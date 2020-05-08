# Tesla 1
A simple pulse generator using nothing more than a ESP8266 8-pin module (ESP-02). Designed to work as an interrupter for my Tesla coil...

![scope shot](pics/scope.png)

Uses the I2S peripheral to achieve precise timing. Smallest pulse-width is 1 us. Smaller pulse-widths are possible by increasing the I2S clock rate.

![UI](pics/ui.png)

Has a minimal but very responsive user interface based on web-sockets. The sliders cover 6 octaves on a logarithnmic scale. There's a configurable maximum on-time and duty cycle, which is always observed. 

Putting an ESP on the control board might be an alternative to these hopelessly overpriced fiber-optic transceivers.
You'll get fully isolated control through WIFI.

Will the ESP and its WIFI connection be stable in the tremendous :zap: EMI :zap: of a tesla coil? We'll find out ...

# Building
Needs platform.io installed. Set your WIFI credentials and other parameters in `platform.ini`.

```code
    pio run -t uploadfs -t upload -t monitor
```
