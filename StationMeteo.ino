/*
  Station météo V2
  Version:  2.00.01
  Date:     10/02/2020 - 13/02/2020
  Auteur:   Serge CLAUS
  Licence:  GPL V3
*/

//-----------------------------------------------
// Librairies
//-----------------------------------------------
#include <WiFi.h>
#include <WiFiMulti.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
// Timers
#include <SimpleTimer.h>
// Client http
#include <HTTPClient.h>

#include <Wire.h>
#include <SPI.h>
#include <Adafruit_Sensor.h>
// SQM
#include <FreqMeasure.h>
#include <Math.h>
// BME280
#include <Adafruit_BME280.h>
//Serveur Web
#include <WebServer.h>
// Json
#include <ArduinoJson.h>
//MLX90614
#include <Adafruit_MLX90614.h>
// Mod1016
#include <AS3935.h>
// DHT22
#include "DHT.h"
// BH1750 Luminosité
#include <BH1750.h>
//SI1145
#include "Adafruit_SI1145.h"
//MLX90614
#include <Adafruit_MLX90614.h>

// Fichiers externes
#include "StationMeteo.h"

//-----------------------------------------------
// Macros
//-----------------------------------------------
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

// Capteur de pluie
#define PinPluie 3	// TODO
// DHT22
#define DHTPIN 2	// PIN DHT22	TODO
#define DHTTYPE DHT22	// Type DTH (11/22)
// Luminosité
#define ID_LIGHT 10	// Luminosité en Lux
// Index UV
#define ID_UV 11	// Indice UV
#define RP_UV 1.3	// Multiplicateur index UV (du au cache dépoli)
#define ID_IR 12
#define ID_VI 13
// MOD-1016
#define ID_ORAGED 3	// Distance de l'éclair
#define ID_ORAGEI 4	// Intensité de l'éclair
#define ID_ORAGEC 5	// Interrupteur compteur d'orage (Envoi "On" à chaque éclair)
#define IRQ_ORAGE 18	// PIN IRQ (18 boitier externe, 19 boitier station)
// SQM TODO
#define SQMPin	4	// PIN du capteur SQM
//-----------------------------------------------
// Activation des modules
//-----------------------------------------------
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
// Luminosité
BH1750 lightSensor;
// SI1145
Adafruit_SI1145 uv = Adafruit_SI1145();
// DHT22
DHT dht(DHTPIN, DHTTYPE);
// Capteur de pluie

//-----------------------------------------------
// Variables globales
//-----------------------------------------------
/*
  0:  Gtemp		Température
  1:  Gpress	Pression athmosphérique
  2:  Ghum		Humidité
  3:  Grain		Pluviomètre
  4:  Gpluie	Capteur de pluie (TOR)				TODO
  5:  Gvent		Vent
  6:  Graf		Rafales
  7:  Gnuage	Couverture nuageuse (MLX)
  8:  GTciel	Température du ciel (MLX)			TODO
  9:  Gsqm		Qualité du ciel SQM					TODO
  10:  Guv		Index UV							TODO
  11: Glum		Luminosité							TODO
  12: GdistO	Distance orage						TODO
  13: GintO		Intensité orage						TODO
  14: GdirV		Direction du vent					TODO
  15: GTabri	Température de l'abri	(DHT22)		TODO
  16: GHabri	Humidité de l'abri 		(DHT22)		TODO
*/
float Gtemp, Ghum, Gpress, Grain, Gvent, Graf, Gnuage, GTciel, Gsqm, Guv, Glum, GdistO, GintO, GdirV;
bool Gpluie=false;

// Chaine json
StaticJsonDocument<2048> doc;

// MLX90614
int cloudy;
float Clouds;
float CouvNuages;

//-----------------------------------------------
// Setup
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
//-----------------------------------------------
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
//-----------------------------------------------
  // Serveur Web
  server.on("/status", webStatus);
  server.on("/meteo", webMeteo);
  server.begin();

  // Timers
  timer.setInterval(10000L, infoPluie);   // Lecture du capteur de pluie TOR toutes les 10s
  timer.setInterval(30000L, infoMeteo);	  // Lecture des capteurs toutes les 30s
  timer.setInterval(180000L, envoiHTTP);   // Envoie les infos météo au serveur Domoticz toutes les 3 mn

  // BME280
  bme.begin(0x76);
  //MLX (Ciel) 
  mlx.begin();
  // DHT22
  dht.begin();
  //BH1750
  lightSensor.begin();
  //SI1145
  uv.begin();
  // Pluie
  pinMode(PinPluie,INPUT);
  //attachInterrupt(digitalPinToInterrupt(PinPluie),pluie,FALLING);
  // MOD61016
  Wire.begin();
  mod1016.init(IRQ_ORAGE);
  delay(2);
  autoTuneCaps(IRQ_ORAGE);
  delay(2);
  //mod1016.setTuneCaps(6);
  //delay(2);
  mod1016.setOutdoors();
  delay(2);
  mod1016.setNoiseFloor(4); 	// Valeur par defaut 5
  pinMode(IRQ_ORAGE, INPUT);
  attachInterrupt(digitalPinToInterrupt(IRQ_ORAGE), orage, RISING);
  mod1016.getIRQ();
  // Envoi des premières mesures
  infoMeteo();
  envoiHTTP();
}

//-----------------------------------------------
// Boucle principale
//-----------------------------------------------
void loop() {
  // OTA
  ArduinoOTA.handle();
  // Timers
  timer.run();
  // Serveur Web
  server.handleClient();
    // MOD1016
  if (detected) {
    translateIRQ(mod1016.getIRQ());
    detected = false;
  }
  // Capteur de pluie TODO
  
}
