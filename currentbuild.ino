/*
buzzer = 12;
Keypad = 48, 46,44,42(ROWS) 40,38,36,34(COLMS);
LCD; rs = 11; en = 10; d4 = 2; d5 = 3; d6 = 4; d7 = 5;
RealTime = 22,24,26;
Stepper: 23,25,27,29;
Photresistor: A0
Buttons: (off) 49 (on) 47; 
LEDS: RED(Locked) 10 Green(Unlocked) 9;
RFID: 45,43,41,39,37
*/


#define RDA 0x80
#define TBE 0x20
//#include <LiquidCrystal.h>
#include <Keypad.h>
#include <Stepper.h>
#include <Wire.h>
#include <SPI.h>
#include <MFRC522.h>
#include <I2C_RTC.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SS_PIN 53
#define RST_PIN 5
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_ADDR 0x3C

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT);

MFRC522 mfrc522(SS_PIN, RST_PIN);

const int ROW_NUM = 4;
const int COLUMN_NUM = 4;

volatile unsigned char *myUCSR0A = (unsigned char *)0x00C0;
volatile unsigned char *myUCSR0B = (unsigned char *)0x00C1;
volatile unsigned char *myUCSR0C = (unsigned char *)0x00C2;
volatile unsigned int *myUBRR0 = (unsigned int *)0x00C4;
volatile unsigned char *myUDR0 = (unsigned char *)0x00C6;

volatile unsigned char *my_ADMUX = (unsigned char *)0x7C;
volatile unsigned char *my_ADCSRB = (unsigned char *)0x7B;
volatile unsigned char *my_ADCSRA = (unsigned char *)0x7A;
volatile unsigned int *my_ADC_DATA = (unsigned int *)0x78;

volatile unsigned char *portB = 0x25;
volatile unsigned char *portDDRB = 0x24;
volatile unsigned char *pinB = 0x23;

volatile unsigned char *myTCCR1A = 0x80;
volatile unsigned char *myTCCR1B = 0x81;
volatile unsigned char *myTCCR1C = 0x82;
volatile unsigned char *myTIMSK1 = 0x6F;
volatile unsigned char *myTIFR1 = 0x36;
volatile unsigned int *myTCNT1 = 0x84;

byte readCard[4];
String MasterTag = "CFAFDF1D";
String TagID = "";

char passcode[4] = { '1', '2', '3', '4' };


/*const int SCL = 4 SDA = 3;
LiquidCrystal lcd(SCL, SDA);
*/
char keys[ROW_NUM][COLUMN_NUM] = {
  { '1', '2', '3', 'A' },
  { '4', '5', '6', 'B' },
  { '7', '8', '9', 'C' },
  { '*', '0', '#', 'D' }
};

byte pin_rows[ROW_NUM] = { 48, 46, 44, 42 };       //connect to the row pinouts of the keypad
byte pin_column[COLUMN_NUM] = { 40, 38, 36, 34 };  //connect to the column pinouts of the keypad

Keypad keypad = Keypad(makeKeymap(keys), pin_rows, pin_column, ROW_NUM, COLUMN_NUM);

bool running = true;
bool passcodeCheck, rfidCheck;
char relock;
int stepsPerRevolution = 500;
int invertedSPR = -500;
Stepper myStepper(stepsPerRevolution, 23, 25, 27, 29);
int motSpeed = 10;
int locked = 1;

void setup() {
  Serial.begin(9600);
  U0init(9600);
  adc_init();
  *portDDRB |= 0x40;
  *portDDRB |= 0x80;
  *portDDRB |= 0x10;
  SPI.begin();
  mfrc522.PCD_Init();
  mfrc522.PCD_DumpVersionToSerial();
  myStepper.setSpeed(motSpeed);
  //lcd.print("Enter Passcode");
  display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR);
  display.clearDisplay();

  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.println("==========");

  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(3, 17);
  display.println("Smart Lock");
  display.setCursor(30, 40);
  display.println("V 2.8");
  display.display();
  delay(2000);
  display.clearDisplay();
  display.setCursor(25, 0);
  display.setTextSize(2);
  display.println("Press");
  display.setTextSize(1);
  display.setCursor(17, 20);
  display.println("* To Unlock or");
  display.setCursor(28, 30);
  display.println("# To lock");
  display.display();
  display.setTextSize(2);
}

void loop() {
  int correctNums = 0;
  int ID = 0;

  while (running == true) {
    *portB |= (0x01 << 7);
    *portB &= ~(0x01 << 4); 
    unsigned int adc = adc_read(15);
    int tempf = adc/7.1;
    Serial.println(tempf);
    if(tempf >= 120) {
      *portB &= ~(0x01 << 7);
      *portB |= (0x01 << 4);
      display.clearDisplay();
      display.setCursor(0, 0);
      display.println("Fire Alert");
       *pinB |= 0x40;
       my_delay(1000);
      display.display();
      delay(10);
      running = false;
    }
    //get input from light sensor and set LED brightness
    char key1 = keypad.getKey();
    if (key1 == '*') {
      display.clearDisplay();
      display.display();
      char keypadInput[4];
      for (int i = 0; i < 4;) {
        char key = keypad.getKey();
        ID = getID();
        if (TagID == MasterTag) {
          break;
        }
        if (locked == 0) {
          display.setCursor(20, 17);
          display.println("Already");
          display.setCursor(18, 37);
          display.println("Unlocked");
          display.display();
          delay(2000);
          *pinB |= 0x40;
          my_delay(1000);
          delay(500);
          *pinB |= 0x00;
          display.clearDisplay();
          display.setCursor(20, 17);
          display.println("Locking");
          display.display();
          myStepper.step(invertedSPR);
          delay(1000);
          myStepper.step(0);
          display.clearDisplay();
          display.setCursor(30, 0);
          display.println("Locked");
          display.setCursor(20, 30);
          display.println("Press *");
          display.setCursor(10, 50);
          display.println("To Unlock");
          display.display();
          locked = 1;
          break;
        }
        if (key) {
          display.setCursor(i * 11 + 40, 17);
          display.println(key);
          display.display();
          U0putchar(key);
          keypadInput[i] = key;
          delay(100);
          i++;
        }
      }
      for (int g = 0; g < 4; g++) {
        if (keypadInput[g] > 0 || TagID == MasterTag) {
          if (keypadInput[g] == passcode[g]) {
            correctNums++;
          }
          if (correctNums == 4 || TagID == MasterTag) {
            //turn on step motor, turn on green light
            display.clearDisplay();
            display.setCursor(6, 0);
            display.println("Unlocking");
            display.display();
            display.clearDisplay();
            myStepper.step(stepsPerRevolution);
            delay(100);
            myStepper.step(0);
            *pinB |= 0x40;
            my_delay(500);
            delay(500);
            *pinB |= 0x00;

            display.setCursor(6, 0);
            display.println(" Unlocked");
            display.setCursor(20, 30);
            display.println("Press #");
            display.setCursor(20, 50);
            display.println("To lock");
            display.display();
            locked = 0;
            correctNums = 0;
            TagID = "";
          } else if (correctNums != 4) {
            *pinB |= 0x40;
            my_delay(250);
            delay(500);
            *pinB |= 0x00;

          }
        }
      }
      //check for keypad input for relocking #
    }
    if (key1 == '#') {
      //make close lock
      //reverse nice bee
      if (locked == 0) {
        *pinB |= 0x40;
        my_delay(250);
        delay(500);
        *pinB |= 0x00;
        display.clearDisplay();
        display.setCursor(20, 17);
        display.println("Locking");
        display.display();
        myStepper.step(invertedSPR);
        delay(500);
        myStepper.step(0);
        display.clearDisplay();
        display.setCursor(30, 0);
        display.println("Locked");
        display.setCursor(20, 30);
        display.println("Press *");
        display.setCursor(10, 50);
        display.println("To Unlock");
        display.display();
        locked = 1;
      } else {
        display.clearDisplay();
        display.setCursor(22, 18);
        display.println("Already");
        display.setCursor(28, 40);
        display.println("Locked");
        display.display();
        delay(2000);
        break;
      }
    }
  }
}




void my_delay(unsigned int freq) {
  // calc period
  double period = 1.0 / double(freq);
  // 50% duty cycle
  double half_period = period / 2.0f;
  // clock period def
  double clk_period = 0.0000000625;
  // calc ticks
  unsigned int ticks = half_period / clk_period;
  // stop the timer
  *myTCCR1B &= 0xFB;
  // set the counts
  *myTCNT1 = (unsigned int)(65536 - ticks);
  // start the timer
  *myTCCR1A = 0x0;
  *myTCCR1B |= 0b00000001;
  // wait for overflow
  while ((*myTIFR1 & 0x01) == 0)
    ;  // 0b 0000 0000
  // stop the timer
  *myTCCR1B &= 0xF8;  // 0b 0000 0000
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

void U0init(int U0baud) {
  unsigned long FCPU = 16000000;
  unsigned int tbaud;
  tbaud = (FCPU / 16 / U0baud - 1);
  // Same as (FCPU / (16 * U0baud)) - 1;
  *myUCSR0A = 0x20;
  *myUCSR0B = 0x18;
  *myUCSR0C = 0x06;
  *myUBRR0 = tbaud;
}

unsigned char U0kbhit() {
  return *myUCSR0A & RDA;
}

unsigned char U0getchar() {
  return *myUDR0;
}

void U0putchar(unsigned char U0pdata) {
  while (!(TBE & *myUCSR0A));
  *myUDR0 = U0pdata;
}

uint8_t getID() {
  // Getting ready for Reading PICCs
  if (!mfrc522.PICC_IsNewCardPresent()) {  //If a new PICC placed to RFID reader continue
    return 0;
  }
  if (!mfrc522.PICC_ReadCardSerial()) {  //Since a PICC placed get Serial and continue
    return 0;
  }
  TagID = "";
  for (uint8_t i = 0; i < 4; i++) {  // The MIFARE PICCs that we use have 4 byte UID
    readCard[i] = mfrc522.uid.uidByte[i];
    TagID.concat(String(mfrc522.uid.uidByte[i], HEX));  // Adds the 4 bytes in a single String variable
  }
  TagID.toUpperCase();
  mfrc522.PICC_HaltA();  // Stop reading
  return 1;
}
