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
#include <dht11.h>
#include <RTClib.h>

/* --------- Global Variables --------- */

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
int previousMin = 0;

void setup() {
  U0init(9600);
  // Initialize LCD
  lcd.begin(16, 2);

  // Start the RTC
  rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  previousMin = rtc.now().minute() - 1;

  outputLCDScreen();
}

void loop() {
  currentTime = rtc.now();

  outputLCDScreen();
}

void outputLCDScreen() {
  // Check if we have a new time to display
  if (currentTime.minute() != previousMin) {
    previousMin = currentTime.minute();

    lcd.setCursor(0, 0);

    // Print humidity values
    lcd.print("Humidity: ");
    //lcd.print((float)dht.humidity);
    lcd.print("%");

    // Set the Cursor
    lcd.setCursor(0, 1);

    // Print temp values
    lcd.print("Temp: ");
    //lcd.print((float)dht.temperature);
    lcd.print("C");
  } 
}

void U0init(int U0baud) {
  unsigned long FCPU = 16000000;
  unsigned int tbaud;
  tbaud = (FCPU / 16 / U0baud - 1);

  //*myUCSR0A = 0x20;
  //*myUCSR0B = 0x18;
  //*myUCSR0C = 0x06;
  //*myUBRR0  = tbaud;
}

void U0putChar(unsigned char U0pdata) {
  //while((*myUCSR0A & TBE)==0);
  //*myUDR0 = U0pdata;
}

void writeToPin(volatile unsigned char* port, unsigned int pin, bool setThePin) {
  if (setThePin) {
    *port |= (0x01 << pin);
  } else {
    *port &= ~(0x01 << pin);
  }
}

bool readFromPin(volatile unsigned char* port, unsigned int pin) {
  return !((*port & (0x01 << pin)) == 0x00);
}