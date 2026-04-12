#include "mbed.h"


UnbufferedSerial uartUsb(USBTX, USBRX, 115200);
Ticker monitorTicker;

bool gasAlarm = false;
bool tempAlarm = false;
bool monitorModeActive = false;

void uartTask(); // forward declerations
void monitorCallBack();
void printMessage();

int main()
{
    printMessage(); //used to print messages in terminal

    while(true) {
        uartTask(); // switch case in void
    }
}

void uartTask()
{
    char receivedChar = '\0';

    if (uartUsb.readable()) {
        uartUsb.read(&receivedChar, 1);

        switch (receivedChar) {

            case '1' :
            gasAlarm = !gasAlarm;

            if (gasAlarm == true) {
            uartUsb.write("START GAS SIM\r\n", 15);
            }else{
                uartUsb.write("END GAS SIM\r\n", 13);
            }
            break;

            case '2' :

                if(gasAlarm) {
                    uartUsb.write("GAS ALARM ACTIVE\r\n", 18);
                }else{
                    uartUsb.write("GAS ALARM DISABLED\r\n", 20);
                }
                break;
                
            case '3' :

                if(tempAlarm) {
                    uartUsb.write("TEMP ALARM ACTIVE\r\n", 19);
                }else {
                uartUsb.write("TEMP ALARM DISABLED\r\n", 21);
                }
                break;
                
            case '4' :

                tempAlarm = !tempAlarm;
                if (tempAlarm == true){
                    uartUsb.write("START TEMP SIM\r\n", 16);
                }else{
                    uartUsb.write("END TEMP SIM\r\n", 14);
                }
                break;
                
            case '5' :

                gasAlarm = false;
                tempAlarm = false;
                uartUsb.write("ALL ALARMS RESET\r\n", 18);
                break;

            case '6' :

                monitorModeActive = !monitorModeActive;
                if (monitorModeActive == true){
                    uartUsb.write("MONITORING ON\r\n", 15);
                    monitorTicker.attach(&monitorCallBack, 2.0f);
                }else{
                    uartUsb.write("MONITORING OFF\r\n", 16);
                    monitorTicker.detach();
                }
                break;
            
            default: 
                uartUsb.write("USE VALID KEYS FROM 1 TO 6\r\n", 28);
                break;
        }
    }
}

void monitorCallBack()
{
    uartUsb.write("--------------------\r\n", 22);

    if (gasAlarm == true) {
        uartUsb.write("GAS ON\r\n", 8);
    }else{
        uartUsb.write("GAS OFF\r\n", 9);
    }

    if (tempAlarm == true) {
        uartUsb.write("TEMP ON\r\n", 9);
    }else{
        uartUsb.write("TEMP OFF\r\n", 10);
    }

    uartUsb.write("--------------------\r\n", 22);
}

void printMessage()
{
    uartUsb.write("   Alarm system\r\n", 17);
    uartUsb.write("1 - GAS SIM ON/OFF\r\n", 20);
    uartUsb.write("2 - REPORT GAS ALARM\r\n", 22);
    uartUsb.write("3 - REPORT TEMP ALARM\r\n", 23);
    uartUsb.write("4 - TEMP SIM ON/OFF\r\n", 21);
    uartUsb.write("5 - RESET SIMS\r\n", 16);
    uartUsb.write("6 - MONITORING MODE\r\n", 21);
    uartUsb.write("--------------------\r\n", 22);
}
