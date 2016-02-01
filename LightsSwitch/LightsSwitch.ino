#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <EEPROM.h>
#include "WiFiConfiguration.h"

#define IS_CONFIGURED_ADDRESS   0
#define SSID_SIZE_ADDRESS       1
#define SSID_START_ADDRESS      2

int SWITCH_STATE = HIGH;
const int led = BUILTIN_LED;
const char* SWITCH_ID = "000016";
const char* APssid = "Switch - 000016";
bool isConfigured = false;
String ssid;
String password;


ESP8266WebServer server(80);
WiFiConfiguration configuration;
InvertedSwitch *sw = new InvertedSwitch(0);
//Switch *sw = new Switch(0);


void switchOn() {
  sw->setOn();
  getState();
}

void switchOff() {
  sw->setOff();
  getState();
}

void getState() {
  String message = "{ \"id\": \"" + String(SWITCH_ID) + "\", \"state\": " + String(sw->getState()) + ", \"ip\": \"" + String(WiFi.localIP()[0]) +"."+ String(WiFi.localIP()[1]) +"."+ String(WiFi.localIP()[2]) +"."+ String(WiFi.localIP()[3])+ "\" }";
  DEBUG_PRINTLN(message + "\0");
  server.send(200, "text/plain", message);
}

void handleConfig() {
 if (server.hasArg("ssid") && server.hasArg("password")) {
    ssid = server.arg("ssid");
    configuration.setSSID(ssid);
      DEBUG_PRINTLN(ssid);
      DEBUG_PRINTLN(ssid.length());
    password = server.arg("password");
    configuration.setPassword(password);
      DEBUG_PRINTLN(password);
      DEBUG_PRINTLN(password.length());

    configuration.save();

    server.send(200, "text/html", "Configured! Restarting in 3 sec...");
    delay(3000); 
    ESP.restart();
 } else {
    server.send(200, "text/html", "<html><head><title>Light Switch Config</title></head><body><form action=\"\"><fieldset>    <legend>Switch Configuration</legend>    <div id=\"ssid\">     <span>SSID</span><input type=\"text\" name=\"ssid\" />    </div>    <div id=\"password\">     <span>Password</span><input type=\"password\" name=\"password\" />    </div>    <span><input type=\"submit\"/><input type=\"reset\"/></span></fieldset></form></body></html>");  
 }
}

int connectToWiFi() {
    int _ssidSize = ssid.length()+1;
    char _ssid[_ssidSize];
    ssid.toCharArray(_ssid, sizeof(_ssid));
    int _passwordSize = password.length()+1;
    char _password[_passwordSize];
    password.toCharArray(_password, sizeof(_password));
    WiFi.begin(_ssid, _password);
    
    int i=0;
    while (WiFi.status() != WL_CONNECTED) {
      if (i == 30) {
        return -1;
      }
      
      delay(500);
      DEBUG_PRINT(".");
      i++;
    }
    
    DEBUG_PRINTLN("");
    DEBUG_PRINTLN("Connected to ");
    DEBUG_PRINTLN(ssid);
    DEBUG_PRINTLN("IP address: ");
    DEBUG_PRINTLN(WiFi.localIP());

    if (MDNS.begin(SWITCH_ID)) {
      DEBUG_PRINTLN("MDNS responder started");
    }

    return 0;
}

void setup(void){
  pinMode(led, OUTPUT);
  pinMode(0, OUTPUT);
  digitalWrite(0, LOW);
  pinMode(2, OUTPUT);
  pinMode(3, INPUT);
  
  Serial.begin(115200,SERIAL_8N1,SERIAL_TX_ONLY);
  
  DEBUG_PRINTLN("Loading config...");
  configuration.load();
  ssid = configuration.getSSID();
  password = configuration.getPassword();
  DEBUG_PRINTLN("Config loaded.");
  
  if (connectToWiFi() == -1) {
    WiFi.softAP(APssid);
    IPAddress myIP = WiFi.softAPIP();
    DEBUG_PRINTLN("IP Address:");
    DEBUG_PRINTLN(myIP);
    isConfigured = false;
  }
  
  server.on("/on", switchOn);
  server.on("/off", switchOff);
  server.on("/state", getState);
  server.on("/config", handleConfig);
  server.onNotFound(getState);
  server.begin();
  DEBUG_PRINTLN("HTTP server started");
}

void loop(void){
  if (isConfigured && WiFi.status() != WL_CONNECTED) {
    connectToWiFi();

    server.on("/on", switchOn);
    server.on("/off", switchOff);
    server.on("/state", getState);
    server.on("/config", handleConfig);
    server.onNotFound(getState);
    server.begin();
    DEBUG_PRINTLN("HTTP server started");
  }

  
  int buttonState = digitalRead(3);
  if (buttonState == LOW) {
    do {
      buttonState = digitalRead(3);
      delay(15);
    } while (buttonState == LOW);

    sw->toggle();
  }

  server.handleClient();
}
