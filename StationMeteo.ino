/*
	Station météo ESP8266
	Serge CLAUS
	GPL V3
	Version 3.0
	12/03/2020

*/

//----------------------------------------
#include "StationMeteo.h"

#include <SimpleTimer.h>
SimpleTimer timer;

//OTA
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

// Serveur Web
#include <WiFiClient.h>
#include <ESP8266WebServer.h>

// Client Web
#include <ESP8266HTTPClient.h>
HTTPClient http;
WiFiClient client;

// Serveur Telnet (SQM)
WiFiServer wifiServer(2121);
WiFiClient client2;
#include "WiFiP.h"

const char* ssid = STASSID;
const char* password = STAPSK;

#ifdef CTCIEL
// MLX
#define  K1 33.0
#define  K2 0.0
#define  K3 4.0
#define  K4 100.0
#define  K5 100.0
#include "Adafruit_MLX90614.h"
Adafruit_MLX90614 mlx = Adafruit_MLX90614();
#endif

// BME280
#include "Adafruit_BME280.h"
Adafruit_BME280 bme;

#ifdef CLUM
// BH1750 Luminosité
#include <BH1750.h>
BH1750 lightSensor;
#endif

#ifdef CUV
// SI1145 UV
#include "Adafruit_SI1145.h"
Adafruit_SI1145 uv = Adafruit_SI1145();
#endif

#if defined CORAGE || defined CCLOT
// Mod1016
#include <AS3935.h>
#endif


#ifdef CTSOL
// 1wire
#include <DallasTemperature.h>
#include <OneWire.h>
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
#endif

#ifdef CHUMSOL
// Humidité Sol
#define PinHumSol A0    // PIN capteur d'humidité
#endif


#ifdef CSQM
// SQM
//#include "Adafruit_TSL2591.h"
#include "SQM_TSL2591.h"	// https://github.com/gshau/SQM_TSL2591
SQM_TSL2591 sqm = SQM_TSL2591(2591);
#endif

// Serveur Web
ESP8266WebServer server ( 80 );

//Clear sky corrected temperature (temp below means 0% clouds)
#define CLOUD_TEMP_CLEAR  -8
//Totally cover sky corrected temperature (temp above means 100% clouds)
#define CLOUD_TEMP_OVERCAST  0
//Activation treshold for cloudFlag (%)
#define CLOUD_FLAG_PERCENT  30

//----------------------------------------

// Valeurs météo
// BME-280
float P, HR, T, Tp, Dew, dewing;

//float P, HR, IR, T, Tp, Thr, Tir, Dew, Light, brightness, lux, mag_arcsec2, Clouds, skyT, Rain;
#ifdef CTSOL
float tsol10, tsol100;
#endif
#ifdef CHUMSOL
float humsol;
#endif
#ifdef CTCIEL
double skyT, Clouds, MLXambient, MLXsky;
int cloudy; //, dewing, frezzing;
#endif
#if defined CPLUIE || defined CPLUV || defined RRAIN
bool Rain = false;
bool LastRain = !Rain;
#endif
#ifdef CSQM
float mag_arcsec2;
#endif
#ifdef CLUM
float lux;
int luminosite;
#endif
#ifdef CUV
float UVindex, ir;
#endif

#ifdef RRAIN
int rainRate = 0;
#endif

#ifdef RVENT
String Wind, Gust;
#endif

#if defined CORAGE || defined CCLOT
// MOD1016
bool detected = false;
#endif

#if defined CCLOT
unsigned long lastImp;
int nbImpact = 50;
bool alerteClot = false;
#endif

// Divers
int Delai5mn = 0;		// Timer 5mn

//----------------------------------------

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  // OTA
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  IPAddress ip(192, 168, 0, 19);
  IPAddress subnet(255, 255, 255, 0);
  IPAddress dns(192, 168, 0, 1);
  IPAddress gateway(192, 168, 0, 1);
  WiFi.config(ip, gateway, subnet, dns);
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    //Serial.println("Connection Failed...");
    delay(5000);
  }
  ArduinoOTA.setHostname("stationmeteo");
  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_FS
      type = "filesystem";
    }

    // NOTE: if updating FS this would be the place to unmount FS using FS.end()
    //Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    //Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    //Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    //Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      //Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      //Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      //Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      //Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      //Serial.println("End Failed");
    }
  });
  ArduinoOTA.begin();
  //Serial.println("Ready");
  //Serial.print("IP address: ");
  //Serial.println(WiFi.localIP());

  // Résistances de rappel I2c
  pinMode(D1, INPUT_PULLUP);
  pinMode(D2, INPUT_PULLUP);

#ifdef CTCIEL
  // MLX
  //mlx.begin();
#endif
  // BME280
  bme.begin(0x76);
  // Timers
  timer.setInterval(60000L, infoMeteo);	  // Mise à jour des données barométriques et envoi des infos à Domoticz
#ifdef CLUM
  //BH1750
  lightSensor.begin();
#endif
#ifdef CUV
  //SI1145
  uv.begin();
#endif
#if defined CORAGE || defined CCLOT
  // MOD-1016
  Wire.begin();
  mod1016.init(IRQ_ORAGE);
  delay(2);
  autoTuneCaps(IRQ_ORAGE);
  delay(2);
  //mod1016.setTuneCaps(6);
  //delay(2);
  mod1016.setOutdoors();
  delay(2);
  mod1016.setNoiseFloor(5);     // Valeur par defaut 5
  mod1016.writeRegister(0x02, 0x30, 0x03 << 4);
  delay(200);
  pinMode(IRQ_ORAGE, INPUT);
  attachInterrupt(digitalPinToInterrupt(IRQ_ORAGE), orage, RISING);
  mod1016.getIRQ();
#endif

#ifdef CCLOT
  lastImp = millis();
#endif


#ifdef CPLUIE
  // Pluie (capteur)
  pinMode(PinPluie, INPUT);
#endif

#ifdef CSQM
  // SQM
  sqm.begin();
  sqm.config.gain = TSL2591_GAIN_LOW;
  sqm.config.time = TSL2591_INTEGRATIONTIME_200MS;
  sqm.configSensor();
  //sqm.showConfig();
  sqm.setCalibrationOffset(0.0);
#endif

  // Serveur telnet
  wifiServer.begin();

  // Serveur Web
  server.begin();
  server.on ( "/watch", watchInfo );
  server.on ("/temp", sendTemperature);
  // Lecture des infos des capteurs initiale
  infoMeteo();
}

//----------------------------------------

void loop() {
  // OTA
  ArduinoOTA.handle();
  // Serveur Web
  server.handleClient();
  // Maj
  timer.run();
#ifdef CPLUIE
  // Pluie
  Rain = digitalRead(PinPluie);
#if definded CPLUV || defined RRAIN
  if (Rain == false && rainRate > 0) Rain = true;
#endif
#endif

#if defined CORAGE || defined CCLOT
  // MOD1016
  if (detected) {
    translateIRQ(mod1016.getIRQ());
    detected = false;
  }
#endif
#ifdef CTCIEL
  if (Serial.available()) {
    String TCint = Serial.readStringUntil(':');
    MLXsky = Serial.readStringUntil('\n').toDouble();
    if (MLXsky < -60 || MLXsky > 40) {
      MLXsky = 10;
    }

    //MLXambient = Tp;
    MLXambient = TCint.toDouble();
    if (MLXambient != 0 || MLXsky != 0) {
      if (MLXambient == 0) MLXambient = 0.01;
      Clouds = cloudIndex();
      skyT = skyTemp();
      if (Clouds > CLOUD_FLAG_PERCENT) {
        cloudy = 1;
      } else {
        cloudy = 0;
      }
    }
  }
#endif

  // Telnet
  if (wifiServer.hasClient()) {
    if (!client2 || !client2.connected()) {
      if (client2) client2.stop();
      client2 = wifiServer.available();
    } else {
      wifiServer.available().stop();
    }
  }
  if (client2.available() > 0) {
    //while (client.connected()) {
    //  while (client.available()>0) {
    char c = client2.read();
    switch (c) {
      case 'r':
        sqmsendreadrequest(client2);
        break;
      case 'i':
        sqmsendinforequest(client2);
        break;
      case 'c':
        sqmsendcalibrationrequest(client2);
        break;
      case 't':
        skytempreadrequest(client2);
        break;
      case 'n':
        skycouvrequest(client2);
        break;
    }
  }
  //client.stop();
}


#if defined CCLOT
void watchdogCloture() {
  nbImpact++;
  /*  Serial.print(millis() - lastImp);
    Serial.print(" nb: ");
    Serial.println(nbImpact);
  */
  lastImp = millis();
}
#endif

#if defined CORAGE || defined CCLOT
ICACHE_RAM_ATTR void orage() {
  detected = true;
}
#endif

#ifdef CCLOT
void translateIRQ(uns8 irq) {
  watchdogCloture();
}
#endif

#ifdef CORAGE
void translateIRQ(uns8 irq) {
  switch (irq) {
    case 1:
      //Serial.println("NOISE DETECTED");
      break;
    case 4:
      //Serial.println("DISTURBER DETECTED");
      break;
    case 8:
      //Serial.println("LIGHTNING DETECTED");
      sendOrage();
      break;
  }
}
#endif

#if defined CCLOT
// Watchdog cloture électrique
#endif
