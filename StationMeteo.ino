/*
  Station météo V2
  Version:  2.00.00
  Date:     10/02/2020
  Auteur:   Serge CLAUS
  Licence:  GPL V3
*/

//-----------------------------------------------
// Librairies
#include <WiFi.h>
#include <WiFiMulti.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

#include <SimpleTimer.h>

#include <HTTPClient.h>

#include <Wire.h>
#include <SPI.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
//Serveur Web
#include <WebServer.h>
// Json
#include <ArduinoJson.h>
//MLX90614
#include <Adafruit_MLX90614.h>

// Fichiers externes
#include "StationMeteo.h"



//-----------------------------------------------

// Macros

// BME280
#define BME280ADDR  0x76
#define SEALEVELPRESSURE_HPA (1020.5)

// MLX90614
//Formula Tcorrection = (K1 / 100) * (Thr - K2 / 10) + (K3 / 100) * pow((exp (K4 / 1000* Thr)) , (K5 / 100));   
#define  K1 33.   
#define  K2 0.   
#define  K3 4.   
#define  K4 100. 
#define K5 100.
#define CLOUD_TEMP_CLEAR -8
#define CLOUD_TEMP_OVERCAST 0
#define CLOUD_FLAG_PERCENT 30

// Meteo
#define NBC 8    // Nombre de capteurs
// WiFi
WiFiMulti wifiMulti;

//Serveur Web
WebServer server(80);

// Timer
SimpleTimer timer;

// BME 280
Adafruit_BME280 bme; // I2C

// MLX90614
Adafruit_MLX90614 mlx = Adafruit_MLX90614();

//-----------------------------------------------
// Variables globales
/*
  0:  Température
  1:  Pression athmosphérique
  2:  Humidité
  3:  Pluviomètre
  4:  Capteur de pluie
  5:  Vent
  6:  Rafales
  7:  Couverture nuageuse (MLX)
*/
float Gval[NBC];        // Résultats des sondes
//float Gold[NBC] = {99, 0, 0}; // Valeurs précédentes
// Chaine json
StaticJsonDocument<2048> doc;

// MLX90614
int cloudy;
float Clouds;
float CouvNuages;

//-----------------------------------------------
void setup() {
  // Port série
  Serial.begin(SERIALBAUD);
  Serial.println("Booting");

  // WiFi
  wifiMulti.addAP("astro", "pwd");
  wifiMulti.addAP("maison", "pwd");
  while (wifiMulti.run() != WL_CONNECTED) {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }

  // OTA
  ArduinoOTA.setHostname("stationmeteo");
  ArduinoOTA
  .onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH)
      type = "sketch";
    else // U_SPIFFS
      type = "filesystem";

    // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
    Serial.println("Start updating " + type);
  })
  .onEnd([]() {
    Serial.println("\nEnd");
  })
  .onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  })
  .onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // Serveur Web
  server.on("/status", webStatus);
  server.on("/meteo", webMeteo);
  server.begin();

  // Timers
  timer.setInterval(10000L, infoMeteo);   // Lecture des capteurs toutes les 10s
  //timer.setInterval(300000L, infoDomo);  //Tente de se connecter au serveur MQTT toutes les 5mn et envoie les données météo au serveur domotique
  timer.setInterval(60000L, envoiHTTP);   // Envoie les infos météo au serveur Domoticz toutes les 1 mn

  // BME280
  bme.begin(0x76);
  //MLX (Ciel) 
  mlx.begin();
}

//-----------------------------------------------
void loop() {
  // OTA
  ArduinoOTA.handle();
  // Timers
  timer.run();
  // Serveur Web
  server.handleClient();
  // Capteur de pluie
}
