#include "mbed.h"

// Blinking rate in milliseconds
#define BLINKING_RATE1     200ms

int main()
{
    // Initialise the digital pin LED1 as an output
#ifdef LED1
    DigitalOut led(LED1);
    DigitalOut led2(LED2);
    DigitalOut led3(LED3);
#else
    bool led;
    bool led2;
    bool led3;

#endif

while (true) {
    
    led = !led;
    ThisThread::sleep_for(BLINKING_RATE1);
    led = 0;

    led2 = !led2;
    ThisThread::sleep_for(BLINKING_RATE1);
    led2 = 0;

    led3 = !led3;
    ThisThread::sleep_for(BLINKING_RATE1); 
    led3 = 0;

    led2 = !led2;
    ThisThread::sleep_for(BLINKING_RATE1);
    led2 = 0;

    led = !led;
    ThisThread::sleep_for(BLINKING_RATE1);
    led = 0;
    
    }
}
