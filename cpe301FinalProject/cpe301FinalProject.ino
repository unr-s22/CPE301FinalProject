/****************************************
 * Names: Carter Webb, Gavin Casper,
          Ryan Du, Tyler Sar
 * Assignment: Final Project
 * Date: 04/18/2023
 * Class: CPE 301 Spring 2023
 ****************************************/
/* --------- Libraries --------- */
#include <LiquidCrystal.h>
#include <Stepper.h>
#include <DHT.h>
#include <RTClib.h>

/* --------- Definitions --------- */
#define RDA 0x80 // Receive Data Avaliable
#define TBE 0x20 // Transfer Buffer Empty
#define DHTPIN 12 
#define DHTTYPE DHT11 //our dht type
#define SENSOR_PIN 1 // Analog Pin 1
#define MIN_WATER_LEVEL 10

/* --------- Addresses --------- */
volatile unsigned char *myUCSR0A = (unsigned char *)0x00C0;
volatile unsigned char *myUCSR0B = (unsigned char *)0x00C1;
volatile unsigned char *myUCSR0C = (unsigned char *)0x00C2;
volatile unsigned int  *myUBRR0  = (unsigned int *) 0x00C4;
volatile unsigned char *myUDR0   = (unsigned char *)0x00C6;

volatile unsigned char* my_ADMUX = (unsigned char*) 0x7C;
volatile unsigned char* my_ADCSRB = (unsigned char*) 0x7B;
volatile unsigned char* my_ADCSRA = (unsigned char*) 0x7A;
volatile unsigned int* my_ADC_DATA = (unsigned int*) 0x78;

volatile unsigned char *port_b = (unsigned char*) 0x25;
volatile unsigned char *ddr_b = (unsigned char*) 0x24;
volatile unsigned char *pin_b = (unsigned char*) 0x23;

/* --------- Global Variables --------- */
// Disabled = 0, Idle = 1, Running = 2, Error = 3.
int stateNum = 1;
bool startButtonReleased = true;

/* ---- LCD Variables ---- */
// Where LCD is on Arduino - Digital Pins 7, 6, 5, 4, 3, 2
const int rs = 7, en = 6, d4 = 5, d5 = 4, d6 = 3, d7 = 2;
// Setup lcd
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

/* ---- Time Variables ---- */
// Initialize Real Time Clock
RTC_DS1307 rtc;
// Initialize Current Time
DateTime currentTime;
int prevMin = 0;

/* ---- DHT Variables ---- */
DHT dht(DHTPIN, DHTTYPE);
unsigned int tempMax = 26;  //temperature threshold for state change

/* ---- Stepper Variables ---- */
int previousStep = 0;
Stepper stepper(2038, 8, 10, 9, 11);

void setup() {
  U0init(9600);
  // Initialize LCD
  lcd.begin(16, 2);
  // Initialize DHT
  dht.begin();

  stepper.setSpeed(15);
  *ddr_b |= 0x80;
  adc_init();

  // Start the RTC
  rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  prevMin = rtc.now().minute() - 1;

  while (! rtc.begin()) {
    printToSerial("Couldn't find RTC");
  }

  outputLCDScreen(0, 0);
}

void loop() {
  float voltage = readVoltage(SENSOR_PIN);
  float waterLevel = (voltage - 0.5) * 100;  
  currentTime = rtc.now();

  dht.read(12); // Reads Digital Pin 12

  // reading temp or humidity takes/delays about 250 milliseconds
  //slow sensor therefore some readings may be old (2 second delay)
  float hum = dht.readHumidity();
  // read temp as Fahrenheit (isFahrenheit = true)
  float fahren = dht.readTemperature(true);

  //Check for failed reads and try again
  if (isnan(hum) || isnan(fahren)) {
    printToSerial("Failed to Read");
    U0putChar('d');
    return;
  }

  if (waterLevel < MIN_WATER_LEVEL){
    printToSerial("Water is too low");
  }

  delay(1000);

  stepperControl();

  outputLCDScreen(hum, fahren);
}

//Continuously call this function to keep checking
//the potentiometer and adjust the stepper motor as needed
void stepperControl() {
  int p_step = adc_read(0);
  stepper.step(p_step - previousStep);
  previousStep = p_step;  
}

//Call this function with 1 to turn on the motor
//Call this function with a 0 to turn off the motor
void motorControl(int input) {
  if (input) {
    *port_b |= 0x80;
  }
  else {
    *port_b &= 0b01111111;
  }
}

float readVoltage(int pin){
  adc_read(1);
}

void outputLCDScreen(float h, float f) {
  // Check if we have a new time to display
  if (currentTime.minute() != prevMin) {
    prevMin = currentTime.minute();

    lcd.setCursor(0, 0);

    // Print humidity values
    lcd.print("Humidity: ");
    lcd.print(h);
    lcd.print("%");

    // Set the Cursor
    lcd.setCursor(0, 1);

    // Print temp values
    lcd.print("Temp: ");
    lcd.print(f);
    lcd.print("F");
  } 
}

void U0init(int U0baud) {
  unsigned long FCPU = 16000000;
  unsigned int tbaud;
  tbaud = (FCPU / 16 / U0baud - 1);

  *myUCSR0A = 0x20;
  *myUCSR0B = 0x18;
  *myUCSR0C = 0x06;
  *myUBRR0  = tbaud;
}

void U0putChar(unsigned char U0pdata) {
  while((*myUCSR0A & TBE)==0);
  *myUDR0 = U0pdata;
}

void adc_init() {
  *my_ADCSRA |= 0b10000000;
  *my_ADCSRA &= 0b11011111;
  *my_ADCSRA &= 0b11110111;
  *my_ADCSRA &= 0b11111000;
  *my_ADCSRB &= 0b11110111;
  *my_ADCSRB &= 0b11111000;
  *my_ADMUX  &= 0b01111111;
  *my_ADMUX  |= 0b01000000;
  *my_ADMUX  &= 0b11011111;
  *my_ADMUX  &= 0b11100000;
}

unsigned int adc_read(unsigned char adc_channel_num) {
  *my_ADMUX  &= 0b11100000;
  *my_ADCSRB &= 0b11110111;
  if(adc_channel_num > 7) {
    adc_channel_num -= 8;
    *my_ADCSRB |= 0b00001000;
  }
  *my_ADMUX  += adc_channel_num;
  *my_ADCSRA |= 0x40;
  while((*my_ADCSRA & 0x40) != 0);
  return *my_ADC_DATA;
}

bool readFromPin(volatile unsigned char* port, unsigned int pin) {
  return !((*port & (0x01 << pin)) == 0x00);
}

void printToSerial (String s) {
    String toPrint = String(currentTime.year());
    for(int i = 0; i < toPrint.length(); i++) {
      U0putChar(toPrint[i]);
    }

    U0putChar('/');
    toPrint = currentTime.month();
    
    for(int i = 0; i < toPrint.length(); i++) {
      U0putChar(toPrint[i]);
    }

    U0putChar('/');
    toPrint = currentTime.day();
    
    for(int i = 0; i < toPrint.length(); i++) {
      U0putChar(toPrint[i]);
    }

    U0putChar(' ');
    toPrint = currentTime.hour();
    
    for(int i = 0; i < toPrint.length(); i++) {
      U0putChar(toPrint[i]);
    }

    U0putChar(':');
    toPrint = currentTime.minute();
    
    for(int i = 0; i < toPrint.length(); i++) {
      U0putChar(toPrint[i]);
    }

    U0putChar(':');
    toPrint = currentTime.second();
    
    for(int i = 0; i < toPrint.length(); i++) {
      U0putChar(toPrint[i]);
    }
    
    s += "\n";    
    
    for(int i = 0; i < s.length(); i++) {
        U0putChar(s[i]);
    }
}

ISR (PCINT1_vect) /* PCINT1 for PJ1 */ {
    if(/* Input of the pin the button is connected to */) {
        startButtonReleased = true;
        if(stateNum != 0) {
            printToSerial("Disabled state entered");
            stateNum = 0; // Disabled
            prevMin--;

            motorControl(0); // turn off fan motor
            
            // We want to write out to our indicator Lights here
        } else {
            printToSerial("Idle state entered");
            stateNum = 1; // Idle
            prevMin--;

            // We want to write out to our indicator Lights here
        }
    }

    if(/* Input of the pin the button is connected to */) {
        if(startButtonReleased) {
            startButtonReleased = false;
        }
    }
}