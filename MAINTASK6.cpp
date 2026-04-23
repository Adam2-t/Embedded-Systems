#include "mbed.h"
#include "arm_book_lib.h"
#include "display.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

AnalogIn pot(A0);
AnalogIn lm35(A1);
AnalogIn mq2(A2);

DigitalOut   alarmLed(LED1);
DigitalInOut buzzer(PE_10);

UnbufferedSerial uartUBS(USBTX, USBRX, 115200);

#define KEYPAD_NUMBER_ROW  4
#define KEYPAD_NUMBER_COL  4

DigitalOut keypadRowPins[KEYPAD_NUMBER_ROW] = {PB_3, PB_5, PC_7, PA_15};
DigitalIn  keypadColPins[KEYPAD_NUMBER_COL] = {PB_12, PB_13, PB_15, PC_6};

char matrixKeypadIndexToCharArray[] = {
    '1', '2', '3', 'A',
    '4', '5', '6', 'B',
    '7', '8', '9', 'C',
    '*', '0', '#', 'D',
};

typedef enum {
    MATRIX_KEYPAD_SCANNING,
    MATRIX_KEYPAD_DEBOUNCE,
    MATRIX_KEYPAD_KEY_HOLD_PRESSED
} matrixKeypadState_t;

matrixKeypadState_t matrixKeypadState;
int  accumulatedDebounceTime     = 0;
char matrixKeypadLastKeyPressed  = '\0';

#define CODE_NUMBER_OF_KEYS  5

char codeSequence[CODE_NUMBER_OF_KEYS] = {'1', '2', '3', '6', '7'};
char keyPressed[CODE_NUMBER_OF_KEYS]   = {'0', '0', '0', '0', '0'};
int  codeIndex = 0;

#define DISPLAY_REFRESH_TIME_MS    1000
#define ALARM_REPORT_TIME_MS       60000

#define TEMPERATURE_WARNING_LIMIT  40.0f
#define GAS_WARNING_LIMIT          500.0f

float lm35Temp        = 0.0f;
float gasPPM          = 0.0f;
float tempThreshold   = 0.0f;
float gasThreshold    = 0.0f;
float oldTempThreshold = 0.0f;

int  timeAlarm = 0;

bool alarmState          = false;
bool codePromptDisplayed = false;

#define EVENT_MAX_STORAGE  5

typedef struct {
    time_t seconds;
    char   typeOfEvent[16];
    float  tempValue;
    float  gasValue;
} systemEvent_t;

systemEvent_t arrayOfStoredEvents[EVENT_MAX_STORAGE];
int eventsIndex = 0;

void  systemInit();
void  updateThresholds();
void  alarmActivation();
void  alarmDeactivation();
bool  areCodesEqual();
float convertToTempCelsius(float rawReading);
void  logThresholdEvent(const char* triggerName, float currentTemp, float currentGas);
void  displayEventLog();
void  matrixKeypadInit();
char  matrixKeypadScan();
char  matrixKeypadUpdate();
void  lcdDisplayInit();
void  lcdDisplayUpdate();


int main()
{
    systemInit();
    uartUBS.write("System Ready set alarm thresholds.\r\n", 51);

    while (true) {
        updateThresholds();
        alarmActivation();
        alarmDeactivation();
        lcdDisplayUpdate();
        delay(10);  // 10 ms main loop tick
    }
}

void systemInit()
{
    alarmLed = 0;
    buzzer.mode(OpenDrain);  //  prevents a short circuit
    buzzer.input();          //  buzzer silent at begininng

    matrixKeypadInit();

    
    for (int i = 0; i < EVENT_MAX_STORAGE; i++) { // event storage is empty
        arrayOfStoredEvents[i].seconds = 0;
    }

    lcdDisplayInit();
}

void updateThresholds()
{
    float rawValue = pot.read();

    tempThreshold = 25.0f + (rawValue * 12.0f);  //  25–37 °C
    gasThreshold  = rawValue * 800.0f;            // 0–800 PPM

   
    if (abs(tempThreshold - oldTempThreshold) > 0.2f) {   // print when threshold changes by 0.2
        char str[100];
        sprintf(str, "Threshold updated Temp=%.1f C | Gas=%.0f ppm\r\n", tempThreshold, gasThreshold);
        uartUBS.write(str, strlen(str));
        oldTempThreshold = tempThreshold;
    }
}

float convertToTempCelsius(float rawReading)
{
    return (rawReading * 3.3f / 0.01f);  // Celsius conversion
}

void alarmActivation()
{
    lm35Temp = convertToTempCelsius(lm35.read());
    gasPPM   = mq2.read() * 1000.0f;

    bool tempTooHigh = (lm35Temp > tempThreshold);
    bool gasTooHigh  = (gasPPM  > gasThreshold);

    if (tempTooHigh || gasTooHigh) {
        alarmState = true;
    }

    if (alarmState == true) {

        
        if (codePromptDisplayed == false) { // log event when alarm active
            uartUBS.write("\r\n ALARM TRIGGERED Enter 5-Digit Code to Deactivate:\r\n", 52);

            displayCharPositionWrite(6, 2);
            displayStringWrite("ON ");

            if (tempTooHigh) {
                logThresholdEvent("OVER_TEMP", lm35Temp, gasPPM);
            } else if (gasTooHigh) {
                logThresholdEvent("HIGH_GAS",  lm35Temp, gasPPM);
            }

            codePromptDisplayed = true;
        }

        buzzer.output();
        buzzer = LOW;  

        timeAlarm += 10;
        if (timeAlarm >= 100) {  
            timeAlarm = 0;
            alarmLed = !alarmLed;
        }

    } else {
        alarmLed = 0;
        buzzer.input();
        timeAlarm = 0;
        codePromptDisplayed = false;
    }
}

bool areCodesEqual()
{
    for (int i = 0; i < CODE_NUMBER_OF_KEYS; i++) {
        if (codeSequence[i] != keyPressed[i]) {
            return false;
        }
    }
    return true;
}

void alarmDeactivation()
{
    char keyReleased = matrixKeypadUpdate();

    if (keyReleased == '\0') return;

    if (keyReleased == '4') {
        displayCharPositionWrite(0, 3);
        if (gasPPM > GAS_WARNING_LIMIT) {
            displayStringWrite("Gas: DETECTED   ");
        } else {
            displayStringWrite("Gas: Not Detectd");
        }
    }

    if (keyReleased == '5') {
        displayCharPositionWrite(0, 3);
        if (lm35Temp > TEMPERATURE_WARNING_LIMIT) {
            displayStringWrite("OvrTmp: DETECTED");
        } else {
            displayStringWrite("OvrTmp: Normal  ");
        }
    }

    
    if (keyReleased != '*' && keyReleased != '#' && keyReleased != '4' && keyReleased != '5') { // special keys
        keyPressed[codeIndex] = keyReleased;
        if (codeIndex < CODE_NUMBER_OF_KEYS - 1) {
            codeIndex++;
        }
    }

    if (keyReleased == '*') {
        if (alarmState == true) {
            if (areCodesEqual()) {
                alarmState = false;
                uartUBS.write("\r\nAlarm Deactivated.\r\n", 22);

                
                displayCharPositionWrite(6, 2); // update LCD 
                displayStringWrite("OFF");

                displayCharPositionWrite(0, 3);
                displayStringWrite("Alarm OFF!      ");
            } else {
                uartUBS.write("\r\nIncorrect code. Try again.\r\n", 30);

                displayCharPositionWrite(0, 3);
                displayStringWrite("Wrong Code!     ");
            }
        }
        codeIndex = 0;
    }

    if (keyReleased == '#') {
        displayEventLog();
        codeIndex = 0;
    }
}

void logThresholdEvent(const char* triggerName, float currentTemp, float currentGas)
{
    arrayOfStoredEvents[eventsIndex].seconds = time(NULL);
    strcpy(arrayOfStoredEvents[eventsIndex].typeOfEvent, triggerName);
    arrayOfStoredEvents[eventsIndex].tempValue = currentTemp;
    arrayOfStoredEvents[eventsIndex].gasValue  = currentGas;

    
    if (eventsIndex < EVENT_MAX_STORAGE - 1) { //replace the previous saved data
        eventsIndex++;
    } else {
        eventsIndex = 0;
    }
}

void displayEventLog()
{
    char str[150];
    uartUBS.write("\r\n LAST 5 ALARM EVENTS \r\n", 31);
    bool hasEvents = false;

    for (int i = 0; i < EVENT_MAX_STORAGE; i++) {
        if (arrayOfStoredEvents[i].seconds != 0) {  // Skip empty slots
            hasEvents = true;
            sprintf(str, "[%s] Temp: %.1f C | Gas: %.0f ppm | %s",
                arrayOfStoredEvents[i].typeOfEvent,
                arrayOfStoredEvents[i].tempValue,
                arrayOfStoredEvents[i].gasValue,
                ctime(&arrayOfStoredEvents[i].seconds)
            );
            uartUBS.write(str, strlen(str));
        }
    }

    if (!hasEvents) {
        uartUBS.write("No events recorded yet.\r\n", 25);
    }
    uartUBS.write("---------------------------\r\n\n", 30);
}

void lcdDisplayInit()
{
    displayInit(DISPLAY_CONNECTION_I2C_PCF8574_IO_EXPANDER);

    displayCharPositionWrite(0, 0);
    displayStringWrite("Temperature:");

    displayCharPositionWrite(0, 1);
    displayStringWrite("Gas:");

    displayCharPositionWrite(0, 2);
    displayStringWrite("Alarm:");

    displayCharPositionWrite(6, 2);
    displayStringWrite("OFF");  // Shows alarm state 

    displayCharPositionWrite(0, 3);
    displayStringWrite("Enter Code:Deact");
}

void lcdDisplayUpdate()
{
    static int accumulatedDisplayTime    = 0;
    static int accumulatedAlarmReportTime = 0;

    char temperatureString[6] = "";

    if (accumulatedDisplayTime >= DISPLAY_REFRESH_TIME_MS) {
        accumulatedDisplayTime = 0;

        sprintf(temperatureString, "%.0f", lm35Temp);
        displayCharPositionWrite(12, 0);
        displayStringWrite(temperatureString);
        displayCharPositionWrite(14, 0);
        displayStringWrite("'C");

        displayCharPositionWrite(4, 1);
        if (gasPPM > GAS_WARNING_LIMIT) {
            displayStringWrite("Detected    ");
        } else {
            displayStringWrite("Not Detected");
        }

        if (lm35Temp > TEMPERATURE_WARNING_LIMIT) {
            displayCharPositionWrite(0, 3);
            displayStringWrite("!!TEMP WARNING!!");
        } else if (gasPPM > GAS_WARNING_LIMIT) {
            displayCharPositionWrite(0, 3);
            displayStringWrite("!! GAS WARNING!!");
        }

    } else {
        accumulatedDisplayTime += 10;
    }

    if (accumulatedAlarmReportTime >= ALARM_REPORT_TIME_MS) {
        accumulatedAlarmReportTime = 0;

        displayCharPositionWrite(6, 2);
        if (alarmState == true) {
            displayStringWrite("ON ");  
        } else {
            displayStringWrite("OFF");
        }

    } else {
        accumulatedAlarmReportTime += 10;
    }
}

void matrixKeypadInit()
{
    matrixKeypadState = MATRIX_KEYPAD_SCANNING;

    for (int i = 0; i < KEYPAD_NUMBER_COL; i++) {
        keypadColPins[i].mode(PullUp);  // column reads when no key pressed
    }
}

char matrixKeypadScan()
{
    for (int row = 0; row < KEYPAD_NUMBER_ROW; row++) {

        for (int i = 0; i < KEYPAD_NUMBER_ROW; i++) keypadRowPins[i] = ON;

        keypadRowPins[row] = OFF;  

        for (int col = 0; col < KEYPAD_NUMBER_COL; col++) {
            if (keypadColPins[col] == OFF) {
                return matrixKeypadIndexToCharArray[row * KEYPAD_NUMBER_COL + col];
            }
        }
    }
    return '\0';
}

char matrixKeypadUpdate()
{
    char keyDetected = '\0';
    char keyReleased = '\0';

    switch (matrixKeypadState) {

        case MATRIX_KEYPAD_SCANNING:
            keyDetected = matrixKeypadScan();
            if (keyDetected != '\0') {
                matrixKeypadLastKeyPressed = keyDetected;
                accumulatedDebounceTime = 0;
                matrixKeypadState = MATRIX_KEYPAD_DEBOUNCE;
            }
            break;

        case MATRIX_KEYPAD_DEBOUNCE:
            if (accumulatedDebounceTime >= 40) {  // Wait 40 ms 
                keyDetected = matrixKeypadScan();
                if (keyDetected == matrixKeypadLastKeyPressed) {
                    matrixKeypadState = MATRIX_KEYPAD_KEY_HOLD_PRESSED;
                } else {
                    matrixKeypadState = MATRIX_KEYPAD_SCANNING;
                }
            }
            accumulatedDebounceTime += 10;
            break;

        case MATRIX_KEYPAD_KEY_HOLD_PRESSED:
            keyDetected = matrixKeypadScan();
            if (keyDetected != matrixKeypadLastKeyPressed) {
                if (keyDetected == '\0') {
                    keyReleased = matrixKeypadLastKeyPressed;
                }
                matrixKeypadState = MATRIX_KEYPAD_SCANNING;
            }
            break;

        default:
            matrixKeypadInit(); 
            break;
    }

    return keyReleased;
}
