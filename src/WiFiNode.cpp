/*
 *  This sketch demonstrates how to scan WiFi networks.
 *  The API is almost the same as with the WiFi Shield library,
 *  the most obvious difference being the different file you need to include:
 */

#include "config.h"

#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Arduino.h>
//#include <Fonts/FreeSans24pt7b.h>
#include <Fonts/FreeSans18pt7b.h>


//111.110
//mosquitto_sub -v -u 'user' -P 'test' -t '#'
//mosquitto_pub  -u 'user' -P 'test' -t "hornany/water1/led" -m "{'led':1}" -r
//mosquitto_pub  -u 'user' -P 'test' -t "hornany/water1/led" -m "{'led':0}" -r

/* topics */
#define LEVEL_TOPIC    "hornany/water1/level"
#define LED_TOPIC     "hornany/water1/led" /* 1=on, 0=off */
#define US_TRIG   19
#define US_ECHO   18
#define PIN_LED 2

ESP32Config_t esp32Config;

/* create an instance of WiFiClientSecure */
WiFiClientSecure espClient;
PubSubClient client(espClient);
Adafruit_SSD1306 display(128, 64, &Wire, -1);

char msg[200];
int counter = 0;
int lastMsg = 0;


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
    digitalWrite(PIN_LED, HIGH); 
  } else {
    /* we got '0' -> on */
    digitalWrite(PIN_LED, LOW);
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
    pinMode(PIN_LED, OUTPUT);
    pinMode(US_TRIG,OUTPUT);
    pinMode(US_ECHO,INPUT);




      /* configure the MQTT server with IPaddress and port */
  client.setServer(esp32Config.mqtt, esp32Config.mqtt_port);
  /* this receivedCallback function will be invoked 
  when client received subscribed topic */
  client.setCallback(receivedCallback);

  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  // Clear the buffer
  display.clearDisplay();
  
  
  //display.setFont(&FreeSans24pt7b);
  display.setFont(&FreeSans18pt7b);
  display.setCursor(0, 60);
  display.setTextColor(SSD1306_WHITE);

}

long duration;
int distance;
int last;
//distance = (echo_high_time_in_µs / 1000000.0) * 17015
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
  if (now - lastMsg > 1000) {
      lastMsg = now;
      // Clear the trigPin by setting it LOW:
      digitalWrite(US_TRIG, LOW);
      
      delayMicroseconds(5);
    // Trigger the sensor by setting the trigPin high for 10 microseconds:
      digitalWrite(US_TRIG, HIGH);
      delayMicroseconds(10);
      digitalWrite(US_TRIG, LOW);
      
      // Read the echoPin. pulseIn() returns the duration (length of the pulse) in microseconds:
      duration = pulseIn(US_ECHO, HIGH);
      
      // Calculate the distance:
      distance = duration*0.034/2;
      display.clearDisplay();
      display.setCursor(0, 60);
      display.print(distance+2);
      display.print(" cm");
      display.display();
      counter++;
      if (last == distance && counter % 300 == 0){
        StaticJsonDocument<200> doc;
        doc["level"] = distance;
        size_t n = serializeJson(doc, msg);
        client.publish(LEVEL_TOPIC, msg,true);
      }
      last = distance;
  }
}