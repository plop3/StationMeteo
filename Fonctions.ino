void infoMeteo() {
  // BME280 Mise à jour des prévisions barométriques
  forecast = sample(P);
  // Récupération des infos Domoticz (Vent)
  // TODO
  // Envoi des donées à Domoticz
  // Index UV, luminosité, T°/H£/Pres. , T° ciel, couverture nuageuse 
  envoiHTTP();
}

void envoiHTTP() {
  // Baromètre
  HTTPClient http;
  if (http.begin("http://192.168.0.7:8080/json.htm?type=command&param=udevice&idx=3556&nvalue=0&svalue="+String(Tp)+";"+String(HR)+";0;"+String(P/100)+";"+String(forecast))) {
	http.GET();
	http.end();
	// T° du ciel, couverture nuageuse
	http.begin("http://192.168.0.7:8080/json.htm?type=command&param=udevice&idx=3557&nvalue=0&svalue="+String(skyT));
	http.GET();
	http.end();
    http.begin("http://192.168.0.7:8080/json.htm?type=command&param=udevice&idx=3558&nvalue=0&svalue="+String(Clouds));
	http.GET();
	http.end();
	// Index UV
	http.begin("http://192.168.0.7:8080/json.htm?type=command&param=udevice&idx=3560&nvalue=0&svalue="+String(UVindex));
	http.GET();
	http.end();
	// Luminosité
		http.begin("http://192.168.0.7:8080/json.htm?type=command&param=udevice&idx=3559&nvalue=0&svalue="+String(luminosite));
	http.GET();
	http.end();
  }
}
//-----------------------------------------------

