#ifndef __XRAY_TPL_H__
#define __XRAY_TPL_H__

//template <class T>
//void XrayApp::function(const String &name, T (*fn)(String args)) {
//    Xray.funcs[name] = std::move([=](String args) {
//        T retval = fn(args);
//        Xray.publish(String("function/")+name+"/return/", retval);
//    });
//    Xray.mqtt_subscribe(String("function/")+name);
//}

//template <typename T>
//void XrayApp::function(const String &name, std::function<T (String)> fn) {
//    Xray.funcs[name] = std::move([=](String args) {
//        T retval = fn(args);
//        Xray.publish(String("function/")+name+"/return/", retval);
//    });
//    Xray.mqtt_subscribe(String("function/")+name);
//}

template <class T>
void XrayApp::variable(const String &name, T &var) {
    T *varp = &var;
    
    Xray.var_get[name] = std::move([=]() {
        return String(*varp);
    });

    Xray.var_set[name] = std::move([=](XString value) {
        *varp = value;
        return String(*varp);
    });

    Xray.mqtt_subscribe(String("variable/get/")+name);
    Xray.mqtt_subscribe(String("variable/set/")+name);
}

template <class T>
void XrayApp::publish(const String &topic, T payload) {
    Xray.mqtt_publish(topic, String(payload));
};



#endif // __XRAY_TPL_H__
