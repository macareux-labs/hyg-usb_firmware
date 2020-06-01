#ifndef LEDS_H
#define LEDS_H

#define LED_ON		 1
#define LED_OFF		 0
#define LED_AUTO	-1

void setup_leds ( void ) ;

void set_green_led ( unsigned ) ;
void set_red_led ( unsigned ) ;
void set_yellow_led ( unsigned ) ;

void set_all_leds ( unsigned ) ;

void toggle_green_led ( void ) ;
void toggle_red_led ( void ) ;
void toggle_yellow_led ( void ) ;

#endif
