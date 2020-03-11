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
  // 1m

  // H% sol

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
  }
}
//-----------------------------------------------

void rainCount() {
		// Incrémente le compteur de pluie
		CountRain+=Plevel;
		updateRain=true;
}
