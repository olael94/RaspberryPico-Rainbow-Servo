#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pwm.h"

//preamble for ADC
#include "hardware/gpio.h" 
#include "hardware/adc.h"

//preamble for ws2812 NEOPIXEL
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "ws2812.pio.h"

//Define colors for the NeoPixel
#define RED 0,0xff,0
#define ORANGE 127,255,000
#define YELLOW 0x80,0x80,0
#define GREEN 0xff,0,0
#define BLUE 00,00,255
#define INDIGO 00,75,130
#define PURPLE 00,148,211
#define NEOPIXEL 19     //Neopixel Pin Definition

const int PIN_TX = 0; //NEOPIXEL PIN DEFINITION

//Send a Color to the NeoPixel
static inline void put_pixel(uint32_t pixel_grb) {
  pio_sm_put_blocking(pio0, 0, pixel_grb << 8u);
}

// Convert RGB values to GRB format used by the NeoPixel
static inline uint32_t urgb_u32(uint8_t r, uint8_t g, uint8_t b) {
  return ((uint32_t)(r) << 8) |
         ((uint32_t)(g) << 16) |
         (uint32_t)(b);
}

//Set PWM Frequency and Duty Cycle
uint32_t pwm_set_freq_duty(uint slice_num, uint chan, uint32_t f, int d) {

    uint32_t clock = 125000000;
    uint32_t divider16 = clock / f / 4096 + (clock % (f * 4096) != 0);
    if (divider16 / 16 == 0)
        divider16 = 16;
    uint32_t wrap = clock * 16 / divider16 / f - 1;
    pwm_set_clkdiv_int_frac(slice_num, divider16 / 16, divider16 & 0xF);
    pwm_set_wrap(slice_num, wrap);
    pwm_set_chan_level(slice_num, chan, wrap * d / 100);

    return wrap;
}

//Get the PWM wrap Value
uint32_t pwm_get_wrap(uint slice_num) {
    valid_params_if(PWM, slice_num >= 0 && slice_num < NUM_PWM_SLICES);

    return pwm_hw->slice[slice_num].top;
}

// Set PWM duty cycle as a percentage
void pwm_set_duty(uint slice_num, uint chan, int d) {
    pwm_set_chan_level(slice_num, chan, pwm_get_wrap(slice_num) * d / 100);
}

// Set PWM duty cycle as a high resolution percentage
void pwm_set_dutyH(uint slice_num, uint chan, int d) {
    pwm_set_chan_level(slice_num, chan, pwm_get_wrap(slice_num) * d / 10000);
}

//Servo struct Definition
typedef struct {
    uint gpio;
    uint slice;
    uint chan;
    uint speed;
    uint resolution;
    bool on;
    bool invert;
} Servo;

//Initialized servo
void ServoInit(Servo *s, uint gpio, bool invert) {
    gpio_set_function(gpio, GPIO_FUNC_PWM);
    s->gpio = gpio;
    s->slice = pwm_gpio_to_slice_num(gpio);
    s->chan = pwm_gpio_to_channel(gpio);

    pwm_set_enabled(s->slice, false);
    s->on = false;
    s->speed = 0;
    s->resolution = pwm_set_freq_duty(s->slice, s->chan, 50, 0);
    pwm_set_dutyH(s->slice, s->chan, 250);

    if (s->chan){
        pwm_set_output_polarity(s->slice, false, invert);
    }
    else {
        pwm_set_output_polarity(s->slice, invert, false);
    }
    s->invert = invert;
}


//Turn the Servo On
void ServoOn(Servo *s) {
    pwm_set_enabled(s->slice, true);
    s->on = true;
}


//Turn the Servo Off
void ServoOff(Servo *s) {
    pwm_set_enabled(s->slice, false);
    s->on = false;
}


//Set the Servo Position
void ServoPosition(Servo *s, uint p) {
    pwm_set_dutyH(s->slice, s->chan, p * 10 + 250);
}


//Map function Implementation in C. got it from https://stackoverflow.com/questions/61000446/map-function-implementation-in-c
//changed long for int32_t
int32_t map(int32_t x, int32_t in_min, int32_t in_max, int32_t out_min, int32_t out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

int main() {
    uint16_t analogInput;
    int32_t servoLocation;

    stdio_init_all();

    Servo s1;
    ServoInit(&s1, 20, false); //SERVO GPIO DEFINE

    ServoOn(&s1);

    // got it from 4.1.1. hardware_adc chrome-extension://efaidnbmnnnibpcajpcglclefindmkaj/https://datasheets.raspberrypi.com/pico/raspberry-pi-pico-c-sdk.pdf
    printf("ADC Example, measuring GPIO26\n");

    adc_init(); 
    // Make sure GPIO is high-impedance, no pullups etc 
    adc_gpio_init(26); 
    // Select ADC input 0 (GPIO26) 
    adc_select_input(0);

    //Initialization for NEOPIXEL
    PIO pio = pio0;
    int sm = 0;
    uint offset = pio_add_program(pio, &ws2812_program);
    char str[12];

    ws2812_program_init(pio, sm, offset, PIN_TX, 800000, false); //we are using a 4 pins instead of 3

    while (true) {
        analogInput = adc_read();                               //Read the ADC value
        servoLocation = map(analogInput, 0, 4095, 0, 100);      // Map ADC value to servo position

        ServoPosition(&s1, servoLocation);                      //Set the Servo Position

        // Set NeoPixel color based on ADC value
        switch(analogInput) {

            case 0 ... 585:
                    put_pixel(urgb_u32(RED));  // RED
                    put_pixel(urgb_u32(RED));  // RED NEOPIXEL2
                    break;
            case 586 ... 1170:
                    put_pixel(urgb_u32(ORANGE));  // ORANGE
                    put_pixel(urgb_u32(ORANGE));  // ORANGE NEOPIXEL2
                    break;
            case 1171 ... 1755:
                    put_pixel(urgb_u32(YELLOW));  // YELLOW
                    put_pixel(urgb_u32(YELLOW));  // YELLOW NEOPIXEL2
                    break;
            
            case 1756 ... 2340:
                    put_pixel(urgb_u32(GREEN));  // GREEN
                    put_pixel(urgb_u32(GREEN));  // GREEN NEOPIXEL2
                    break;
            
            case 2341 ... 2925:
                    put_pixel(urgb_u32(BLUE));  // BLUE
                    put_pixel(urgb_u32(BLUE));  // BLUE NEOPIXEL2
                    break;
            
            case 2926 ... 3510:
                    put_pixel(urgb_u32(INDIGO));  // INDIGO
                    put_pixel(urgb_u32(INDIGO));  // INDIGO NEOPIXEL2
                    break;

            case 3511 ... 4095:
                    put_pixel(urgb_u32(PURPLE));  // PURPLE
                    put_pixel(urgb_u32(PURPLE));  // PURPLE NEOPIXEL2
                    break;
        }//END of Switch Statements

        sleep_ms(40);       //Delay

    }//END of While Loop

    return 0;

}//END of Main() Function