# How to test an MQTT-publishing sketch

Your sketch uses `PubSubClient` (Nick O'Leary's MQTT library) to publish to a
broker. Test that it publishes to the right topic with the right payload, and
that subscribed-topic callbacks fire.

## Pattern: assert on publishes

```cpp
#include <PubSubClient.h>
#include <esp32sim_unity/esp32sim.h>

void test_publishes_temperature(void) {
    esp32sim::Sim::reset();
    esp32sim::Sim::runSetup();
    esp32sim::Sim::runLoop();

    auto& publishes = esp32sim::Network::instance().mqtt_publishes();
    TEST_ASSERT_EQUAL_size_t(1, publishes.size());
    TEST_ASSERT_EQUAL_STRING("sensors/temp", publishes[0].topic.c_str());
    TEST_ASSERT_EQUAL_STRING("22.5", publishes[0].payload.c_str());
}
```

## Pattern: drive an incoming message to a subscribed topic

```cpp
void test_handles_command(void) {
    esp32sim::Sim::reset();
    esp32sim::Sim::runSetup();  // sketch subscribes to "control/led"

    esp32sim::Network::instance().mqtt_deliver("control/led", "ON");
    esp32sim::Sim::runLoop();   // mqtt.loop() inside delivers via callback

    // Assert sketch reacted, e.g. LED pin is HIGH
    TEST_ASSERT_EQUAL_INT(HIGH, esp32sim::Sim::gpio(2).level());
}
```

## What this catches / doesn't catch

**Catches:** wrong topic strings, wrong payload format, missing publishes,
forgotten subscribes, callback-handling bugs, disconnect-publish-fails logic.

**Doesn't catch:** real broker behavior, QoS handling, retained-message
edge cases on real brokers, TLS-on-MQTT timing. The fake `PubSubClient`
unconditionally accepts publishes when "connected"; real brokers have
backpressure/limits.
