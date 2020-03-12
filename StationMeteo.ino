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

// EEprom
#include <EEPROM.h>

#ifndef STASSID
#define STASSID "astro"
#define STAPSK  "B546546AF0"
#endif

const char* ssid = STASSID;
const char* password = STAPSK;

// MLX
#define  K1 33.
#define  K2 0.
#define  K3 4.
#define  K4 100.
#define  K5 100.
#include "Adafruit_MLX90614.h"
Adafruit_MLX90614 mlx = Adafruit_MLX90614();

// BME280
#include "Adafruit_BME280.h"
Adafruit_BME280 bme;

// BH1750 Luminosité
#include <BH1750.h>
BH1750 lightSensor;

// SI1145 UV
#include "Adafruit_SI1145.h"
Adafruit_SI1145 uv = Adafruit_SI1145();

// Mod1016
#include <AS3935.h>


// 1wire
#include <DallasTemperature.h>
#include <OneWire.h>
OneWire oneWire(ONE_WIRE_BUS);                                                                                                              
DallasTemperature sensors(&oneWire);                                                                                                        


// Sol
#define PinHumSol A0    // PIN capteur d'humidité

// SQM
#include "SQM_TSL2591.h"	// https://github.com/gshau/SQM_TSL2591
SQM_TSL2591 sqm = SQM_TSL2591(2591);

// Serveur Web
ESP8266WebServer server ( 80 );

// Client Web
HTTPClient http;

// Pluviomètre
pinMode(PINrain, INPUT);
//attachInterrupt(pinrain, RainCount, RISING);


//Clear sky corrected temperature (temp below means 0% clouds)
#define CLOUD_TEMP_CLEAR  -8
//Totally cover sky corrected temperature (temp above means 100% clouds)
#define CLOUD_TEMP_OVERCAST  0
//Activation treshold for cloudFlag (%)
#define CLOUD_FLAG_PERCENT  30

//----------------------------------------

// Valeurs météo
float P, HR, IR, T, Tp, Thr, Tir, Dew, Light, brightness, lux, mag_arcsec2, Clouds, skyT, Rain;
float tsol10, tsol100, humsol;
int cloudy, dewing, frezzing;
 

// Pluie
unsigned int CountRain=0;
int CountBak=0;		// Sauvegarde des données en EEPROM / 24H
volatile bool updateRain=true;
unsigned long PrevTime=0;
unsigned long PrevCount=CountRain;
int RainRate=0;

// TX20 anémomètre
volatile boolean TX20IncomingData = false;
unsigned char chk;
unsigned char sa,sb,sd,se;
unsigned int sc,sf, pin;
String tx20RawDataS = "";
unsigned int Wind, Gust, Dir, DirS;
const char *DirT[]={'N','NNE','NE','ENE','E','ESE','SE','SSE','S','SSW','SW','WSW','W','WNW','NW','NNW'};

// Divers
int Delai5mn=0;		// Timer 5mn

float UVindex, ir;
int luminosite;

//----------------------------------------

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);

  // OTA
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  byte i = 5;
  while ((WiFi.waitForConnectResult() != WL_CONNECTED) && (i > 0)) {
    Serial.println("Connection Failed...");
    delay(5000);
    i--;
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
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("End Failed");
    }
  });
  ArduinoOTA.begin();
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // Lecture des données de l'eeprom
  // L'adresse 0 doit correspondre à 24046 Sinon, initialisation des valeurs
  int Magic;
  EEPROM.get(0, Magic);
  if (Magic != 24046) {
    // Initialisation des valeurs
    Magic=2406;
    EEPROM.put(0, Magic);
    EEPROM.put(4,CountRain);
  }
  else {
    // Sinon, récupération des données
    EEPROM.get(4,CountRain);
	PrevCount=CountRain;
  }
  // MLX
  mlx.begin();
  // BME280
  bme.begin(0x76);
  // Timers
  timer.setInterval(60000L, infoMeteo);	  // Mise à jour des données barométriques et envoi des infos à Domoticz
  //BH1750
  lightSensor.begin();

  //SI1145
  uv.begin();

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
  mod1016.setNoiseFloor(4);     // Valeur par defaut 5
  pinMode(IRQ_ORAGE, INPUT);
  attachInterrupt(digitalPinToInterrupt(IRQ_ORAGE), orage, RISING);
  mod1016.getIRQ();                                                                                                                         
  send(msqDist.set(0));
  
  // Pluie (capteur)
    pinMode(PinPluie,INPUT);
	
  // SQM
  sqm.begin():
  sqm.config.gain = TSL2591_GAIN_LOW;
  sqm.config.time = TSL2591_INTEGRATIONTIME_200MS;
  sqm.configSensor();
  //sqm.showConfig();
  sqm.setCalibrationOffset(0.0);
  
  // TX20 anémomètre
  pinMode(DATAPIN, INPUT);
  attachInterrupt(digitalPinToInterrupt(DATAPIN), isTX20Rising, RISING);
	
  // Serveur Web
  server.begin();
  server.on ( "/watch", watchInfo );
}

//----------------------------------------

void loop() {
  // OTA
  ArduinoOTA.handle();
  // Serveur Web
  server.handleClient();
  // Maj
  timer.run();
  // Pluie
  Rain=digitalRead(PinPluie);
  if (updateRain) {
	  // Envoi des infos à Domoticz
	  // TODO Calcul du rain rate
	  unsigned long currentTime = millis();
	RainRate=360000L*Plevel*(CountRain-PrevCount)/(unsigned long)(currentTime-PrevTime);	//mm*100
	http.begin("http://192.168.0.7:8080/json.htm?type=command&param=udevice&idx=3561&nvalue=0&svalue=" String(RainRate)+";"+ String(CountRain/1000.0));
	http.GET();
	http.end();
	updateRain=false;
	PrevTime=currentTime;
	PrevCount=CountRain;
  }
    // MOD1016
  if (detected) {
    translateIRQ(mod1016.getIRQ());                                                                                                         
    detected = false;
  }        

  // TX20 anémomètre
  if (TX20IncomingData) {
    if (readTX20()) {
		// Data OK
		Wind=sa;
		Gust=sa;
		Dir=sb*22.5;
		DirS=sb;
	}
  }	  
}                                                                                                                                           



