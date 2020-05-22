# Tesla 1
A simple pulse generator using nothing more than a ESP8266 8-pin module (ESP-02). Designed to work as an interrupter for my Tesla coil...

![scope shot](pics/scope.png)

Uses the I2S peripheral to achieve precise timing. Smallest pulse-width is 1 us. Smaller pulse-widths are possible by increasing the I2S clock rate.

![UI](pics/ui.png)

Has a minimal but very responsive user interface based on web-sockets. The sliders cover 6 decades on a logarithnmic scale. There's a configurable maximum on-time and duty cycle, which is always observed. 

Putting an ESP on the control board might be an alternative to these hopelessly overpriced fiber-optic transceivers.
You'll get fully isolated control through WIFI.

Will the ESP and its WIFI connection be stable in the tremendous :zap: EMI :zap: of a tesla coil? We'll find out ...

__... update__: yes, it is stable :ok_hand:. 

I've had it successfully operating in a grounded metal box, directly connected to the [UD2.7 board](http://www.loneoceans.com/labs/ud27/), the box sitting right below the secondary.

Shielding the electronics from the electric field is absolutely mandatory. A grounded cookie box works wonders keeping the electric field out while WIFI still leaks in surprisingly well, which is what we need here :thumbsup:.

# Demo
[Tesla1 60 Hz sync](https://youtu.be/7h7VzaL8ETk)

[Tesla1 dubsteb](https://youtu.be/TvuAkynQ4Mo)

# Building
Needs platform.io installed. Set your WIFI credentials and other parameters in `platform.ini`.

```code
    pio run -t uploadfs -t upload -t monitor
```

# Caveats
  * will output a ~100 ms ON pulse after a reset. So power up this circuit first, then turn up the power on the variac. The turn-on transient is worse when running in `ACTIVE_LOW` mode.

# TODO
    * Add 50 Hz zero crossing synchronization
