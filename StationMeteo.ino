#include "StationMeteo.h"

#include <SimpleTimer.h>
SimpleTimer timer;

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

//Clear sky corrected temperature (temp below means 0% clouds)
#define CLOUD_TEMP_CLEAR  -8
//Totally cover sky corrected temperature (temp above means 100% clouds)
#define CLOUD_TEMP_OVERCAST  0
//Activation treshold for cloudFlag (%)
#define CLOUD_FLAG_PERCENT  30

float P, HR, IR, T, Tp, Thr, Tir, Dew, Light, brightness, lux, mag_arcsec2, Clouds, skyT;
int cloudy, dewing, frezzing;

float UVindex, ir;
int luminosite;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  // MLX
  mlx.begin();
  // BME280
  bme.begin(0x76);
  // Timers
  timer.setInterval(60000L, infoMeteo);	  // Lecture des capteurs toutes les 30s
}

void loop() {
  // Maj 
  timer.run();
  // MLX
  Tir = mlx.readAmbientTempC();
  IR = mlx.readObjectTempC();
  Clouds = cloudIndex();
  skyT = skyTemp();
  if (Clouds > CLOUD_FLAG_PERCENT) {
    cloudy = 1;
  } else {
    cloudy = 0;
  }

  // BME280
  Tp = bme.readTemperature();
  P = bme.readPressure();
  HR = bme.readHumidity();
  Dew = dewPoint(Tp, HR);
    if (Tp <= Dew + 2) {
      dewing = 1;
    } else {
      dewing = 0;
    }
  
  // Luminosité
  luminosite=lightSensor.readLightLevel();

  // Index UV
  ir = uv.readIR();   
  UVindex = uv.readUV()*RP_UV;   
  UVindex /= 100.0;

  // T° sol
  // 10cm
  // 1m

  // H% sol

  // Envoi des données:
  Serial.println("Tciel="+String(skyT));
  Serial.println("CouvN="+String(Clouds));
  Serial.println("Text="+String(Tp));
  Serial.println("Hext="+String(HR));
  Serial.println("Pres="+String(P/100));
  Serial.println("IR="+String(ir));
  Serial.println("Dew="+String(Dew));
  Serial.println("UV="+String(UVindex));
  Serial.println("Lux="+String(luminosite));
  //Serial.println("SQM=20.3");
  //Serial.println("Vent=15");
  delay(5000);
}

// dewPoint function NOAA
// reference: http://wahiduddin.net/calc/density_algorithms.htm
double dewPoint(double celsius, double humidity)
{
  double A0 = 373.15 / (273.15 + celsius);
  double SUM = -7.90298 * (A0 - 1);
  SUM += 5.02808 * log10(A0);
  SUM += -1.3816e-7 * (pow(10, (11.344 * (1 - 1 / A0))) - 1);
  SUM += 8.1328e-3 * (pow(10, (-3.49149 * (A0 - 1))) - 1);
  SUM += log10(1013.246);
  double VP = pow(10, SUM - 3) * humidity;
  double T = log(VP / 0.61078); // temp var
  return (241.88 * T) / (17.558 - T);
}

double skyTemp() {
  //Constant defined above
  double Td = (K1 / 100.) * (T - K2 / 10) + (K3 / 100.) * pow((exp (K4 / 1000.* T)) , (K5 / 100.));
  double Tsky = IR - Td;
  return Tsky;
}

double cloudIndex() {
  double Tcloudy = CLOUD_TEMP_OVERCAST, Tclear = CLOUD_TEMP_CLEAR;
  double Tsky = skyTemp();
  double Index;
  if (Tsky < Tclear) Tsky = Tclear;
  if (Tsky > Tcloudy) Tsky = Tcloudy;
  Index = (Tsky - Tclear) * 100 / (Tcloudy - Tclear);
  return Index;
}
