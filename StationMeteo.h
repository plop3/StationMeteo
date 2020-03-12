#define DEBUG_OFF
// Communication série
#define SERIALBAUD 9600
// BME280
#define ALTITUDE 34.00 // Local Altitude
// Capteur UV
#define RP_UV 1.3       // Multiplicateur index UV (du au cache dépoli)

// Pluviomètre
#define Plevel 517	// mm de pluie/1000 par impulsion (Réel: 0.517mm/imp)
// Pins
#define PINrain			D4 // Pluviomètre
#define ONE_WIRE_BUS 	D3 // PIN 1-wire
#define PinPluie 		D6 // Capteur de pluie
#define IRQ_ORAGE 		D5 // PIN IRQ MOD1016	
#define DATAPIN			D7 // Anémomètre Lacrosse TX20