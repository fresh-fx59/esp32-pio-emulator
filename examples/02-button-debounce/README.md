# 02-button-debounce — testing debounce logic

Classic 50ms debouncer reading an INPUT_PULLUP button on pin 4 and
driving an LED on pin 2. Three tests:

1. Clean press (LOW for >50ms) drives LED LOW.
2. Press then release returns LED to HIGH.
3. Sub-window bouncing (LOW/HIGH oscillation for <50ms) does NOT
   trigger the LED to change state.

Run:

    pio test -e native
