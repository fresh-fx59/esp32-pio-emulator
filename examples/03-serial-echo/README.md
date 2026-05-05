# 03-serial-echo — testing UART read/write

Reads bytes from `Serial`, echoes them back uppercase. Three tests:

1. Inject "hello", expect "HELLO" on TX.
2. Mixed input ("Hi 42!") passes non-lowercase characters through
   unchanged.
3. No input → no output.

Run:

    pio test -e native
