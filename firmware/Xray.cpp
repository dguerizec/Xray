#include "Xray.h"


void user_setup() __attribute((weak));
void user_setup() {
}

void setup() __attribute((weak));
void setup() {
    Xray.setup();
}

void loop() __attribute((weak));
void loop() {
    Xray.loop();
}

void debug(String topic, String msg) {
    Xray.publish(topic, msg);
}


XrayApp::XrayApp(String name, uint8_t *host, uint16_t port) : my_name(name) {
    my_name = name; //System.deviceID();
    mqtt_setup(host, port);
}

XrayApp::XrayApp(String name, String host, uint16_t port) : my_name(name) {
    my_name = name; //System.deviceID();
    mqtt_setup(host, port);
}

void XrayApp::mqtt_setup(uint8_t *host, uint16_t port) {
    mqtt = MQTT(mqtt_hostname, port, [](char *topic,uint8_t *cpayload, unsigned int psz){ XrayApp::mqtt_callback(topic, cpayload, psz); });
}

void XrayApp::mqtt_setup(String host, uint16_t port) {
    mqtt = MQTT((char*)host.c_str(), port, [](char *topic,uint8_t *cpayload, unsigned int psz){ XrayApp::mqtt_callback(topic, cpayload, psz); });
}

void XrayApp::mqtt_connect() {
    static bool first_time = true;

    if(!WiFi.ready() || mqtt.isConnected()) {
        return;
    }

    mqtt.connect(my_name);

    if(mqtt.isConnected()) {
        mqtt_topic = "device/";
        mqtt_topic += my_name;
        mqtt_topic += "/"; 
        mqtt_inbox = mqtt_topic + "inbox/";

        // raw subscriptions (other devices, broadcasts, ...)
        for (auto it = subs.cbegin(); it != subs.cend(); ++it) {
            mqtt.subscribe(it->first);
        }

        // functions
        for (auto it = funcs.cbegin(); it != funcs.cend(); ++it) {
            mqtt.subscribe(mqtt_inbox + "function/" + it->first);
        }

        // variable get
        for (auto it = var_get.cbegin(); it != var_get.cend(); ++it) {
            mqtt.subscribe(mqtt_inbox + "variable/get/" + it->first);
        }
        // variable set
        for (auto it = var_set.cbegin(); it != var_set.cend(); ++it) {
            mqtt.subscribe(mqtt_inbox + "variable/set/" + it->first);
        }

        // catch all under inbox
        //mqtt.subscribe((mqtt_inbox + "#").c_str());

        if(first_time) {
            // publish boot time in seconds
            mqtt_publish("boot", String(millis()/1000.));
            first_time = false;
        }
    }
}

void XrayApp::mqtt_callback(char *topic,uint8_t *cpayload, unsigned int psz) {
    String payload = std::string((char *)cpayload, psz).c_str();
    const char *t;
    auto sz = Xray.mqtt_inbox.length();

    if(!strncmp(topic, Xray.mqtt_inbox, sz)) {
        t = topic + sz;
        if(t[0] == '/') {
            t++;
        }
    } else {
        // this is a raw subscription
        t = topic;
        if(Xray.subs.count(t)) {
            Xray.subs.at(t)(t, payload);
        }
        return;
    }

    if(!strncmp(t, "function/", 9)) {
        t += 9;
        if(!t[0]) {
            return;
        }
        if(Xray.funcs.count(t)) {
            Xray.funcs.at(t)(payload);
        }
        return;
    } else if(!strncmp(t, "variable/get/", 13)) {
        t += 13;
        if(!t[0]) {
            return;
        }
        if(Xray.var_get.count(t)) {
            String v = Xray.var_get.at(t)();
            Xray.publish(String("variable/")+t, v);
        }
        return;
    } else if(!strncmp(t, "variable/set/", 13)) {
        t += 13;
        if(!t[0]) {
            return;
        }
        if(Xray.var_set.count(t)) {
            Xray.var_set.at(t)(payload);
            String v = Xray.var_get.at(t)();
            Xray.publish(String("variable/")+t, v);
        }
        return;
    }
}

void XrayApp::mqtt_publish(const String &topic, const String &payload) {
    mqtt_connect();
    mqtt.publish(mqtt_topic + topic, payload);
}

void XrayApp::setup() {
    mqtt_connect();
    user_setup();
}

void XrayApp::loop() {
    if(mqtt.isConnected()) {
        mqtt.loop();
    } else {
        mqtt_connect();
    }
    for(auto &p : periodics) {
        // FIXME: This is inefficient, use sorted list
        auto now = millis();
        if(now - p.last_run > p.period) {
            p.last_run = now;
            p.fun();
        }
    }
    for (unsigned i=0; i<once.size(); i++) {
        // FIXME: This is even more inefficient, use sorted list
        auto now = millis();
        if(now - once[i].start > once[i].timeout) {
            once[i].fun();
            once.erase(once.begin()+i);
        }
    }
}

const String &XrayApp::name() {
    return my_name;
}

void XrayApp::subscribe(const String &topic, void (*fn)(String topic, String payload)) {
    if(!topic.length()) {
        return;
    }
    Xray.subs[topic] = std::move(fn);
    if(Xray.mqtt.isConnected()) {
        //if(topic[0] == '/') {
        //    Xray.mqtt.subscribe(topic.c_str() + 1); // raw topic subscription
        //} else {
            Xray.mqtt.subscribe(topic);
        //}
    }
}

//String XrayApp::topic(String &t) {
//    return String("device/") + my_name + "/" + t;
//}

int XrayApp::mqtt_subscribe(String &topic) {
    if(mqtt.isConnected()) {
        return mqtt.subscribe(String("device/") + my_name + "/inbox/" + topic);
    }
    return 0;
}

void XrayApp::function(const String &name, std::function<String(String)>fn) {
    Xray.funcs[name] = std::move([=](String args) {
        String retval = fn(args);
        Xray.publish(String("function/")+name+"/return/", retval);
    });
    Xray.mqtt_subscribe(String("function/")+name);
}

void XrayApp::function(const String &name, std::function<double(String)>fn) {
    Xray.funcs[name] = std::move([=](String args) {
        double retval = fn(args);
        Xray.publish(String("function/")+name+"/return/", retval);
    });
    Xray.mqtt_subscribe(String("function/")+name);
}


void XrayApp::every(long ms, std::function<void()> fn) {
    XrayPeriodic p;
    p.fun = fn;
    p.period = ms;
    p.last_run = millis();
    Xray.periodics.push_back(p);
}

void XrayApp::after(long ms, std::function<void()> fn) {
    XrayOnce p;
    p.fun = fn;
    p.timeout = ms;
    p.start = millis();
    Xray.once.push_back(p);
}

const String XrayApp::time() {
    return Time.timeStr();
}

void when_event(system_event_t event, int param) {
    auto fnlist = Xray.events_fn.find(event);
    if(fnlist == Xray.events_fn.end()) {
        return;
    }
    for(auto fn : fnlist->second) {
        // avoid blocking with network functions called from fn
        //Xray.after(0, [=]() {
            fn(param);
        //});
    }
}

bool XrayApp::register_event(system_event_t event, std::function<void(int)> fn) {
    events_fn[event].push_back(std::move(fn));
    if(events_fn[event].size() > 1) {
        return true;
    }
    return System.on(event, when_event);
}

void XrayApp::on_button_pressed(std::function<void()> fn) {
    register_event(button_status, [=](int param) {
        if(!param) {
            fn();
        }
    });
}

void XrayApp::on_button_released(std::function<void(int)> fn) {
    register_event(button_status, [=](int param) {
        if(param) {
            fn(param);
        }
    });
}

void XrayApp::on_button_click(std::function<void()> fn) {
    register_event(button_click, [=](int param) {
        fn();
    });
}

void XrayApp::on_button_clicks(std::function<void(int)> fn) {
    register_event(button_final_click, [=](int param) {
        fn(param);
    });
}

void XrayApp::on_reset(std::function<void()> fn) {
    register_event(reset, [=](int param) {
        fn();
    });
}

void XrayApp::on_reset_pending(std::function<void()> fn) {
    register_event(reset_pending, [=](int param) {
        fn();
    });
}

void XrayApp::on_firmware_update_pending(std::function<void()> fn) {
    register_event(firmware_update_pending, [=](int param) {
        fn();
    });
}


