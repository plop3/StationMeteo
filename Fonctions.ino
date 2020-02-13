// Récupération des données des capteurs
void infoMeteo() {
  // BME280
  Gtemp = round(bme.readTemperature() * 10) / 10.0;
  Gpress = bme.readPressure() / 100.0F;
  Ghum = bme.readHumidity();
#if defined(DEBUG)
  Serial.print("Temperature = ");
  Serial.print(Gtemp);
  Serial.println(" *C");
  Serial.print("Pressure = ");
  Serial.print(Gpress);
  Serial.print(" "); Serial.print(weather[forecast]);
  Serial.println(" hPa");
  Serial.print("Humidity = ");
  Serial.print(Ghum);
  Serial.println(" %");
#endif
	// Pluie (WS2300)
	HTTPClient http;
	http.begin("http://192.168.0.7:8080/json.htm?type=devices&rid=3465");
	int code=http.GET();
	if(code == HTTP_CODE_OK) {
		String payload = http.getString();
		deserializeJson(doc, payload);
		Grain=doc["result"][0]["RainRate"];
	
		http.end();
		// Vent (WS2300)
		http.begin("http://192.168.0.7:8080/json.htm?type=devices&rid=3466");
		code=http.GET();
		if(code == HTTP_CODE_OK) {
			String payload = http.getString();
			deserializeJson(doc, payload);
			Gvent=doc["result"][0]["Speed"];
			if (Gvent==59) Gvent=0;	// Bug de l'anémomètre
			http.end();
			http.begin("http://192.168.0.7:8080/json.htm?type=devices&rid=3466");
			code=http.GET();
			if(code == HTTP_CODE_OK) {
				String payload = http.getString();
				deserializeJson(doc, payload);
				Graf=doc["result"][0]["Gust"];
				if (Graf==59) Graf=0;	// Bug de l'anémomètre
			}
		}
	}
	http.end();
	//MLX (Ciel)
  float tempciel=mlx.readObjectTempC();
  float tempMlx=mlx.readAmbientTempC();
  float stateciel;
  double Tcloudy = CLOUD_TEMP_OVERCAST, Tclear = CLOUD_TEMP_CLEAR;
  //double Td = (K1 / 100.) * (Gtemp - K2 / 10) + (K3 / 100.) * pow((exp (K4 / 1000.* Gtemp)) , (K5 / 100.));
  double Td = (K1 / 100.) * (tempMlx - K2 / 10) + (K3 / 100.) * pow((exp (K4 / 1000.* tempMlx)) , (K5 / 100.));
  tempciel=tempciel-Td;
  //Serial.print("T° ciel: ");Serial.println(tempciel);
  GTciel=tempciel;
  Clouds = cloudIndex(tempciel);
  if (tempciel<Tclear) stateciel=Tclear;
  if (tempciel>Tcloudy) stateciel=Tcloudy; 
  Gnuage=(stateciel-Tclear)*100/(Tcloudy-Tclear);
  //Serial.print("Couverture nuageuse"+String(Gnuage));
  if (Gnuage>CLOUD_FLAG_PERCENT) {
    cloudy=1;
  } else {
    cloudy=0;
  }
  // BH1750
  Glum=lightSensor.readLightLevel();
  // Index UV
  float ir = uv.readIR();
  Guv = uv.readUV()*RP_UV;
  Guv /= 100.0;
  // SQM
  int count=0;
  double sum=0;
  bool reading=true;
  FreqMeasure.begin();
  while (reading) {
	  if (FreqMeasure.available()) {
		  sum = sum + FreqMeasure.read();
		  count++;
		  if (count > 30) {
			double frequency = F_CPU / (sum / count);
			Gsqm = A - 2.5*log10(frequency); //Frequency to magnitudes/arcSecond2 formula
			reading=false;
			FreqMeasure.end();
		  }
	  }
  }
}

//-----------------------------------------------
// Envoi des infos au serveur Domoticz
//-----------------------------------------------
void envoiHTTP() {
  // Baromètre
  HTTPClient http;
  http.begin("http://192.168.0.7:8080/json.htm?type=command&param=udevice&idx=3529&nvalue=0&svalue="+String(Gtemp)+";"+String(Ghum)+";0;"+String(Gpress)+";"+String(forecast));
  http.GET();
  http.end();
  // T° du ciel (soit T° soit couverture nuageuse)
  http.begin("http://192.168.0.7:8080/json.htm?type=command&param=udevice&idx=484&nvalue=0&svalue="+String(GTciel));
  //http.begin("http://192.168.0.7:8080/json.htm?type=command&param=udevice&idx=484&nvalue=0&svalue="+String(Gnuage));
  http.GET();
  http.end();
}

//-----------------------------------------------
// Informations à destination du driver Indi
//-----------------------------------------------
void webStatus() {
  // Envoi les infos météo à Indi
  //TODO Voir pour remplacer la température du ciel par la couverture nuageuse
  //String msg = "pluie="+String(Grain)+"\ntemperature=" + String(Gtemp) + "\nvent="+String(Gvent)+"\nrafale="+String(Graf)+"\nnuages="+String(50-GTciel+Gtemp)+"\n";
  String msg = "pluie="+String(Grain)+"\ntemperature=" + String(Gtemp) + "\nvent="+String(Gvent)+"\nrafale="+String(Graf)+"\nnuages="+String(Gpluie?120,Gnuage)+"\n";
  server.send(200, "text/plain", msg);
  //server.send(200, "application/json", msg);
}

//-----------------------------------------------
// Lecture du capteur de pluie
//-----------------------------------------------
bool infoPluie() {
	Gpluie=!digitalRead(PinPluie);
}
//-----------------------------------------------
// Serveur web fournissant les infos météo
//-----------------------------------------------
void webMeteo() {
  // Envoi les infos météo
  String msg = "Temperature = " + String(Gtemp) + " C\n";
  msg = msg + "Humidite = " + String(Ghum) + "%\n";
  msg = msg + "Pression = " + String(Gpress) + " Hpa " + String(weather[forecast]) + "\n";
  server.send(200, "text/plain", msg + "\n");
}

//-----------------------------------------------
// Calcul de l'index nuageux
//-----------------------------------------------
double cloudIndex(double Tsky) {
  double Tcloudy = CLOUD_TEMP_OVERCAST, Tclear = CLOUD_TEMP_CLEAR;
  //double Tsky = skyTemp();
  double Index;
  if (Tsky < Tclear) Tsky = Tclear;
  if (Tsky > Tcloudy) Tsky = Tcloudy;
  Index = (Tsky - Tclear) * 100 / (Tcloudy - Tclear);
  return Index;
}

//-----------------------------------------------
// Orage
//-----------------------------------------------
void orage() {
  detected = true;
}

//-----------------------------------------------
// Type de détection d'orage
//-----------------------------------------------
void translateIRQ(uns8 irq) {
  switch (irq) {
    case 1:
      Serial.println("NOISE DETECTED");
      break;
    case 4:
      Serial.println("DISTURBER DETECTED");
      break;
    case 8:
      Serial.println("LIGHTNING DETECTED");
      sendOrage();
      break;
  }
}

//-----------------------------------------------
// Envoi des infos d'orage TODO
//-----------------------------------------------
void sendOrage() {
  int distance = mod1016.calculateDistance();
  int intensite = mod1016.getIntensity();
  if (distance == -1)
    Serial.println("Lightning out of range");
  else if (distance == 1)
    Serial.println("Distance not in table");
  else if (distance == 0)
    Serial.println("Lightning overhead");
  else {
    Serial.print("Lightning ~");
    Serial.print(distance);
    Serial.println("km away\n");
    Serial.print("Intensité: ");Serial.println(intensite);
  }
}