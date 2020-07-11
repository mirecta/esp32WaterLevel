#include <inttypes.h>
#include "Arduino.h"
#include "WiFi.h"
#include "EEPROM.h"


#ifndef _CONFIG_H_
#define _CONFIG_H_
    


    struct ESP32Config_t {
        uint8_t      stored; 
        char        ssid[20];
        char        passwd[20];
        char        mqtt[20];
        uint16_t    mqtt_port;
        char        mqtt_user[20];
        char        mqtt_passwd[20];
    };

    void wifiSetup(ESP32Config_t& esp32Config);






#endif //_CONFIG_H_




