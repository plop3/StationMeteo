void infoMeteo() {
  // BME280
  Gval[0] = round(bme.readTemperature() * 10) / 10.0;
  Gval[1] = bme.readPressure() / 100.0F;
  Gval[2] = bme.readHumidity();
#if defined(DEBUG)
  Serial.print("Temperature = ");
  Serial.print(Gval[0]);
  Serial.println(" *C");
  Serial.print("Pressure = ");
  Serial.print(Gval[1]);
  Serial.print(" "); Serial.print(weather[forecast]);
  Serial.println(" hPa");
  Serial.print("Humidity = ");
  Serial.print(Gval[2]);
  Serial.println(" %");
#endif
	// Pluie (WS2300)
	HTTPClient http;
	http.begin("http://192.168.0.7:8080/json.htm?type=devices&rid=3465");
	int code=http.GET();
	if(code == HTTP_CODE_OK) {
		String payload = http.getString();
		deserializeJson(doc, payload);
		Gval[3]=doc["result"][0]["RainRate"];
	
		http.end();
		// Vent (WS2300)
		http.begin("http://192.168.0.7:8080/json.htm?type=devices&rid=3466");
		code=http.GET();
		if(code == HTTP_CODE_OK) {
			String payload = http.getString();
			deserializeJson(doc, payload);
			Gval[5]=doc["result"][0]["Speed"];
			http.end();
			http.begin("http://192.168.0.7:8080/json.htm?type=devices&rid=3466");
			code=http.GET();
			if(code == HTTP_CODE_OK) {
				String payload = http.getString();
				deserializeJson(doc, payload);
				Gval[6]=doc["result"][0]["Gust"];
			}
		}
	}
	http.end();
	//MLX (Ciel)
  float tempciel=mlx.readObjectTempC();
  float tempMlx=mlx.readAmbientTempC();
  float stateciel;
  double Tcloudy = CLOUD_TEMP_OVERCAST, Tclear = CLOUD_TEMP_CLEAR;
  //double Td = (K1 / 100.) * (Gval[0] - K2 / 10) + (K3 / 100.) * pow((exp (K4 / 1000.* Gval[0])) , (K5 / 100.));
  double Td = (K1 / 100.) * (tempMlx - K2 / 10) + (K3 / 100.) * pow((exp (K4 / 1000.* tempMlx)) , (K5 / 100.));
  tempciel=tempciel-Td;
  Serial.print("T° ciel: ");Serial.println(tempciel);
  Gval[7]=tempciel;
  Clouds = cloudIndex(tempciel);
  if (tempciel<Tclear) stateciel=Tclear;
  if (tempciel>Tcloudy) stateciel=Tcloudy; 
  CouvNuages=(stateciel-Tclear)*100/(Tcloudy-Tclear);
  Serial.print("Couverture nuageuse"+String(CouvNuages));
  if (CouvNuages>CLOUD_FLAG_PERCENT) {
    cloudy=1;
  } else {
    cloudy=0;
  }
  
}

//-----------------------------------------------

void envoiHTTP() {
  // Baromètre
  HTTPClient http;
  http.begin("http://192.168.0.7:8080/json.htm?type=command&param=udevice&idx=3529&nvalue=0&svalue="+String(Gval[0])+";"+String(Gval[2])+";0;"+String(Gval[1])+";"+String(forecast));
  http.GET();
  http.end();
  // T° du ciel (soit T° soit couverture nuageuse)
  http.begin("http://192.168.0.7:8080/json.htm?type=command&param=udevice&idx=484&nvalue=0&svalue="+String(Gval[7]));
  //http.begin("http://192.168.0.7:8080/json.htm?type=command&param=udevice&idx=484&nvalue=0&svalue="+String(CouvNuages));
  http.GET();
  http.end();
}

//-----------------------------------------------
void webStatus() {
  // Envoi les infos météo à Indi
  //TODO Voir pour remplacer la température du ciel par la couverture nuageuse
  //String msg = "pluie="+String(Gval[3])+"\ntemperature=" + String(Gval[0]) + "\nvent="+String(Gval[5])+"\nrafale="+String(Gval[6])+"\nnuages="+String(50-Gval[7]+Gval[0])+"\n";
  String msg = "pluie="+String(Gval[3])+"\ntemperature=" + String(Gval[0]) + "\nvent="+String(Gval[5])+"\nrafale="+String(Gval[6])+"\nnuages="+String(CouvNuages)+"\n";
  server.send(200, "text/plain", msg);
  //server.send(200, "application/json", msg);
}

//-----------------------------------------------
void webMeteo() {
  // Envoi les infos météo
  String msg = "Temperature = " + String(Gval[0]) + " C\n";
  msg = msg + "Humidite = " + String(Gval[2]) + "%\n";
  msg = msg + "Pression = " + String(Gval[1]) + " Hpa " + String(weather[forecast]) + "\n";
  server.send(200, "text/plain", msg + "\n");
}

double cloudIndex(double Tsky) {
  double Tcloudy = CLOUD_TEMP_OVERCAST, Tclear = CLOUD_TEMP_CLEAR;
  //double Tsky = skyTemp();
  double Index;
  if (Tsky < Tclear) Tsky = Tclear;
  if (Tsky > Tcloudy) Tsky = Tcloudy;
  Index = (Tsky - Tclear) * 100 / (Tcloudy - Tclear);
  return Index;
}
