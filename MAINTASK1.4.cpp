#include "mbed.h"

// Blinking rate in milliseconds
#define BLINKING_RATE1     1000ms

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

    
for(int i = 0; i <10; i++){ // For loop to make the Leds flash 5 times
     led = !led;
    led2 = !led2;
    led3 = !led3;
    ThisThread::sleep_for(BLINKING_RATE1);
    } 

    led = 1;
    led2 = 0;
    led3 = 0;
   
    }
