void infoMeteo() {
  // BME280 Mise à jour des prévisions barométriques
  forecast = sample(P);
  // Récupération des infos Domoticz (Vent)
  // TODO
  // Envoi des donées à Domoticz
  // Index UV, luminosité, T°/H£/Pres. , T° ciel, couverture nuageuse

  // Lecture des capteurs
  mesureCapteurs();
  envoiHTTP();

#ifdef CCLOT
  if (nbImpact < 30) { // Moins de 30 impacts/min
    if (!alerteClot) {
      alerteClot = true;
      sendAlerteClot(true);

    }
  }
  else if (alerteClot) {
    alerteClot = false;
    sendAlerteClot(false);
  }
  nbImpact = 0;
#endif

  Delai5mn++;
  if (Delai5mn > 5) {
    Delai5mn = 0;

#ifdef CCLOT
    //sendAlerteClot(alerteClot);
#endif

  }
}

void mesureCapteurs() {
#ifdef CTCIEL
  // MLX
  /*  MLXambient = mlx.readAmbientTempC();
    MLXsky = mlx.readObjectTempC();
    Clouds = cloudIndex();
    skyT = skyTemp();
    if (Clouds > CLOUD_FLAG_PERCENT) {
      cloudy = 1;
    } else {
      cloudy = 0;
    }
  */
#endif
  // BME280
  Tp = int(bme.readTemperature() * 10) / 10.0;
  P = bme.readPressure();
  float HRt = bme.readHumidity();
  if (HRt > 0) HR = HRt;
  Dew = dewPoint(Tp, HR);
  if (Tp <= Dew + 2) {
    dewing = 1;
  } else {
    dewing = 0;
  }
#ifdef CLUM
  // Luminosité
  luminosite = lightSensor.readLightLevel();
#endif

#ifdef CUV
  // Index UV
  ir = uv.readIR();
  UVindex = uv.readUV() * RP_UV;
  UVindex /= 100.0;
#endif

#ifdef CTSOL
  // T° sol
  // 10cm
  sensors.requestTemperatures();
  float t = sensors.getTempCByIndex(0);
  if (t > -50 && t < 79) tsol10 = t;
  // 1m
  t = sensors.getTempCByIndex(1);
  if (t > -50 && t < 79) tsol100 = t;
#endif

#ifdef CHUMSOL
  // H% sol
  humsol = 200 - constrain(1023 - analogRead(PinHumSol) / 10.23, 0, 100) * 2;
#endif

#ifdef CSQM
  // SQM
  sqm.takeReading();
  mag_arcsec2 = sqm.mpsas;
  if (mag_arcsec2 <0 || mag_arcsec2 > 40) mag_arcsec2=0;
  /*
    sqm.full()
    sqm.ir()
    sqm.cis()
    sqm.dmpsas()
  */
#endif
#ifdef RRAIN
  rainRate = getRain();
#endif

#ifdef RVENT
  Wind = getVent();
  Gust = getRaf();
#endif
}

void envoiHTTP() {

  //Serial.println("Envoi données");
  // Baromètre
  if (http.begin(client, "http://192.168.0.7:8080/json.htm?type=command&param=udevice&idx=3556&nvalue=0&svalue=" + String(Tp) + ";" + String(HR) + ";0;" + String(P / 100) + ";" + String(forecast))) {
    http.GET();
    http.end();
#ifdef CTCIEL
    // T° du ciel, couverture nuageuse
    http.begin(client, "http://192.168.0.7:8080/json.htm?type=command&param=udevice&idx=3557&nvalue=0&svalue=" + String(skyT));
    http.GET();
    http.end();
    http.begin(client, "http://192.168.0.7:8080/json.htm?type=command&param=udevice&idx=3558&nvalue=0&svalue=" + String(Clouds));
    http.GET();
    http.end();
#endif
#ifdef CUV
    // Index UV
    http.begin(client, "http://192.168.0.7:8080/json.htm?type=command&param=udevice&idx=3560&nvalue=0&svalue=" + String(UVindex) + ";0");
    http.GET();
    http.end();
#endif
#ifdef CLUM
    // Luminosité
    http.begin(client, "http://192.168.0.7:8080/json.htm?type=command&param=udevice&idx=3559&nvalue=0&svalue=" + String(luminosite));
    http.GET();
    http.end();
#endif
#ifdef CTSOL
    // T° sol
    http.begin(client, "http://192.168.0.7:8080/json.htm?type=command&param=udevice&idx=3565&nvalue=0&svalue=" + String(tsol10));
    http.GET();
    http.end();
    http.begin(client, "http://192.168.0.7:8080/json.htm?type=command&param=udevice&idx=3563&nvalue=0&svalue=" + String(tsol100));
    http.GET();
    http.end();
#endif
#ifdef CHUMSOL
    // H% sol
    http.begin(client, "http://192.168.0.7:8080/json.htm?type=command&param=udevice&idx=3566&nvalue=0&svalue=" + String(humsol));
    http.GET();
    http.end();
#endif
#ifdef CSQM
    // SQM
    http.begin(client, "http://192.168.0.7:8080/json.htm?type=command&param=udevice&idx=3571&nvalue=0&svalue=" + String(mag_arcsec2));
    http.GET();
    http.end();
#endif
#if defined CPLUIE
    if (Rain != LastRain) {
      LastRain = Rain;
      // Capteur de pluie
      http.begin(client, "http://192.168.0.7:8080/json.htm?type=command&param=switchlight&idx=3572&switchcmd=" + String(Rain ? "On" : "Off"));
      //json.htm?type=command&param=switchlight&idx=99&switchcmd=On
      http.GET();
      http.end();
    }
#endif
  }
}
//-----------------------------------------------

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

#ifdef CTCIEL
double skyTemp(double MLXsky, double MLXambient) {
  //Constant defined above
  double Td = (K1 / 100.) * (MLXambient - K2 / 10.) + (K3 / 100.) * pow((exp (K4 / 1000. * MLXambient)) , (K5 / 100.));
  double Tsky = MLXsky - Td;
  return Tsky;
}

double cloudIndex(double Tsky) {
  double Index;
  if (Tsky < CLOUD_TEMP_CLEAR) Tsky = CLOUD_TEMP_CLEAR;
  if (Tsky > CLOUD_TEMP_OVERCAST) Tsky = CLOUD_TEMP_OVERCAST;
  Index = (Tsky - CLOUD_TEMP_CLEAR) * 100 / (CLOUD_TEMP_OVERCAST - CLOUD_TEMP_CLEAR);
  return Index;
}
#endif

void watchInfo() {
  //Serial.println("Envoi Web");
  String Page = "Text=" + String(Tp) + "\nHext=" + String(HR) + "\nPres=" + String(P / 100) + "\nDew=" + String(Dew) + "\nFore=" + String(forecast);
#ifdef CTCIEL
  Page = Page + "\nTciel=" + String(skyT) + "\nCouvN=" + String(Clouds);
  Page = Page + "\nTir=" + String(MLXsky);

#endif
#if defined CPLUIE || defined RRAIN
  Page = Page + "\nPluie=" + String(Rain);
#endif
#ifdef CLUM
  Page = Page + "\nLum=" + String(luminosite);
#endif
#ifdef CUV
  Page = Page + "\nUV=" + String(UVindex) + "\nIR=" + String(ir);
#endif
#ifdef CSQM
  Page = Page + "\nSQM=" + String(mag_arcsec2);
#endif
#ifdef RVENT
  Page = Page + "\nVent=" + Wind;
  Page = Page + "\nRaf=" + Gust;
#endif
#ifdef CCLOT
  Page = Page + "\nNbimp=" + nbImpact;
#endif

  server.send ( 200, "text/plain", Page);
}

void sendTemperature() {
  server.send (200, "text/plain", String(Tp));
}

#ifdef RVENT
String getVent() {
  String Wind;
  http.begin(client, "http://192.168.0.14/wind");
  if (http.GET() == 200)  {
    Wind = http.getString();
  }
  else Wind = "0";
  http.end();
  return Wind;
}
String getRaf() {
  String Gust;
  http.begin(client, "http://192.168.0.14/gust");
  if (http.GET() == 200)  {
    Gust = http.getString();
  }
  else Gust = "0";
  http.end();
  return Gust;
}
#endif

#ifdef RRAIN
int getRain() {
  http.begin(client, "http://192.168.0.14/rain");
  if (http.GET() == 200) {
    int rainRate = http.getString().toInt();
    //Serial.print("Rain: ");
    //Serial.println(rainRate);
  }
  else rainRate = 0;
  http.end();
  return rainRate;
}
#endif

#ifdef CORAGE
void sendOrage() {
  int distance = mod1016.calculateDistance();
  /* if (distance == -1) {
     //Serial.println("Lightning out of range");
    else if (distance == 1)
     //Serial.println("Distance not in table");
    else if (distance == 0)
     //Serial.println("Lightning overhead");
    else {
  */
  if (distance > 1) {
    //Serial.print("Lightning ~");
    //Serial.print(distance);
    //Serial.println("km away\n");
    //Serial.print("Intensité: ");Serial.println(intensite);
    http.begin(client, "http://192.168.0.7:8080/json.htm?type=command&param=udevice&idx=3564&nvalue=0&svalue=" + String(distance));
    http.GET();
    http.end();
    http.begin(client, "http://192.168.0.7:8080/json.htm?type=command&param=udevice&idx=3568&nvalue=0&svalue=1");
    http.GET();
    http.end();
  }
}
#endif

#ifdef CCLOT
void sendAlerteClot(bool etat) {
  //json.htm?type=command&param=udevice&idx=IDX&nvalue=LEVEL&svalue=TEXT
  http.begin(client, "http://192.168.0.7:8080/json.htm?type=command&param=udevice&idx=3573&nvalue=" + String(etat ? 4 : 1) + "&svalue=" + String(etat ? "Cloture\%20coupée" : "Cloture\%20OK"));
  http.GET();
  http.end();
}
#endif

// MySQMpro
void skytempreadrequest(WiFiClient client) {
  //String rxstring = String(skyT) + ";" + String(Clouds);
  //client.println(rxstring);
}

// TODO Dummy valeurs si pas CTCIEL
void skycouvrequest(WiFiClient client) {
  //client.println(Clouds);
}

#ifdef CSQM
void sqmsendreadrequest(WiFiClient client) {
  char tempStr[14];
  String rxString = "r, ";
  String tmp = "";
  if (mag_arcsec2 < 10.0) {
    rxString = rxString + "0";
  }
  
  rxString = rxString + String(mag_arcsec2, 2) + "m,";
  memset(tempStr, 0, 12);
  tmp = "";
  sprintf( tempStr, "%lu", 20000L );
  int index = strlen(tempStr);
  while ( index < 10)
  {
    tmp = tmp + "0";
    index++;
  }
  rxString = rxString + tmp + tempStr;
  rxString = rxString + "Hz,";
  // period of sensor in counts, period of sensor in seconds,
  rxString = rxString + "0000000020c,0000000.000s, ";
  tmp = String(Tp, 1) + 'C';
  while ( tmp.length() < 6)
  {
    tmp = '0' + tmp;
  }
  rxString = rxString + tmp;
  client.println(rxString);
}

void sqmsendcalibrationrequest(WiFiClient client) {
  String rxString = "c,00000017.60m,0000000.000s, 020.0C,00000008.71m, 020.0C";
  client.println(rxString);
}

void sqmsendinforequest(WiFiClient client) {
  client.println("i,00000002,00000003,00000001,00000413");
}

#endif
