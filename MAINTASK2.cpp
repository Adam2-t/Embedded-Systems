#include "mbed.h"
#include "arm_book_lib.h"

enum systemState{ // enum used for switch case states
    passState,
    warningState,
    LockState
};

int main()
{
    DigitalIn enterButton(BUTTON1);// button dec
    DigitalIn aButton(D2);
    DigitalIn bButton(D3);
    DigitalIn cButton(D4);
    DigitalIn dButton(D5);
    DigitalIn eButton(D6);
    DigitalIn fButton(D7);

    DigitalOut alarmLed(LED1);
    DigitalOut incorrectCodeLed(LED3);
    DigitalOut lockdownLed(LED2);

    aButton.mode(PullDown);
    bButton.mode(PullDown);
    cButton.mode(PullDown);
    dButton.mode(PullDown);
    eButton.mode(PullDown);
    fButton.mode(PullDown);

    alarmLed = ON;
    incorrectCodeLed = OFF;
    lockdownLed = OFF;

    Timer warningTimer; // timers
    Timer blinkTimer;
    Timer lockdownTimer;

    
    bool warningActive = 0;
    int wrongAttempts = 0;

    bool lockdownActive = 0; 
    int lockdownCounter = 0;

    systemState currentState = passState;

    while (true) {
        switch (currentState){
            case passState:
                if ( aButton && bButton && cButton && dButton) {
                    wrongAttempts = 0;
                    alarmLed = OFF;
            
                } else if (enterButton){
                    wrongAttempts++;

                    if (wrongAttempts == 3){
                    currentState = warningState;

                    } else if (wrongAttempts == 4){
                    currentState = LockState;
                    } 
                }
                    
                ThisThread::sleep_for(200ms);
                break;
            
            case warningState:
                if (!warningActive){
                    warningTimer.start();
                    blinkTimer.start();
                    warningActive = 1;
                }

                if (blinkTimer.read_ms()>= 500){
                    incorrectCodeLed = !incorrectCodeLed;
                    blinkTimer.reset();
                }

                if(warningTimer.read() >=30.0f){
                    incorrectCodeLed = 0;
                    warningActive = false;
                    warningTimer.stop();
                    warningTimer.reset();
                    currentState = passState;
                }break;

            case LockState:
                if (!lockdownActive){
                    lockdownCounter++;
                    lockdownLed = ON;
                    lockdownTimer.start();
                    blinkTimer.start();
                    lockdownActive = true;
                }

                if (lockdownTimer.read() <= 60.0f) {

                    if(blinkTimer.read_ms() >= 250){
                    alarmLed = !alarmLed;
                    blinkTimer.reset();
                    }
                }else{
                    alarmLed = OFF;
                }

                if (cButton && dButton && eButton && fButton){
                lockdownLed = OFF;
                alarmLed = OFF;
                incorrectCodeLed = OFF;

                lockdownActive = false;
                lockdownTimer.stop();
                lockdownTimer.reset();

                wrongAttempts = 0;
                currentState = passState;
                }break;
        }
    }
}
