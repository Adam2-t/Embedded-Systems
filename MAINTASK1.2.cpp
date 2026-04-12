#include "mbed.h"

// Blinking rate in milliseconds
#define BLINKING_RATE1     500ms
#define BLINKING_RATE2     250ms
#define BLINKING_RATE3     1000ms

int main()
{
    
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

    led2 = !led2;
    ThisThread::sleep_for(BLINKING_RATE2);

    led3 = !led3;
    ThisThread::sleep_for(BLINKING_RATE3); 
    }
}
