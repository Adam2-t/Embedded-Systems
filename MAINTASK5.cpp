#include "mbed.h"
#include "arm_book_lib.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>




AnalogIn pot(A0);//analog inputs
AnalogIn lm35(A1);
AnalogIn mq2(A2);


DigitalOut alarmLed(LED1); //Digital Hardware outputs
DigitalInOut Buzzer(PE_10);


#define Number_Key 4  //Keypad configuration
#define Keypad_Number_Row 4
#define Keypad_Number_Col 4


DigitalOut keypadRowPins[Keypad_Number_Row] = {PB_3, PB_5, PC_7, PA_15};//keypad pin layout
DigitalIn keypadColPins[Keypad_Number_Col]  = {PB_12, PB_13, PB_15, PC_6};


char codeSequence[Number_Key]   = { '1', '2', '3', '4' }; //Password array
char keyPressed[Number_Key] = { '0', '0', '0', '0' }; 
int MatrixKeypadIndex = 0;


typedef enum {
    MATRIX_KEYPAD_SCANNING,
    MATRIX_KEYPAD_DEBOUNCE,
    MATRIX_KEYPAD_KEY_HOLD_PRESSED
} matrixKeypadState_t;

matrixKeypadState_t matrixKeypadState;

int accumulatedDebounceMatrixKeypadTime = 0; // keypad layout
char matrixKeypadLastKeyPressed = '\0';
char matrixKeypadIndexToCharArray[] = {
    '1', '2', '3', 'A',
    '4', '5', '6', 'B',
    '7', '8', '9', 'C',
    '*', '0', '#', 'D',
};


int TimeAlarm = 0; // For LED timer to stop system disruption

UnbufferedSerial uartUBS(USBTX, USBRX, 115200);

float lm35Temp = 0.0;
float gasPPM = 0.0;

float tempThreshold = 0.0;
float gasThreshold = 0.0;

float oldtempThreshold = 0.0;

bool alarmState = OFF;
bool codePromptDisplayed = false;

#define EVENT_MAX_STORAGE 5
typedef struct systemEvent { // single event
    char typeOfEvent[16];
    float tempValue;
    float gasValue;
} systemEvent_t;

systemEvent_t arrayOfStoredEvents[EVENT_MAX_STORAGE];// array storage
int eventsIndex = 0;

void BeginOutput();
void NewThresholds();
void alarmActivation();
void AlarmDeactivation();
bool areEqual();
float TempCelFormula(float RawReading);

void logThresholdEvent(const char* triggerName, float currentTemp, float currentGas);
void displayEventLog();
char matrixKeypadUpdate();
char matrixKeypadScan();
void matrixKeypadInit();

int main()
{

    BeginOutput();

    while (true) {

        NewThresholds();
        alarmActivation();
        AlarmDeactivation();

        delay(10);

    }
}

void BeginOutput() // makes all alarms off
{
    alarmLed = 0;
    Buzzer.mode(OpenDrain);
    Buzzer.input();
    
    matrixKeypadInit(); 
    
    for(int i = 0; i < EVENT_MAX_STORAGE; i++){
        arrayOfStoredEvents[i].typeOfEvent[0] = '\0'; 
    }
}

void NewThresholds() // math for threshold vlaues set by the potentiometer
{
    float RawValue = pot.read();

    tempThreshold = 25.0 + (RawValue * 12.0);
    gasThreshold = (RawValue * 800.0);

    if( abs(tempThreshold - oldtempThreshold)>0.2 ){ 

        char str[100];

        sprintf(str, "Thershold set to: Temp: %.1f C & Gas: %.0f ppm\r\n", tempThreshold, gasThreshold);
        uartUBS.write(str, strlen(str));

        oldtempThreshold = tempThreshold;
    }

}

float TempCelFormula( float RawReading ) // changes the raw value of temp sensor to celcius
{
    return (RawReading * 3.3 / 0.01 );
}

void alarmActivation()
{
    lm35Temp = TempCelFormula( lm35.read());
    gasPPM = mq2.read() * 1000.0;

    bool tempTooHigh = (lm35Temp > tempThreshold);
    bool gasTooHigh = (gasPPM > gasThreshold);

    if ((tempTooHigh || gasTooHigh) && alarmState == OFF){
        alarmState = ON;
    }

    if (alarmState == ON) {

        if (codePromptDisplayed == false) { //checks if message is displayed
            uartUBS.write("\r\nEnter 4-Digit Code to Deactivate\r\n", 36);
            
            if (tempTooHigh) logThresholdEvent("OVER_TEMP", lm35Temp, gasPPM); // stores data to log threshold if it is over threshold
            else if (gasTooHigh) logThresholdEvent("HIGH_GAS", lm35Temp, gasPPM);
            
            codePromptDisplayed = true; 
        }

        Buzzer.output();
        Buzzer = LOW;

        if (alarmState == ON){

        TimeAlarm = TimeAlarm + 10;

        if (TimeAlarm >= 100) {
            TimeAlarm = 0;
            alarmLed = !alarmLed;
        }

    }else{ // when alarm is deactivated 

    alarmLed = 0;
    Buzzer.input();
    TimeAlarm = 0;
    codePromptDisplayed = false;
    }
    }
}

bool areEqual()
{
    int i;

    for (i = 0; i < Number_Key; i++) {
        if (codeSequence[i] != keyPressed[i]) { //compares the set code to the uses inputs
            return false; // incrrect code is then incorrect
        }
    }

    return true; // all numbers are correct
}

void AlarmDeactivation()
{
    char keyReleased = matrixKeypadUpdate(); // reads the button press on keypad

    if (keyReleased != '\0' && keyReleased != '#' && keyReleased != '*'){ // check  to see if there is a valid key

        keyPressed[MatrixKeypadIndex] = keyReleased; // saves the input to the array

        if (MatrixKeypadIndex < Number_Key - 1){ // moves the key to the next slot 
            MatrixKeypadIndex++;
        }
    }

    if (keyReleased == '*'){ // enter key for passcode
        if (alarmState == ON){

            if (areEqual()) {
                alarmState = OFF;
                uartUBS.write("\r\nAlarm Deactivatied\r\n\n", 24);
            }else{ 

                uartUBS.write("\r\nIncorrect code try again\r\n", 28);
            }
            MatrixKeypadIndex = 0;
        }
    }

    if (keyReleased == '#'){ // displays the log
        displayEventLog();

        MatrixKeypadIndex = 0;
    }
}

void logThresholdEvent(const char* triggerName, float currentTemp, float currentGas)
{
    strcpy(arrayOfStoredEvents[eventsIndex].typeOfEvent, triggerName); //saves the sensor values
    arrayOfStoredEvents[eventsIndex].tempValue = currentTemp;
    arrayOfStoredEvents[eventsIndex].gasValue = currentGas;

    if (eventsIndex < EVENT_MAX_STORAGE - 1) { // stores 5 values
        eventsIndex++;
    } else {
        eventsIndex = 0; //stores data on to old events
    }
}

void displayEventLog()
{
    char str[150];
    uartUBS.write("\r\n LAST 5 ALARM EVENTS \r\n", 25);
    bool hasEvents = false;

    for (int i = 0; i < EVENT_MAX_STORAGE; i++) {
        if (arrayOfStoredEvents[i].typeOfEvent[0] != '\0') { 
            hasEvents = true;
            sprintf(str, "[%s] Temp: %.1f C & Gas: %.0f ppm", 
                arrayOfStoredEvents[i].typeOfEvent,
                arrayOfStoredEvents[i].tempValue,
                arrayOfStoredEvents[i].gasValue );

            uartUBS.write(str, strlen(str));
        }
    }
    if (hasEvents == false) uartUBS.write("No events recorded\r\n", 25);
    uartUBS.write("---------------------------\r\n\n", 30);
}

void matrixKeypadInit() {
    matrixKeypadState = MATRIX_KEYPAD_SCANNING;
    for(int i=0; i<Keypad_Number_Col; i++) keypadColPins[i].mode(PullUp);
}

char matrixKeypadScan() {
    for(int row=0; row<Keypad_Number_Row; row++) {
        for(int i=0; i<Keypad_Number_Row; i++) keypadRowPins[i] = ON;
        keypadRowPins[row] = OFF;
        for(int col=0; col<Keypad_Number_Col; col++) {
            if(keypadColPins[col] == OFF) return matrixKeypadIndexToCharArray[row*Keypad_Number_Row + col];
        }
    }
    return '\0';
}

char matrixKeypadUpdate() {
    char keyDetected = '\0';
    char keyReleased = '\0';

    switch(matrixKeypadState) {
    case MATRIX_KEYPAD_SCANNING:
        keyDetected = matrixKeypadScan();
        if(keyDetected != '\0') {
            matrixKeypadLastKeyPressed = keyDetected;
            accumulatedDebounceMatrixKeypadTime = 0;
            matrixKeypadState = MATRIX_KEYPAD_DEBOUNCE;
        }
        break;
    case MATRIX_KEYPAD_DEBOUNCE:
        if(accumulatedDebounceMatrixKeypadTime >= 40) { // 40ms debounce
            keyDetected = matrixKeypadScan();
            if(keyDetected == matrixKeypadLastKeyPressed) matrixKeypadState = MATRIX_KEYPAD_KEY_HOLD_PRESSED;
            else matrixKeypadState = MATRIX_KEYPAD_SCANNING;
        }
        accumulatedDebounceMatrixKeypadTime += 10;
        break;
    case MATRIX_KEYPAD_KEY_HOLD_PRESSED:
        keyDetected = matrixKeypadScan();
        if(keyDetected != matrixKeypadLastKeyPressed) {
            if(keyDetected == '\0') keyReleased = matrixKeypadLastKeyPressed;
            matrixKeypadState = MATRIX_KEYPAD_SCANNING;
        }
        break;
    default:
        matrixKeypadInit();
        break;
    }
    return keyReleased;
}
