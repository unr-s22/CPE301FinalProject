//Name: Ryan Du
//Date: 4/19/23
//Purpose: This file contains code used to control the stepper motor used to 
//         change the vent angle (controlled by a potentiometer), and the DC
//         motor for the fan of the swap cooler.

#include <Stepper.h>

#define RDA 0x80
#define TBE 0x20 

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
int previous = 0;

Stepper stepper(2038, 8, 10, 9, 11);

void setup() {
  stepper.setSpeed(15);
  *ddr_b |= 0x80;
  adc_init();
}

void loop() {
  stepperControl();
}

//Continuously call this function to keep checking
//the potentiometer and adjust the stepper motor as needed
void stepperControl() {
  int p_step = adc_read(0);
  stepper.step(p_step - previous);
  previous = p_step;  
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