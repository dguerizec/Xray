// Important note: Add MQTT library before compiling this example
//
#include "Xray/Xray.h"

double temperature = 20.;
String topic = "ping";

int set_topic(String args) {
    topic = args;
    // return value will get published on device/beam/function/set_topic/return
    return 42; 
}


Xray_setup("beam", "192.168.1.200") {

    // export/bind variables
    // available on device/beam/inbox/variable/temperature/get and .../set
    Xray.variable("temperature", temperature);
    // available on device/beam/inbox/variable/topic/get and .../set
    Xray.variable("topic", topic);

    // export functions
    // available on device/beam/inbox/function/set_topic
    Xray.function("set_topic", set_topic);

    // using a lambda
    // available on device/beam/inbox/function/set_temperature
    Xray.function("set_temperature", [&](String args) -> double {
        temperature = args.toFloat();
        return temperature;
    });

    
    // subscriptions
    // available on device/beam/inbox/chill_factor
    Xray.subscribe("chill_factor", [](String topic, String payload) {
        temperature *= payload.toFloat();
    });

    // timed events
    Xray.every(5000, []() {
        // publish the temperature every 5 sec
        // will be published on device/beam/temperature
        Xray.publish("temperature", temperature);
    });

    // because blinking a led is absolutely necessary in any project
    pinMode(D7, OUTPUT);
    Xray.every(1000, []() {
        digitalWrite(D7, HIGH);
        Xray.after(500, []() {
            digitalWrite(D7, LOW);
        });
        // and send a keepalive on device/beam/[topic]
        Xray.publish(topic, String(millis()));
    });
}
