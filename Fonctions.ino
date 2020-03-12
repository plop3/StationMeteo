void infoMeteo() {
  // BME280 Mise à jour des prévisions barométriques
  forecast = sample(P);
  // Récupération des infos Domoticz (Vent)
  // TODO
  // Envoi des donées à Domoticz
  // Index UV, luminosité, T°/H£/Pres. , T° ciel, couverture nuageuse

  // Lecture des capteurs
  mesureCapteurs();

  CountBak++;
  // Sauvegarde des données journalière
  if (CountBak>1440) { //1440 minutes=24H
    CountBak=0;
    EEPROM.put(4, CountRain);
  }

  /*
    // Envoi des données:
    Serial.println("Tciel=" + String(skyT));
    Serial.println("CouvN=" + String(Clouds));
    Serial.println("Text=" + String(Tp));
    Serial.println("Hext=" + String(HR));
    Serial.println("Pres=" + String(P / 100));
    //  Serial.println("IR=" + String(ir));
    Serial.println("Dew=" + String(Dew));
    Serial.println("UV=" + String(UVindex));
    Serial.println("Lux=" + String(luminosite));
    //Serial.println("SQM=20.3");
    //Serial.println("Vent=15");
  */
  envoiHTTP();
  	Delai5mn++;
	if (Delai5mn > 5) {
		Delai5mn=0;
		updateRain=true;	// Envoi des données du pluviomètre
	}
}

void mesureCapteurs() {
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
  float HRt = bme.readHumidity();
  if (HRt > 0) HR = HRt;
  Dew = dewPoint(Tp, HR);
  if (Tp <= Dew + 2) {
    dewing = 1;
  } else {
    dewing = 0;
  }

  // Luminosité
  luminosite = lightSensor.readLightLevel();


  // Index UV
  ir = uv.readIR();
  UVindex = uv.readUV() * RP_UV;
  UVindex /= 100.0;

  // T° sol
  // 10cm
  sensors.requestTemperatures();                                                                                                            
  float t=sensors.getTempCByIndex(0);
  if (t>-50 && t < 79) tsol10=t;
  // 1m
  t=sensors.getTempCByIndex(1);
  if (t>-50 && t < 79) tsol100=t;
  
  // H% sol
  humsol=200-constrain(1023-analogRead(PinHumSol)/10.23,0,100)*2;

  // SQM
  sqm.takeReading();
  mag_arcsec2=sqm.mpsas();
  /*
  sqm.full()
  sqm.ir()
  sqm.cis()
  sqm.dmpsas()
  */
}

void envoiHTTP() {
  // Baromètre
    if (http.begin("http://192.168.0.7:8080/json.htm?type=command&param=udevice&idx=3556&nvalue=0&svalue=" + String(Tp) + ";" + String(HR) + ";0;" + String(P / 100) + ";" + String(forecast))) {
    http.GET();
    http.end();
    // T° du ciel, couverture nuageuse
    http.begin("http://192.168.0.7:8080/json.htm?type=command&param=udevice&idx=3557&nvalue=0&svalue=" + String(skyT));
    http.GET();
    http.end();
    http.begin("http://192.168.0.7:8080/json.htm?type=command&param=udevice&idx=3558&nvalue=0&svalue=" + String(Clouds));
    http.GET();
    http.end();
    // Index UV
    http.begin("http://192.168.0.7:8080/json.htm?type=command&param=udevice&idx=3560&nvalue=0&svalue=" + String(UVindex) + ";0");
    http.GET();
    http.end();
    // Luminosité
    http.begin("http://192.168.0.7:8080/json.htm?type=command&param=udevice&idx=3559&nvalue=0&svalue=" + String(luminosite));
    http.GET();
    http.end();
	// T° sol
	http.begin("http://192.168.0.7:8080/json.htm?type=command&param=udevice&idx=3565&nvalue=0&svalue=" + String(tsol10));
    http.GET();
    http.end();
	http.begin("http://192.168.0.7:8080/json.htm?type=command&param=udevice&idx=3563&nvalue=0&svalue=" + String(tsol100));
    http.GET();
    http.end();
	// H% sol
	http.begin("http://192.168.0.7:8080/json.htm?type=command&param=udevice&idx=3566&nvalue=0&svalue=" + String(humsol));
    http.GET();
    http.end();
	// SQM
	http.begin("http://192.168.0.7:8080/json.htm?type=command&param=udevice&idx=3571&nvalue=0&svalue=" + String(mag_arcsec2));
    http.GET();
    http.end();
	// Anémomètre
	http.begin("http://192.168.0.7:8080/json.htm?type=command&param=udevice&idx=3570&nvalue=0&svalue=" + String(Dir)+";"+DirT[DirS]+";"+String(Vent)+";"+String(Gust)+";"+String(Tp)+";24");
    http.GET();
    http.end();

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

void watchInfo() {
  String Page = "Tciel=" + String(skyT) + "\nCouvN=" + String(Clouds) + "\nText=" + String(Tp) + "\nHext=" + String(HR) + "\nPres=" + String(P / 100) + "\nDew=" + String(Dew) + "\n"Pluie"+String(Rain)+"\nlum=" + String(luminosite) + "\nUV=" + String(UVindex) + "\nIR=" + String(ir);
  server.send ( 200, "text/plain", Page);
}

void rainCount() {
	// Incrémente le compteur de pluie
	CountRain+=Plevel;
	updateRain=true;
}

void orage() {
  detected = true;
}
                                                                                                                                            
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

void sendOrage() {
  int distance = mod1016.calculateDistance();
  int intensite = mod1016.getIntensity();
  if (distance == -1)
    //Serial.println("Lightning out of range");
  else if (distance == 1)
    //Serial.println("Distance not in table");
  else if (distance == 0)
    //Serial.println("Lightning overhead");
  else {
    //Serial.print("Lightning ~");
    //Serial.print(distance);
    //Serial.println("km away\n");
    //Serial.print("Intensité: ");Serial.println(intensite);
    send(msgCount.set(1));
	http.begin("http://192.168.0.7:8080/json.htm?type=command&param=udevice&idx=3564&nvalue=0&svalue=" + String(distance));
    http.GET();
    http.end();
	http.begin("http://192.168.0.7:8080/json.htm?type=command&param=udevice&idx=3568&nvalue=0&svalue=1";
    http.GET();
    http.end();
  }                                                                                                                                         
}                              

void isTX20Rising() {
  if (!TX20IncomingData) {
    TX20IncomingData = true;
  }  
}

boolean readTX20() {
    int bitcount=0;
    
    sa=sb=sd=se=0;
    sc=0;sf=0;
    tx20RawDataS = "";

    for (bitcount=41; bitcount>0; bitcount--) {
      pin = (digitalRead(DATAPIN));
      if (!pin) {
        tx20RawDataS += "1";      
      } else {
        tx20RawDataS += "0";      
      }
      if ((bitcount==41-4) || (bitcount==41-8) || (bitcount==41-20)  || (bitcount==41-24)  || (bitcount==41-28)) {
        tx20RawDataS += " ";
      }      
      if (bitcount > 41-5){
        // start, inverted
        sa = (sa<<1)|(pin^1);
      } else
      if (bitcount > 41-5-4){
        // wind dir, inverted
        sb = sb>>1 | ((pin^1)<<3);
      } else
      if (bitcount > 41-5-4-12){
        // windspeed, inverted
        sc = sc>>1 | ((pin^1)<<11);
      } else
      if (bitcount > 41-5-4-12-4){
        // checksum, inverted
        sd = sd>>1 | ((pin^1)<<3);
      } else 
      if (bitcount > 41-5-4-12-4-4){
        // wind dir
        se = se>>1 | (pin<<3);
      } else {
        // windspeed
        sf = sf>>1 | (pin<<11);
      } 
          
      delayMicroseconds(1220);    
    }
    chk= ( sb + (sc&0xf) + ((sc>>4)&0xf) + ((sc>>8)&0xf) );chk&=0xf;
    delayMicroseconds(2000);  // just in case
    TX20IncomingData = false;  

    if (sa==4 && sb==se && sc==sf && sd==chk){      
      return true;
    } else {
      return false;      
    }
}                                                                                                         