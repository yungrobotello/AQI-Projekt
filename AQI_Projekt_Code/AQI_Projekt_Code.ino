#include <Wire.h>
#include <DHT.h>
#include <Adafruit_SGP30.h>
#include <SDS011.h>
#include <MHZ19.h>
#include <SoftwareSerial.h>
#include <ESP8266WiFi.h>
#include <AdafruitIO_WiFi.h>

//DHT22 Konfig
#define DHTTYPE DHT22 
#define DHTPIN 0 //GPIO0/D3
DHT dht(DHTPIN, DHTTYPE);

//SGP30 Konfig
Adafruit_SGP30 sgp;

//SDS011 Konfig
#define SDSTXD 14 //GPIO14/D5
#define SDSRXD 12 //GPIO2/D6
SDS011 my_sds;

//MHZ19C Konfig
#define MHZRXD 13 //GPIO13/D7
#define MHZTXD 15 //GPIO15/D8
SoftwareSerial mhz(MHZRXD, MHZTXD);
int readCO2();
void calibrateZeroPoint();
bool calibrationPerformed = false;

float temp, humidity;
float tvoc;
int error;
float p10, p25;
int co2;

//IO Konfig
#define WIFI_SSID "***"  //WLAN-SSID
#define WIFI_PASS "***"  //WLAN-Passwort
#define IO_USERNAME "***" //Benutzername meines Accounts bei adafruit.io
#define IO_KEY "***" //Adafruit IO Key

AdafruitIO_WiFi io(IO_USERNAME, IO_KEY, WIFI_SSID, WIFI_PASS); //Verbindungsobjekt

//Adafruit IO Feeds
AdafruitIO_Feed *Temperatur = io.feed("Temperatur");
AdafruitIO_Feed *Luftfeuchtigkeit = io.feed("Luftfeuchtigkeit");
AdafruitIO_Feed *TVOCs = io.feed("TVOC's");
AdafruitIO_Feed *FeinstaubPM25 = io.feed("Feinstaub PM2.5");
AdafruitIO_Feed *FeinstaubPM10 = io.feed("Feinstaub PM10");
AdafruitIO_Feed *Co2 = io.feed("Co2");

void setup() {
  Serial.begin(115200);

  while(! Serial); //Wartet auf serielle Verbindung

  Serial.print("Connecting to Adafruit IO");

  io.connect(); //Baut Verbindung zu io.adafruit.com auf

  while(io.status() < AIO_CONNECTED) {  //Wartet auf die Verbindung
    Serial.print(".");
    delay(500);
  }

  Serial.println();
  Serial.println(io.statusText()); //Gibt Verbindungsstatus aus
  
  dht.begin();
  sgp.begin();
  my_sds.begin(SDSTXD, SDSRXD);
  mhz.begin(9600);

  if (!calibrationPerformed) {
    calibrateZeroPoint();
    calibrationPerformed = true;
  }

  delay(10000);
}

void loop() {
  io.run();

  //Temp und Luftfeuchte
  temp = dht.readTemperature();
  humidity = dht.readHumidity();
  
  //tVOC Werte
  if (sgp.IAQmeasure()){
    tvoc = sgp.TVOC;
  }
  
  //PM2.5 und PM10
  error = my_sds.read(&p25, &p10); 

  //CO2 
  co2 = readCO2();

  //Konsolenausgabe
  // Serial.println("Temperatur: " + String(temp) + "°C");
  // Serial.println("Luftfeuchtigkeit: " + String(humidity) + "%");
  // Serial.println("TVOC: " + String(tvoc) + " ppb");
  // if(!error){
  //   Serial.println("P2.5: " + String(p25) + " μg/m³");
  //   Serial.println("P10:  " + String(p10) + " μg/m³");
  // }
  // Serial.println("Co2: " + String(co2) + " ppm");
  // Serial.println("------");

  //Sendung der Daten zu Adafruit.io
  Temperatur->save(temp);
  Luftfeuchtigkeit->save(humidity);
  TVOCs->save(tvoc);
  FeinstaubPM25->save(p25);
  FeinstaubPM10->save(p10);
  Co2->save(co2);

  delay(20000);
}

//Auslesen der Daten vom MHZ19
int readCO2() {
    byte cmd[9] = {0xFF, 0x01, 0x86, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79};
    mhz.write(cmd, sizeof(cmd));
    
    byte response[9];
    if (mhz.readBytes(response, 9) == 9) {
        int high = response[2];
        int low = response[3];
        return (high * 256 + low); 
    }
    return -1;
}

//Kalibrierung des MHZ19
void calibrateZeroPoint() {
    byte cmd[9] = {0xFF, 0x01, 0x87, 0x00, 0x00, 0x00, 0x00, 0x00, 0x78};
    mhz.write(cmd, sizeof(cmd));
}