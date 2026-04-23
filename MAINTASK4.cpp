#include "mbed.h"
#include "arm_book_lib.h"

UnbufferedSerial uartUsb(USBTX, USBRX, 115200);//serial monitor

AnalogIn potentiometer(A0);
AnalogIn lm35(A1);
DigitalInOut Buzzer(PE_10);
DigitalIn mq2(PE_12);
DigitalOut alarmLed(LED1);

float lm35TempC   = 0.0;//float values
float lm35Reading = 0.0;
float POT_Reading = 0.0;


const float POT_MIN = 0.0f;
const float POT_MAX = 100.0f;

void Commands();
void uartTask();
void pcSerialComStringWrite(const char* str);
char pcSerialComCharRead();
float lm35Cel_Formula(float analogReading);
float readPotentiometerThreshold();

int main()
{
    Buzzer.mode(OpenDrain);
    Buzzer.input();
    mq2.mode(PullUp);
    alarmLed = OFF;

    
    pcSerialComStringWrite("Press any key to start\r\n");// when terminal is open and key is pressed
    while (pcSerialComCharRead() == '\0') {
        delay(100);
    }

    Commands();

    while (true) {
        uartTask();
    }
}


void Commands()
{
    pcSerialComStringWrite("\r\n       SENSOR MENU\r\n"); // menu for sensors
    pcSerialComStringWrite(" Press 'a' - View live sensor readings\r\n");
    pcSerialComStringWrite(" Press 'b' - Run alarm system\r\n");
}

void uartTask()
{
    char receivedChar = '\0';
    char str[120] = "";
    receivedChar = pcSerialComCharRead();

    if (receivedChar != '\0') {
        switch (receivedChar) {

        case 'a': // displays the sensor readings continously
            pcSerialComStringWrite("Displaying readings Press 'q' to stop\r\n");
            while (!(receivedChar == 'q' || receivedChar == 'Q')) {

                pcSerialComStringWrite("-----------------------------\r\n");
                POT_Reading = potentiometer.read();
                str[0] = '\0';
                sprintf(str, "Potentiometer: %.2f (Threshold: %.1f C)\r\n", // displays the POT values
                        POT_Reading, readPotentiometerThreshold());
                pcSerialComStringWrite(str);
                pcSerialComStringWrite("\r\n");
                
                lm35Reading = lm35.read();
                lm35TempC = lm35Cel_Formula(lm35Reading); // turns temp reading to celsius
                str[0] = '\0';
                sprintf(str, "Temperature: %.2f C\r\n", lm35TempC);
                pcSerialComStringWrite(str);
                pcSerialComStringWrite("\r\n");
                if (!mq2) {
                    pcSerialComStringWrite("Gas: Detected\r\n");
                } else {
                    pcSerialComStringWrite("Gas: Not Detected\r\n");
                }
                
                pcSerialComStringWrite("-----------------------------\r\n");

                delay(500); //serial monitor delay
                receivedChar = pcSerialComCharRead();
            }
            pcSerialComStringWrite("Back to menu\r\n");
            Commands();
            break;


        case 'b': // display the alarm system
            pcSerialComStringWrite("Alarm system ON Press 'q' to stop\r\n");
            while (!(receivedChar == 'q' || receivedChar == 'Q')) {

              
                lm35Reading = lm35.read();  // Read sensors
                lm35TempC   = lm35Cel_Formula(lm35Reading);
                float threshold   = readPotentiometerThreshold();
                bool  gasDetected = (!mq2);

                bool temperatureWarning = (lm35TempC > threshold);
                bool gasWarning         = gasDetected;

                if (temperatureWarning || gasWarning) {
                    
                    Buzzer.output();// Activate buzzer and LED
                    Buzzer = HIGH;
                    alarmLed = ON;

                    if (temperatureWarning && gasWarning) {
                        sprintf(str,"Buzzer ON - Cause: Temperature and Gas | Temp: %.1f C |GAS: Detected | Threshold: %.1f C\r\n",lm35TempC, threshold);
                    } else if (temperatureWarning) {
                        sprintf(str,"Buzzer ON - Cause: Temperature | Temp: %.1f C > Threshold: %.1f C\r\n",lm35TempC, threshold);
                    } else {
                        sprintf(str,"Buzzer ON - Cause: Gas | Gas: Detected | Threshold: %.1f C\r\n", threshold);
                    }
                    pcSerialComStringWrite(str);
                } else {
                    
                    Buzzer.input(); // alarms off
                    alarmLed = OFF;

                    sprintf(str,"System Normal | Temp: %.1f C | Gas: Not Detected | Threshold: %.1f C\r\n", lm35TempC, threshold);
                    pcSerialComStringWrite(str);
                }

                delay(500);
                receivedChar = pcSerialComCharRead();
            }

            Buzzer.input();            // truns off buzzer
            alarmLed = OFF;
            pcSerialComStringWrite("Back to menu\r\n");
            Commands();
            break;

        default:
            Commands();
            break;
        }
    }
}

float lm35Cel_Formula(float analogReading) // celcius formula
{
    return analogReading * 330.0;
}

float readPotentiometerThreshold() // POT thershold
{
    float potValue = potentiometer.read();
    return POT_MIN + potValue * (POT_MAX - POT_MIN);
}

void pcSerialComStringWrite(const char* str)
{
    while (*str) {
        uartUsb.write(str++, 1);
    }
}

char pcSerialComCharRead()
{
    char receivedChar = '\0';
    if (uartUsb.readable()) {
        uartUsb.read(&receivedChar, 1);
    }
    return receivedChar;
}
