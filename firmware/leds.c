#include <xc.h>
#include "leds.h"

// Setup RC2 RC3 RC4 for led operations
void setup_leds (  ) {

	ANSELC = 0x00;		// Disable analog use of port C
	TRISCbits.TRISC4 = 0 ;	// Set ports C pins as output
	TRISCbits.TRISC2 = 0 ;
	TRISCbits.TRISC3 = 0 ;

	PORTCbits.RC2 = 0 ;	// Set ports C pins off
	PORTCbits.RC4 = 0 ;
	PORTCbits.RC3 = 0 ;
}

void set_green_led ( unsigned s ) {
	PORTCbits.RC4 = s ;
}

void set_yellow_led ( unsigned s ) {
	PORTCbits.RC3 = s ;
}

void set_red_led ( unsigned s ) {
	PORTCbits.RC2 = s ;
}

void toggle_green_led (  ) {
	PORTCbits.RC4 = !PORTCbits.RC4 ;
}

void toggle_yellow_led (  ) {
	PORTCbits.RC3 = !PORTCbits.RC3 ;
}

void toggle_red_led (  ) {
	PORTCbits.RC2 = !PORTCbits.RC2 ;
}

void set_all_leds ( unsigned s ) {
	PORTCbits.RC4 = s ;
	PORTCbits.RC2 = s ;
	PORTCbits.RC3 = s ;
}

/* Local Variables:    */
/* mode: c             */
/* c-basic-offset: 8   */
/* indent-tabs-mode: t */
/* End:                */
