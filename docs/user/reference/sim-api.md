# `esp32sim::Sim::*` API reference

The user-facing test API exported by `<esp32sim_unity/esp32sim.h>`. Every method is `static`
on the `esp32sim::Sim` class.

## Lifecycle

```cpp
static void reset();
```
Resets every piece of simulator state: virtual clock to 0, event log empty, every pin LOW
+ INPUT mode + no listeners, every UART buffer empty. **Call this in your test's `setUp()`.**
Does NOT reset your sketch's global variables — that's still your responsibility.

```cpp
static void runSetup();
```
Calls the user's `setup()` function once. Equivalent to one boot cycle.

```cpp
static void runLoop(int n = 1);
```
Calls the user's `loop()` function `n` times. No virtual time passes between iterations
unless the sketch's `loop()` itself calls `delay()`.

```cpp
static bool runUntil(std::function<bool()> predicate, uint64_t timeoutMs);
```
Repeatedly: call `loop()`, advance 1 ms of virtual time, check predicate. Returns `true` if
predicate became true within `timeoutMs`, `false` if it timed out.

## Time

```cpp
static void advanceMs(uint64_t ms);
static void advanceUs(uint64_t us);
static uint64_t nowMs();
static uint64_t nowUs();
```
Read or advance virtual time. Nothing in the simulator ever sleeps; `advanceMs(500)` is
instantaneous wall-clock but moves the virtual clock forward by 500 ms, firing any
scheduled callbacks (timers, alarms) along the way.

## GPIO

```cpp
auto pin = esp32sim::Sim::gpio(2);
int  pin.level();              // read pin's current digital level
PinMode pin.mode();             // read pin's current mode (INPUT/OUTPUT/etc.)
void pin.setLevel(int v);       // simulate an external driver pulling the pin
void pin.pulse(int level, uint64_t ms);  // brief pulse: setLevel(level), advance ms, setLevel(prev)
```

`setLevel` overrides any internal pull-up / pull-down — matches real hardware where an
external driver wins.

## UART (Serial)

```cpp
auto uart = esp32sim::Sim::uart(0);  // 0 = Serial, 1 = Serial1, 2 = Serial2
std::string uart.drainTx();          // returns and clears un-drained TX bytes
std::string uart.txAll();            // entire TX history (since last reset)
bool uart.txContains(std::string);   // substring match against tx history
void uart.inject(std::string);       // add bytes to RX so the sketch can read them
```

## Event log

```cpp
auto q = esp32sim::Sim::events();
q.kind(esp32sim::EventKind::GPIO_WRITE).pin(2).count();
q.after(timestamp_us).all();
```

Builder for querying recorded simulator events. Available `EventKind`s in T1: `GPIO_WRITE`,
`GPIO_PIN_MODE`, `UART_TX`, `UART_RX`, `INTERRUPT_FIRED`. T2 will widen this list.

## See also

- [Supported Arduino APIs](supported-arduino-apis.md) — what works in T1.
- [Why virtual time?](../explanation/why-virtual-time.md) — the design rationale.
