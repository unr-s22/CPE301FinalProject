//by Gavin Casper

#include "DHT.h" //humidity and temperature sensor library - Adafruit
#include "RTClib.h" //clock library - Adafruit

#define DHTPIN 12
#define DHTTYPE DHT11 //our dht type

DHT dht(DHTPIN, DHTTYPE);
unsigned int tempMax = 26;  //temperature threshold for state change

RTC_DS1307 rtc;
int prevMin = 0;
DateTime now;

void setup() {
  Serial.begin(9600);
  Serial.println(F("DHT test:"));

  dht.begin();

  while (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
  }
  rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  prevMin = rtc.now().minute() - 1;
}


void loop() {
  now = rtc.now();

  dht.read(12); // read digital 12

  // Wait a few seconds between measurements.

  // reading temp or humidity takes/delays about 250 milliseconds
  //slow sensor therefore some readings may be old (2 second delay)
  float h = dht.readHumidity();
  // read temp as Celsius (default)
  float t = dht.readTemperature();
  // read temp as Fahrenheit (isFahrenheit = true)
  float f = dht.readTemperature(true);

  //Check for failed reads and try again
  if (isnan(h) || isnan(t) || isnan(f)) {
    Serial.println(F("Failed to read from DHT sensor!"));
    return;
  }

  // Compute heat index in Fahrenheit (the default)
  float hif = dht.computeHeatIndex(f, h);
  // Compute heat index in Celsius (isFahreheit = false)
  float hic = dht.computeHeatIndex(t, h, false);

  Serial.print(F("Humidity: "));
  Serial.print(h);
  Serial.print(F("%  Temperature: "));
  Serial.print(f);
  Serial.print(F("°F  Heat index: "));
  Serial.print(hif);
  Serial.println(F("°F"));
}
