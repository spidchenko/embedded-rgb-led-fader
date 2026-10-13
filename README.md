# RGB LED Fader

A small program that smoothly fades two RGB LEDs (left and right) between random colors on an ARM device.

## How it works

1. Every 2 seconds, a new random target color is picked from the fully saturated HSV palette (random hue, with saturation and value at maximum).
2. The LEDs smoothly transition from their current color toward the target color, updating 60 times per second.
3. Once the next target color is selected, the fade continues toward it, so the transitions never stop.

## Build

### Cross-compile for 32-bit ARM

``` Bash
arm-linux-gnueabihf-gcc -Wall -O3 -pedantic -std=c99 -static main.c -o b2-fader-arm.elf
```

The `-static` flag links everything into the binary, so it runs on the target without extra libraries.
