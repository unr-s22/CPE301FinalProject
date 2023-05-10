// Water level Senor
// By Tyler Sar
// 5/1/2023

// Definition

#define RDA 0x80  // Receive Data Avaliable
#define TBE 0x20  // Transfer Buffer Empty
#define SENSOR_PIN 1 // Analog Pin 1
#define MIN_WATER_LEVEL 70.0 // The water level

// Water Level Senor Address
volatile unsigned char *port_f = (unsigned char*) 0x31;
volatile unsigned char *ddr_f = (unsigned char*) 0x30;
volatile unsigned char *pin_f = (unsigned char*) 0x2F;

// Analog
volatile unsigned char* my_ADMUX = (unsigned char*) 0x7C;
volatile unsigned char* my_ADCSRB = (unsigned char*) 0x7B;
volatile unsigned char* my_ADCSRA = (unsigned char*) 0x7A;
volatile unsigned int* my_ADC_DATA = (unsigned int*) 0x78;
  
volatile unsigned char *myUCSR0A = (unsigned char *)0x00C0;
volatile unsigned char *myUCSR0B = (unsigned char *)0x00C1;
volatile unsigned char *myUCSR0C = (unsigned char *)0x00C2;
volatile unsigned int  *myUBRR0  = (unsigned int *) 0x00C4;
volatile unsigned char *myUDR0   = (unsigned char *)0x00C6;




void setup() {
  Serial.begin(9600);
  // Initialize the ADC
  adc_init();
}

void loop() {

  // Read the voltage from the sensor
  float voltage = readVoltage(SENSOR_PIN);

  // calculate the level based on the reading
  float waterLevel = (voltage - 0.5) * 100;
  // testing the readings
  Serial.println(waterLevel);

  // check water level
  if (waterLevel < MIN_WATER_LEVEL){
      // display this message when water is low
      Serial.println("Water is too low");
  }

  delay(1000);
}
  // read from the analog pin
  float readVoltage(int pinIn){
    *ddr_f &= 0b11111101;
    *port_f |= 0b00000010;

   // set the pin as input and read the voltage
    float voltage = adc_read(pinIn) * 5.0 / 1023.0;

    return voltage;
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
