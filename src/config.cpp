#include "config.h"
#include <functional>
#include <WebServer.h>
#include <ESPmDNS.h>

#define PIN_CONFIG 4


#define AP_SSID "IOT82547"
#define AP_PASSWD "heslo1234"

void configIOT(ESP32Config_t& esp32Config);

void handleRoot(ESP32Config_t* esp32Config, WebServer *server){

    String message = "<html>"
    "<head>"
    "<title>IOTmt config</title>"
    "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">"
    "<style>"
    "html body {background-color:#333333;width: 100%;color:#8c8c8c;}"
    "table {margin:auto;}"
    "td {padding:5px;}"
    "button {"
        "background-color: #4CAF50;"
        "border: none;"
        "color: white;"
        "padding: 15px 20px;"
        "text-align: center;"
        "text-decoration: none;"
        "display: inline-block;"
        "font-size: 16px;"
        "margin: 5px;"
        "}"
    "</style>"
    "<script>"
        "function showPasswd(el) {"
            "if (el.type === \"password\") {el.type = \"text\";} else { el.type = \"password\"; }}"
    "</script>"
    "</head>"
    "<body>"
    "<form method='POST'>"
    "<table>"
        "<tr><td>IOTmt settings</td></tr>"
        "<tr>"
            "<td>SSiD</td>"
            "<td>"
                "<input type=\"text\" name=\"ssid\" list=\"ssidList\" value=\"";
    message += esp32Config->ssid;
    message += "\">"
                "<datalist id=\"ssidList\">";
    Serial.println("scanning...");           
    int n = WiFi.scanNetworks();
    for (int i = 0; i < n; ++i) {
        message += "<option value=\"";
        message += WiFi.SSID(i);
        Serial.print("SSID: ");
        Serial.println(WiFi.SSID(i));
        message += "\">";
    }
    message +=  "</datalist>"
            "</td>"
        "</tr>"
        "<tr>"
            "<td>Password</td>"
            "<td>"
                "<input type=\"password\" name=\"pwd\" id=\"pwd\" value=\"";
    //pass
    message += esp32Config->passwd;
    message += "\">"
                "<label><input type=\"checkbox\" onclick=\"showPasswd(document.getElementById('pwd'))\">Show</label>"
            "</td>"
        "</tr>"
        "<tr>"
            "<td>Status</td>"
            "<td>";
    //status
    if(WiFi.status() != WL_CONNECTED){
        message += "disconnected";  
    }else{
        message += "ok ( ";
        message += WiFi.localIP().toString();
        message += " )";
    }
    message += "</td>"
        "</tr>"
        "<tr>"
            "<td>MQTT Server</td>"
            "<td><input type=\"text\" name=\"mqtt\" value=\"";
    //mqtt server
    message += esp32Config->mqtt;
    message +=  "\"></td>"
        "</tr>"
        "<tr>"
            "<td>MQTT Port</td>"
            "<td><input type=\"text\" name=\"mqtt-port\" value=\"";
    //mqtt port
    message += esp32Config->mqtt_port;
    message += "\"></td>"
        "</tr>"
        "<tr>"
            "<td>MQTT User</td>"
            "<td><input type=\"text\" name=\"mqtt-user\" value=\"";
    //mqtt user
    message += esp32Config->mqtt_user;
    message += "\"></td>"
        "</tr>"
        "<tr>"
            "<td>MQTT Pass</td>"
            "<td><input type=\"password\" name=\"mqtt-pwd\" id=\"mqtt-pwd\" value=\"";
    //mqtt passwd
    message += esp32Config->mqtt_passwd;
    message += "\">"
                "<label><input type=\"checkbox\" onclick=\"showPasswd(document.getElementById('mqtt-pwd'))\">Show</label>"
            "</td>"
        "</tr>"
        "<tr>"
            "<td><button formaction=\"reload\">Reload</button></td>"
            "<td>"
            "<button formaction=\"save\">Save</button>"
            "<button formaction=\"done\">Done</button></td>"
        "</tr>"
        "</table>"
        "</form>"
        "</body>"
        "</html>";

    server->sendHeader("Cache-Control", "no-store");
    server->sendHeader("Expires", "0");
    server->send(200, "text/html", message); 
}
void setConfig(ESP32Config_t* esp32Config, WebServer *server, bool save=false, bool changeWifi=false){
    

    for (uint8_t i = 0; i < server->args(); i++) {
        String name = server->argName(i);
        if (name == "ssid"){
           if(server->arg(i) != esp32Config->ssid ) changeWifi = true; 
            server->arg(i).toCharArray(esp32Config->ssid,20);
            continue;
        }
        if (name == "pwd"){
            if(server->arg(i) != esp32Config->passwd ) changeWifi = true;
            server->arg(i).toCharArray(esp32Config->passwd,20);
            continue;
        }
        if (name == "mqtt"){
            server->arg(i).toCharArray(esp32Config->mqtt,20);
            continue;
        }
        if (name == "mqtt-port"){
            esp32Config->mqtt_port = server->arg(i).toInt();
            continue;
        }
        if (name == "mqtt-user"){
            server->arg(i).toCharArray(esp32Config->mqtt_user,20);
            continue;
        }
        if (name == "mqtt-pwd"){
            server->arg(i).toCharArray(esp32Config->mqtt_passwd,20);
            continue;
        }

    }
    if (changeWifi){
        WiFi.begin(esp32Config->ssid,esp32Config->passwd);
        delayMicroseconds(2000000);
    }
    if(save == true){
        EEPROM.put(0,*esp32Config);
        EEPROM.commit();
    }
}

void redir(WebServer  *server){
    server->sendHeader("Location", "/");
    server->sendHeader("Cache-Control", "no-cache");
    server->send(302);
}

void handleReload(ESP32Config_t* esp32Config, WebServer *server){
    setConfig(esp32Config,server,false,true);
    redir(server);
 
}
void handleSave(ESP32Config_t* esp32Config, WebServer *server){
    setConfig(esp32Config,server,true); 
    redir(server);
}
void handleDone(ESP32Config_t* esp32Config, WebServer *server){
    setConfig(esp32Config,server,true);
    ESP.restart();
}

void configIOT(ESP32Config_t& esp32Config){
    Serial.println("Get config start AP");
    WiFi.disconnect();
    WiFi.softAP(AP_SSID,AP_PASSWD);
    delay(1000);
 
    //now start webserver 
    WebServer server(80);
    server.on("/", std::bind(handleRoot,&esp32Config, &server));
    server.on("/reload", std::bind(handleReload,&esp32Config, &server));
    server.on("/save", std::bind(handleSave,&esp32Config, &server));
    server.on("/done", std::bind(handleDone,&esp32Config, &server));
    server.begin();
    while(true){
        server.handleClient();
    }   
}

void wifiSetup(ESP32Config_t& esp32Config){

    if (!EEPROM.begin(sizeof(ESP32Config_t)))
    {
        Serial.println("failed to initialise EEPROM"); delay(1000000);
    }

    EEPROM.get(0,esp32Config);
    if (esp32Config.stored != 0xaa){
        Serial.println("No config in EEPROM");
        esp32Config.stored = 0xaa;
        esp32Config.mqtt[0] = 0x00;
        esp32Config.mqtt_passwd[0] = 0x00;
        esp32Config.mqtt_user[0] = 0x00;
        esp32Config.ssid[0] = 0x00;
        esp32Config.passwd[0] = 0x00;
        configIOT(esp32Config);
    }
#ifdef PIN_CONFIG
    pinMode(PIN_CONFIG, INPUT_PULLUP);
    digitalWrite(PIN_CONFIG,HIGH);

    if (digitalRead(PIN_CONFIG) == LOW){
        Serial.println("Pin shorted !!");
        configIOT(esp32Config);
    }
#endif

    WiFi.begin(esp32Config.ssid,esp32Config.passwd);
    int connCount = 0;
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
        connCount++;
        if(connCount == 20){
            Serial.println("Can't connect to wifi");
            configIOT(esp32Config);
        }
    }
    
    Serial.println(WiFi.localIP());
}


