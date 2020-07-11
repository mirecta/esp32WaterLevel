/*
 *  This sketch demonstrates how to scan WiFi networks.
 *  The API is almost the same as with the WiFi Shield library,
 *  the most obvious difference being the different file you need to include:
 */

#include "config.h"

#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

//111.110
//mosquitto_sub -v -u 'user' -P 'test' -t '#'
//mosquitto_pub  -u 'user' -P 'test' -t "smarthome/room1/led" -m "{'led':1}" -r
//mosquitto_pub  -u 'user' -P 'test' -t "smarthome/room1/led" -m "{'led':0}" -r

/* topics */
#define COUNTER_TOPIC    "smarthome/room1/counter"
#define LED_TOPIC     "smarthome/room1/led" /* 1=on, 0=off */

ESP32Config_t esp32Config;
int led=2;

/* create an instance of WiFiClientSecure */
WiFiClientSecure espClient;
PubSubClient client(espClient);

long lastMsg = 0;
char msg[200];
int counter = 0;



void receivedCallback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message received: ");
  Serial.println(topic);

  Serial.print("payload: ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
  StaticJsonDocument<256> doc;
  deserializeJson(doc, payload, length);
  /* we got '1' -> on */
  if (doc["led"] == 1){
    digitalWrite(led, HIGH); 
  } else {
    /* we got '0' -> on */
    digitalWrite(led, LOW);
  }

}

void mqttconnect() {
  /* Loop until reconnected */
  while (!client.connected()) {
    Serial.println("MQTT connecting ...");
    Serial.println(esp32Config.mqtt_user );
    Serial.println(esp32Config.mqtt_passwd );

    /* client ID */
    String clientId = "ESP32Client";
    /* connect now */
    if (client.connect(clientId.c_str(),esp32Config.mqtt_user,esp32Config.mqtt_passwd)) {
      Serial.println("connected");
      /* subscribe topic */
      client.subscribe(LED_TOPIC);
    } else {
      Serial.print("failed, status code =");
      Serial.println(client.state());
      Serial.println("try again in 5 seconds");
      /* Wait 5 seconds before retrying */
      delay(5000);
    }
  }
}

void setup()
{
    Serial.begin(115200);
    wifiSetup(esp32Config);
    
    Serial.print("IP address of server: ");
    Serial.println(esp32Config.mqtt);

    Serial.println(WiFi.dnsIP(0).toString());
    pinMode(led, OUTPUT);

      /* configure the MQTT server with IPaddress and port */
  client.setServer(esp32Config.mqtt, esp32Config.mqtt_port);
  /* this receivedCallback function will be invoked 
  when client received subscribed topic */
  client.setCallback(receivedCallback);
}
void loop()
{
 /* if client was disconnected then try to reconnect again */
  if (!client.connected()) {
    mqttconnect();
  }
  /* this function will listen for incomming 
  subscribed topic-process-invoke receivedCallback */
  client.loop();
  /* we increase counter every 3 secs
  we count until 3 secs reached to avoid blocking program if using delay()*/
  long now = millis();
  if (now - lastMsg > 3000) {
    lastMsg = now;
    if (counter < 100) {
      counter++;
      snprintf (msg, 20, "%d", counter);
      StaticJsonDocument<200> doc;
      doc["counter"] = counter;
      size_t n = serializeJson(doc, msg);
      /* publish the message */
      client.publish(COUNTER_TOPIC, msg,true);
    }else {
      counter = 0;  
    }
  }
}