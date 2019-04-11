#include <FS.h>                   //this needs to be first, or it all crashes and burns...

#include <ESP8266WiFi.h>          //ESP8266 Core WiFi Library (you most likely already have this in your sketch)
#include <DNSServer.h>            //Local DNS Server used for redirecting all requests to the configuration portal
#include <ESP8266WebServer.h>     //Local WebServer used to serve the configuration portal
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager WiFi Configuration Magic

#include <ArduinoJson.h>          //https://github.com/bblanchon/ArduinoJson

#include "OSCManager.h"
char osc_remote_IP[40];
char note_config[40];

IPAddress * remoteIPAddressConfig;
OSCManager * myOSCManager;
  int ip[4];


//flag for saving data
bool shouldSaveConfig = false;

//callback notifying us of the need to save config
void saveConfigCallback () {
  Serial.println("Should save config");
  shouldSaveConfig = true;
}

void setup() {

  Serial.begin(115200);


  //read configuration from FS json
  Serial.println("mounting FS...");

  if (SPIFFS.begin()) {
    Serial.println("mounted file system");
    if (SPIFFS.exists("/config.json")) {
      //file exists, reading and loading
      Serial.println("reading config file");
     // SPIFFS.remove("/config.json");
      exit;
      File configFile = SPIFFS.open("/config.json", "r");
      if (configFile) {
        Serial.println("opened config file");
        size_t size = configFile.size();
        // Allocate a buffer to store contents of the file.
        std::unique_ptr<char[]> buf(new char[size]);

        configFile.readBytes(buf.get(), size);
        DynamicJsonBuffer jsonBuffer;
        JsonObject& json = jsonBuffer.parseObject(buf.get());
        json.printTo(Serial);
        if (json.success()) {
          Serial.println("\nparsed json");

          strcpy(osc_remote_IP, json["osc_remote_IP"]);
          strcpy(note_config, json["note_config"]);

        } else {
          Serial.println("failed to load json config");
        }
        configFile.close();
      }
    }
  } else {
    Serial.println("failed to mount FS");
  }
  //end read

  WiFiManager wifiManager;
  //wifiManager.resetSettings();

  //reset saved settings
  //wifiManager.resetSettings();

  //fetches ssid and pass from eeprom and tries to connect
  //if it does not connect it starts an access point with the specified name
  //here  "AutoConnectAP"
  //and goes into a blocking loop awaiting configuration

  // id/name, placeholder/prompt, default, length
  WiFiManagerParameter OSC_remoteaddress("OSC_IP", "OSC remote address", osc_remote_IP, 40);
  WiFiManagerParameter OSC_note("note_config", "note_config", note_config, 40);

  //set config save notify callback
  wifiManager.setSaveConfigCallback(saveConfigCallback);
  wifiManager.addParameter(&OSC_remoteaddress);
  wifiManager.addParameter(&OSC_note);
  //fetches ssid and pass and tries to connect
  //if it does not connect it starts an access point with the specified name
  //here  "AutoConnectAP"
  //and goes into a blocking loop awaiting configuration
  if (!wifiManager.autoConnect("AutoConnectAP", "password")) {
    Serial.println("failed to connect and hit timeout");
    delay(3000);
    //reset and try again, or maybe put it to deep sleep
    ESP.reset();
    delay(5000);
  }

  //if you get here you have connected to the WiFi
  Serial.println("connected...yeey :)");

  //read updated parameters
  strcpy(osc_remote_IP, OSC_remoteaddress.getValue());
  strcpy(note_config, OSC_note.getValue());
  //save the custom parameters to FS
  if (shouldSaveConfig) {
    Serial.println("saving config");
    DynamicJsonBuffer jsonBuffer;
    JsonObject& json = jsonBuffer.createObject();
    json["osc_remote_IP"] = osc_remote_IP;
    json["note_config"] = note_config;
    File configFile = SPIFFS.open("/config.json", "w");
    if (!configFile) {
      Serial.println("failed to open config file for writing");
    }

    json.printTo(Serial);
    json.printTo(configFile);
    configFile.close();
    //end save
  }

  Serial.println("local ip");
  Serial.println(WiFi.localIP());


  // IPAddress remoteIPAddressConfig = IPAddress(String(osc_remote_IP).str());
Serial.println("parsing IP address");
  sscanf(osc_remote_IP, "%d.%d.%d.%d", &ip[0], &ip[1], &ip[2], &ip[3]);
  remoteIPAddressConfig = new IPAddress(ip[0], ip[1], ip[2], ip[3]);
  Serial.println("Launching OSC manager");
  myOSCManager = new OSCManager(remoteIPAddressConfig, 8000, 8888);
  myOSCManager->setup();
}

void loop() {
  // put your main code here, to run repeatedly:
  int value = analogRead(A0);
   if(value > 200)
   {
     Serial.println(value);

     myOSCManager->sendNote(atoi(note_config), 120, 200);
   }


  delay(10);
}
