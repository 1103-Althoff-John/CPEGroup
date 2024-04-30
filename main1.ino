/*
 *  Names: Jackson Robertson, Timmy Norris, John Althoff
 *  Assignment: Final Project 
 *  Class: CPE301 Spring 2023
 *  Date: 4/30/2024
 */

int passcode, keypadInput;
passcode = 1234;
bool running, passcodeCheck, rfidCheck;
char relock;

void setup() {

}

void loop() {
    while(running){
        //get input from light sensor and set LED brightness
        //get input from keypad set to keypadInput
        //display inputted numbers to the LED
        if(keypadInput == passcode){
            passcodeCheck = true; 
        }
        else //make buzzer go off
        //check for rfid and set rfidCheck
        if(passcodeCheck & rfidCheck){
            //OpenLock
            //nice beep
        }
        //check for keypad input for relocking # 
        if(relock == '#'){
            //make close lock
            //reverse nice beep
        }        
        //check for off button and if pressed set running to false
    }
}
