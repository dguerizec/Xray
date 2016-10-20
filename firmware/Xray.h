#ifndef __XRAY_H__
#define __XRAY_H__

#include "application.h"
#include "MQTT/MQTT.h"
#include <map>
#include <vector>
#include <functional>

#define EVERY(T, that)                              \
do {                                                \
    uint32_t interval = (uint32_t)T;                \
    static uint32_t last = millis() - (interval);   \
    uint32_t now = millis();                        \
    if(now - last > (interval)) {                   \
        do { that; } while(0);                           \
        last = now;                                 \
    }                                               \
} while(0)


#define Xray_setup3(name, host, port) XrayApp __xray__ = XrayApp(name, host, port); XrayApp &Xray = __xray__; void user_setup()

#define Xray_setup2(name, host) XrayApp __xray__ = XrayApp(name, host, 1883); XrayApp &Xray = __xray__; void user_setup()

#define GET_MACRO(_0, _1, _2, _3, NAME, ...) NAME
#define Xray_setup(...) GET_MACRO(_0, ##__VA_ARGS__, Xray_setup3, Xray_setup2)(__VA_ARGS__)

class XrayApp;

extern XrayApp &Xray;

void debug(String topic, String msg);

// used to automatically convert a String to int and double
class XString : public String {
    public:
        XString(const String s) : String(s) {};
        XString(const char *s) : String(s) {};

        operator double() const { return toFloat(); };
        operator long() const { return toInt(); };

};

struct XrayPeriodic {
    using fun_periodic = std::function<void()>;

    fun_periodic fun;
    uint32_t period;
    uint32_t last_run;
};

struct XrayOnce {
    using fun_once = std::function<void()>;

    fun_once fun;
    uint32_t timeout;
    uint32_t start;
};

class XrayApp {
    private:
        String my_name;

        using fun_func  = std::function<void(String)>;
        using fun_sub   = std::function<void(String,String)>;
        using fun_get   = std::function<String()>;
        using fun_set   = std::function<String(String)>;

        std::vector<XrayPeriodic>   periodics;
        std::vector<XrayOnce>       once;
        std::map<String,fun_func>   funcs;
        std::map<String,fun_get>    var_get;
        std::map<String,fun_set>    var_set;
        std::map<String,fun_sub>    subs;

        // MQTT transport
        MQTT mqtt;
        String mqtt_topic;
        String mqtt_inbox;
        uint8_t *mqtt_hostname;
        uint16_t mqtt_port;

        void mqtt_setup(uint8_t * host, uint16_t port=1883);
        void mqtt_setup(String host, uint16_t port=1883);
        void mqtt_connect();
        static void mqtt_callback(char *t, uint8_t *p, unsigned int s);
        void mqtt_publish(const String &topic, const String &payload);
        int mqtt_subscribe(String &topic);



    public:
        XrayApp(String name, uint8_t * host, uint16_t port);
        XrayApp(String name, String host, uint16_t port);
        void loop();
        void setup();

        const String &name();
        //String topic(String &t = String(""));

        //template <class T>
        //void function(const String &name, T (*fn)(String args));
        void function(const String &name, std::function<String (String)> fn);
        void function(const String &name, std::function<double (String)> fn);

        template <class T>
        void variable(const String &name, T &var);

        template <class T>
        void publish(const String &topic, T payload);

        void subscribe(const String &topic, void (*fn)(String topic, String payload));

        void every(long ms, std::function<void()> fn);
        void after(long ms, std::function<void()> fn);

        const String time();

        // system events
        std::map<system_event_t, std::vector<std::function<void(int)>>> events_fn;
        bool register_event(system_event_t event, std::function<void(int)> fn);
        void on_button_pressed(std::function<void()> fn);
        void on_button_released(std::function<void(int)> fn);
        void on_button_click(std::function<void()> fn);
        void on_button_clicks(std::function<void(int)> fn);
        void on_reset(std::function<void()> fn);
        void on_reset_pending(std::function<void()> fn);
        void on_firmware_update_pending(std::function<void()> fn);
};

#include "Xray_tpl.h"

#endif // __XRAY_H__

