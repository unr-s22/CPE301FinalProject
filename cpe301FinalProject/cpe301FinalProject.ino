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
#define TEMP_THRESHOLD 72.0 // In Fahrenheit

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

// LED Addresses
volatile unsigned char *port_a = (unsigned char*) 0x22;
volatile unsigned char *ddr_a = (unsigned char*) 0x21;
volatile unsigned char *pin_a = (unsigned char*) 0x20;

// Digital Pin Addresses
volatile unsigned char *port_b = (unsigned char*) 0x25;
volatile unsigned char *ddr_b = (unsigned char*) 0x24;
volatile unsigned char *pin_b = (unsigned char*) 0x23;

// Button Addresses
volatile unsigned char *port_j = (unsigned char*) 0105;
volatile unsigned char *ddr_j = (unsigned char*) 0x104;
volatile unsigned char *pin_j = (unsigned char*) 0x2103;

// Timer Addresses
volatile unsigned char *myTCCR1A = (unsigned char *) 0x80;
volatile unsigned char *myTCCR1B = (unsigned char *) 0x81;
volatile unsigned char *myTCCR1C = (unsigned char *) 0x82;
volatile unsigned char *myTIMSK1 = (unsigned char *) 0x6F;
volatile unsigned int  *myTCNT1  = (unsigned  int *) 0x84;
volatile unsigned char *myTIFR1 =  (unsigned char *) 0x36;

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

/* ---- LED Status Pins ---- */
//Green LED:  pin 23
//Blue LED:   pin 25
//Yellow LED: pin 27
//Red LED:    pin 29

/* ---- Button Pin ---- */
// Digital Pin 0

void setup() {
  U0Init(9600);
  // Initialize LCD
  lcd.begin(16, 2);
  // Initialize DHT
  dht.begin();

  stepper.setSpeed(15);
  *ddr_b |= 0x80;
  
  *ddr_j &= 0b11111101;
  *port_j |= 0b00000010;

  adc_init();

  // Start the RTC
  rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  prevMin = rtc.now().minute() - 1;

  while (! rtc.begin()) {
    printToSerial("Couldn't find RTC");
  }

  outputLCDScreen(0, 0);

  //Setup output pins for port A
  *ddr_a |= 0xAA;
}

void loop() {
  stepperControl();
  
  if (stateNum == 0) {
    disabledState();
  } else if (stateNum == 1) {
    idleState();
  } else if (stateNum == 2) {
    runningState();
  } else if (stateNum == 3) {
    errorState();
  }
}

void disabledState() {
  // Handled by ISR
}

void idleState() {
    printToSerial("Idle state entered");
    //Turn on green LED and turn off others
    *port_a |= 0x02;
    *port_a &= 0x56;

    float waterLevel = (readVoltage(SENSOR_PIN) - 0.5) * 100;  
    currentTime = rtc.now();
    float hmdty = dht.readHumidity();
    float temp = dht.readTemperature(true);

    outputLCDScreen(hmdty, temp);

    //compare temperature with threshold
    if ( temp > TEMP_THRESHOLD) {
        printToSerial("Temperature is above threshold");
        stateNum = 2;
    }

    //check water level
    if (waterLevel < MIN_WATER_LEVEL){
        printToSerial("Water is too low");
        stateNum = 3;
    }
}

void runningState() {
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

  stepperControl();

  outputLCDScreen(hum, fahren);

  if (waterLevel < MIN_WATER_LEVEL){
    printToSerial("Water low, entered Error state.");
    stateNum = 3; // Error State
    // Write to LED's
    prevMin--;

    lcd.setCursor(0, 0);
    lcd.print("ERROR:");
    lcd.setCursor(0, 1);
    lcd.print("Water low");

    motorControl(0);
  } else if (dht.readTemperature(true) <= TEMP_THRESHOLD) {
    printToSerial("Temps too high, entered Idle state.");
    stateNum = 1; // Idle State
    // Write to LED's
    motorControl(0);
  }
}

void errorState() {
  printToSerial("Error state entered");
  //turn on red LED and turn off others
  *port_a |= 0x80;
  *port_a &= 0xD5;

  //display info while waiting for state change
  while(stateNum == 3) {
    lcd.setCursor(0, 0);
    lcd.print("ERROR:");
    lcd.setCursor(0, 1);
    lcd.print("Water low");
    //float hum = dht.readHumidity();
    //float fahren = dht.readTemperature(true);
    //outputLCDScreen(hum, fahren);
  }
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

void U0Init(int U0baud) {
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

// PCINT1_vect is for PJ1 input
ISR (PCINT1_vect) {
    if(*pin_j == 1) {
        startButtonReleased = true;
        if(stateNum != 0) {
            printToSerial("Disabled state entered");
            stateNum = 0; // Disabled
            prevMin--;

            motorControl(0); // turn off fan motor
            
            *port_a |= 0b00100000; // Turns on Yellow LED;
            *port_a &= 0b01110101; // Turns off Red, Green, Blue LEDS
        } else {
            printToSerial("Idle state entered");
            stateNum = 1; // Idle
            prevMin--;

            *port_a |= 0b00000010;
            *port_a &= 0b01010111;
        }
    }

    if(*pin_j == 3) {
        if(startButtonReleased) {
            startButtonReleased = false;
        }
    }
}
