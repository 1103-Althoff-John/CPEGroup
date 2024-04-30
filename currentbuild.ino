/*
buzzer = 12;
Keypad = 48, 46,44,42(ROWS) 40,38,36,34(COLMS);
LCD; rs = 11; en = 10; d4 = 2; d5 = 3; d6 = 4; d7 = 5;
RealTime = 22,24,26;
Stepper: 23,25,27,29;
Photresistor: A0
Buttons: (off) 49 (on) 47; 
LEDS: RED(Locked) 53 Green(Unlocked) 51;
RFID: 45,43,41,39,37
*/


#define RDA 0x80
#define TBE 0x20
#include <LiquidCrystal.h>
#include <Keypad.h>
#include <Stepper.h>
#include <Wire.h>
#include <I2C_RTC.h>


const int ROW_NUM = 4;
const int COLUMN_NUM = 4; 

volatile unsigned char *myUCSR0A = (unsigned char *)0x00C0;
volatile unsigned char *myUCSR0B = (unsigned char *)0x00C1;
volatile unsigned char *myUCSR0C = (unsigned char *)0x00C2;
volatile unsigned int  *myUBRR0  = (unsigned int *) 0x00C4;
volatile unsigned char *myUDR0   = (unsigned char *)0x00C6;

volatile unsigned char* my_ADMUX = (unsigned char*) 0x7C;
volatile unsigned char* my_ADCSRB = (unsigned char*) 0x7B;
volatile unsigned char* my_ADCSRA = (unsigned char*) 0x7A;
volatile unsigned int* my_ADC_DATA = (unsigned int*) 0x78;

volatile unsigned char *portB     = 0x25;
volatile unsigned char *portDDRB  = 0x24;
volatile unsigned char *pinB = 0x23;

volatile unsigned char *myTCCR1A  = 0x80;
volatile unsigned char *myTCCR1B  = 0x81;
volatile unsigned char *myTCCR1C  = 0x82;
volatile unsigned char *myTIMSK1  = 0x6F;
volatile unsigned char *myTIFR1   = 0x36;
volatile unsigned int  *myTCNT1   = 0x84;

int passcode[4] = {1, 2, 3, 4};
int keypadInput[4];

const int RS = 11, EN = 10, D4 = 2, D5 = 3, D6 = 4, D7 = 5;
LiquidCrystal lcd(RS, EN, D4, D5, D6, D7);
char keys[ROW_NUM][COLUMN_NUM] = {
  {'1','2','3', 'A'},
  {'4','5','6', 'B'},
  {'7','8','9', 'C'},
  {'*','0','#', 'D'}
};

byte pin_rows[ROW_NUM] = {48, 46, 44, 42}; //connect to the row pinouts of the keypad
byte pin_column[COLUMN_NUM] = {40, 38, 36, 34}; //connect to the column pinouts of the keypad

Keypad keypad = Keypad( makeKeymap(keys), pin_rows, pin_column, ROW_NUM, COLUMN_NUM );

bool running = true;
bool passcodeCheck, rfidCheck;
char relock;

void setup() 
{
  //U0init(9600);
  Serial.begin(9600);
  *portDDRB |= 0x40;
  lcd.print("Enter Passcode");
}

void loop() 
{
  int correctNums = 0;


  while(running == true){
        //get input from light sensor and set LED brightness
        char key1 = keypad.getKey();
        if(key1 == '*'){
          lcd.clear();
          for(int i = 0; i < 4;){
            char key = keypad.getKey();
            if(key){
              
              Serial.println(key);
              lcd.setCursor(i,0);
              lcd.print(key);
              keypadInput[i] = key;
              delay(100);
              i++;
              
            }
          }
          for(int i = 0; i < 4; i++){
          
            if(keypadInput[4] > 0) {
              if(keypadInput == 30068){
              correctNums++;
              lcd.clear();
              lcd.print("Correct");
              }
              else{
              *pinB |= 0x40;
              
              lcd.clear();
              my_delay(250);
              lcd.print('keypadInput');
              }
            }
          }
          if(correctNums == 4){
            passcodeCheck = true;
            if(passcodeCheck == true || rfidCheck == true){

            }
          }
        }
        //check for keypad input for relocking # 
        if(key1 == '#'){
            //make close lock
            //reverse nice beep
        }        
    }
  }



void my_delay(unsigned int freq)
{
  // calc period
  double period = 1.0/double(freq);
  // 50% duty cycle
  double half_period = period/ 2.0f;
  // clock period def
  double clk_period = 0.0000000625;
  // calc ticks
  unsigned int ticks = half_period / clk_period;
  // stop the timer
  *myTCCR1B &= 0xFB;
  // set the counts
  *myTCNT1 = (unsigned int) (65536 - ticks);
  // start the timer  
  * myTCCR1A = 0x0;
  * myTCCR1B |= 0b00000001;
  // wait for overflow
  while((*myTIFR1 & 0x01)==0); // 0b 0000 0000
  // stop the timer
  *myTCCR1B &= 0xF8;   // 0b 0000 0000
  // reset TOV           
  *myTIFR1 |= 0x01;
  // Generate square wave on Pin 12 with specified frequency
}

void adc_init()
{
  // setup the A register
  *my_ADCSRA |= 0b10000000; // set bit   7 to 1 to enable the ADC
  *my_ADCSRA &= 0b11011111; // clear bit 6 to 0 to disable the ADC trigger mode
  *my_ADCSRA &= 0b11110111; // clear bit 5 to 0 to disable the ADC interrupt
  *my_ADCSRA &= 0b11111000; // clear bit 0-2 to 0 to set prescaler selection to slow reading
  // setup the B register
  *my_ADCSRB &= 0b11110111; // clear bit 3 to 0 to reset the channel and gain bits
  *my_ADCSRB &= 0b11111000; // clear bit 2-0 to 0 to set free running mode
  // setup the MUX Register
  *my_ADMUX  &= 0b01111111; // clear bit 7 to 0 for AVCC analog reference
  *my_ADMUX  |= 0b01000000; // set bit   6 to 1 for AVCC analog reference
  *my_ADMUX  &= 0b11011111; // clear bit 5 to 0 for right adjust result
  *my_ADMUX  &= 0b11100000; // clear bit 4-0 to 0 to reset the channel and gain bits
}

unsigned int adc_read(unsigned char adc_channel_num)
{
  // clear the channel selection bits (MUX 4:0)
  *my_ADMUX  &= 0b11100000;
  // clear the channel selection bits (MUX 5)
  *my_ADCSRB &= 0b11110111;
  // set the channel number
  if(adc_channel_num > 7)
  {
    // set the channel selection bits, but remove the most significant bit (bit 3)
    adc_channel_num -= 8;
    // set MUX bit 5
    *my_ADCSRB |= 0b00001000;
  }
  // set the channel selection bits
  *my_ADMUX  += adc_channel_num;
  // set bit 6 of ADCSRA to 1 to start a conversion
  *my_ADCSRA |= 0x40;
  // wait for the conversion to complete
  while((*my_ADCSRA & 0x40) != 0);
  // return the result in the ADC data register
  return *my_ADC_DATA;
}

void U0init(int U0baud)
{
 unsigned long FCPU = 16000000;
 unsigned int tbaud;
 tbaud = (FCPU / 16 / U0baud - 1);
 // Same as (FCPU / (16 * U0baud)) - 1;
 *myUCSR0A = 0x20;
 *myUCSR0B = 0x18;
 *myUCSR0C = 0x06;
 *myUBRR0  = tbaud;
}

unsigned char U0kbhit()
{
  return *myUCSR0A & RDA;
}

unsigned char U0getchar()
{
  return *myUDR0;
}

void U0putcahr(unsigned char U0pdata)\
{
  while(!(TBE & *myUCSR0A));
  *myUDR0 = U0pdata;
}