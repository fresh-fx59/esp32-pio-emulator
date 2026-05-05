# 05-pwm-fade — LEDC PWM sweep

LED on pin 5 fades 0 → 255 → 0 using LEDC channel 0 at 1kHz. Tests assert
duty cycle progresses correctly across virtual time.

```bash
pio test -e native
```
