/*
 *     M      M     AA    TTTTTTT  TTTTTTT  EEEEEEE M      M     AA     NN    N
 *     MM    MM    A  A      T        T     E       MM    MM    A  A    NN    N
 *     M M  M M   A    A     T        T     E       M M  M M   A    A   N  N  N
 *     M  MM  M   AAAAAA     T        T     EEEE    M  MM  M   AAAAAA   N  N  N - AUTOMATION
 *     M      M  A      A    T        T     E       M      M  A      A  N   N N 
 *     M      M  A      A    T        T     E       M      M  A      A  N    NN  
 *     M      M  A      A    T        T     EEEEEEE M      M  A      A  N    NN  
 *     
 *     
 *     Project    : WiFi Analyser for Home Assistant
 *     Version    : 1.0
 *     Date       : 05-2022
 *     Programmer : Ap Matteman
 */    

#include <Wire.h>  
#include "SSD1306.h"
#include <ESP8266WiFi.h>
#include <ESP8266Ping.h>  // Download from: https://github.com/dancol90/ESP8266Ping
#include <PubSubClient.h>
#include "arduino_secrets.h"

int WiFiTry = 0;          // nr of times the WiFi is not available
int Retry = 0;
const char* ssid = YourSSID;
const char* password = YourWiFiPassWord;
const char* HostName = "WatchDog";

const char* mqtt_server = YourMQTTserver;
const char* mqtt_user = YourMQTTuser;
const char* mqtt_password = YourMQTTpassword;

String Message;


SSD1306  display(0x3C, D2, D1); //Address set here 0x3C, D2 (SDA/Serial Data), and D1 (SCK/Serial Clock).
WiFiClient espClient;
PubSubClient client(espClient); // MQTT Client

void setup()   {                
  display.init();

  client.setServer(mqtt_server, 1883);
  
  Serial.begin(115200);
}

void ConnectWiFi() { 
  //Connect with WiFi network
  WiFiTry = 0;
   WiFi.begin(ssid, password);
   // WiFi.setHostname(HostName);
   Serial.println("Connecting to WiFi");
  //Try to connect to WiFi for 11 times
  while (WiFi.status() != WL_CONNECTED && WiFiTry < 11) {

    ++WiFiTry;
    // Serial.print("WiFi status: ");; Serial.println(WiFi.status());
    // Serial.print("WiFiTry: ");; Serial.println(WiFiTry);
    delay(1000);
  }
  Serial.println("");
  Serial.print("WiFiTry: ");; Serial.println(WiFiTry);
  Serial.print("Got IP: ");  Serial.println(WiFi.localIP());
  
  MQTT_Connect();
}

void MQTT_Connect() {
  // Recconnect to the MQTT server

  Serial.println("Attempting Reconnect MQTT connection...");
  // Attempt to connect
  if (client.connect(mqtt_server, mqtt_user, mqtt_password)) 
  {
    Serial.println("MQTT connected");
    Retry = 0;
    client.subscribe("master/night");
  } 
  else 
  {
    // MQTT connection does not work
    ++Retry;
    Serial.println();
    Serial.print("MQTT failed, rc=");
    Serial.println(client.state());
    if (Retry > 15)
    {
      Message += "Geen MQTT \n";
      Serial.println("REBOOTING");
      Retry = 0;
      Serial.println(" Reboot in 2 seconds");
      // Wait 2 seconds before retrying
      delay(2000);
      ESP.restart();
    }
  }  
}

void loop() {
  String myIPAddress;
  String hostName = "www.google.com";
  int pingResult;
  
  Message = "Testing: " +  String(ssid) + "\n";
  
  ConnectWiFi();
  

  
  if (WiFiTry < 11) {
    Serial.print("WiFi network: "); Serial.print(ssid); Serial.println(" is OK");
    Message += "WiFi = OK    RSSI = " + String(WiFi.RSSI()) + "\n";   
    Message += "Got IP: "; 
    Message += WiFi.localIP().toString();
    Message += "\n"; 
    Serial.print("RSSI = ");
    Serial.println(WiFi.RSSI());
    if (!client.connected()) {
      Serial.println("Geen MQTT verbinding");
      // Message += "Geen MQTT \n";
      MQTT_Connect();
    }   
  }
  else { 
     Serial.println(" is NOT OK");
     Message += "WiFi = NOT OK :(";
  }

  IPAddress ip1 (192, 168, 10, 254); // The remote ip to ping
  bool Router1 = Ping.ping(ip1);
  Serial.print("Router1 = "); Serial.println(Router1);
  Message += "R1=" + String(Router1);
  IPAddress ip2 (192, 168, 50, 254); // The remote ip to ping
  bool Router2 = Ping.ping(ip2);
  Serial.print("Router2 = "); Serial.println(Router2);
  Message += "  R2=" + String(Router2);
  bool Google = Ping.ping("www.google.com");
  Serial.print("Google = "); Serial.println(Google);
  Message += "  Google=" + String(Google);
 
  client.publish("Watchdog/Netwerk/Naam", ssid);
  client.publish("Watchdog/Netwerk/IP", WiFi.localIP().toString().c_str() );
  client.publish("Watchdog/Netwerk/Rssi", String(WiFi.RSSI()).c_str());
  client.publish("Watchdog/Netwerk/availability", "online");
  client.publish("Watchdog/Netwerk/ping/Router1", String(Router1).c_str());
  client.publish("Watchdog/Netwerk/ping/Router2", String(Router2).c_str());
  client.publish("Watchdog/Netwerk/ping/Google", String(Google).c_str());



  
  display.clear();
  display.drawString(0, 0, Message);
  display.display();    
  delay(1000);
}
