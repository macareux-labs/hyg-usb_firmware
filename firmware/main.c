#include <xc.h>
#include <string.h>

#include "types.h"
#include "main.h"
#include "leds.h"
#include "usb.h"
#include "i2c.h"
#include "si7021A10.h"

#include "descriptors.h"

// CONFIG1

#pragma config FOSC = INTOSC	// * Oscillator Selection Bits
#pragma config WDTE = OFF	// Watchdog Timer Enable
#pragma config PWRTE = OFF	// Power-up Timer Enable
#pragma config MCLRE = ON	// MCLR Pin Function Select
#pragma config CP = OFF		// Flash Program Memory Code Protection
#pragma config BOREN = ON	// Brown-out Reset Enable
#pragma config CLKOUTEN = OFF	// Clock Out Enable
#pragma config IESO = OFF	// Internal/External Switchover Mode
#pragma config FCMEN = OFF	// Fail-Safe Clock Monitor Enable

// CONFIG2

#pragma config LVP = OFF	// Low-Voltage Programming Enable (Low-voltage programming enabled)
#pragma config LPBOR = ON	// Low-Power Brown Out Reset (Low-Power BOR is disabled)
#pragma config BORV = LO	// Brown-out Reset Voltage Selection (Brown-out Reset Voltage (Vbor), low trip point selected.)
#pragma config STVREN = ON	// Stack Overflow/Underflow Reset Enable (Stack Overflow or Underflow will cause a Reset)
#pragma config PLLEN = ENABLED	// * PLL Enable Bit
#pragma config PLLMULT = 3x	// * PLL Multipler Selection Bit
#pragma config CPUDIV = CLKDIV3	// * CPU System Clock Selection Bit
#pragma config WRT = OFF	// Flash Memory Self-Write Protection (Write protection off)

#if defined(_USB_LOWSPEED)
#pragma config USBLSCLK = 24MHz	// * USB Low SPeed Clock Selection bit
#else
#pragma config USBLSCLK = 48MHz
#endif

void interrupt interrupt_handler (  ) {
	usb_interrupt_handler (  );
}

__INTERNAL_DEVSTATE __dev_state;

char mappingHex[16] =
	{ '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd',
	'e', 'f'
};

void main ( void ) {

	unsigned char i, j, k;
	si7021serial serial;

	// Set up clock for 8MHz

#if defined(_USB_LOWSPEED)
	OSCCONbits.IRCF = 0b1110;	/* 0v1110 = 8Mhz HFINTOSC */
#else
	OSCCONbits.IRCF = 0b1111;	/* 0b1111 = 16MHz HFINTOSC postscalar */

	/* Enable Active clock-tuning from the USB */
	ACTCONbits.ACTSRC = 1;	/* 1=USB */
	ACTCONbits.ACTEN = 1;
#endif

	// setup sub systems
	setup_leds (  );

	set_red_led ( 1 );

	for ( j = 0; j < 255; j++ )
		for ( k = 0; k < 255; k++ )
			NOP (  );

	// Activate LDO to supply power to the sensor ( RC5 )
	TRISCbits.TRISC5 = 0;
	PORTCbits.RC5 = 1;

	// initialize measures
	__dev_state.hyg.value = 0;
	__dev_state.temp.value = 0;

	__dev_state.green_led = LED_AUTO;
	__dev_state.red_led = LED_OFF;
	__dev_state.yellow_led = LED_OFF;

	__dev_state.crc = 0;

	set_yellow_led ( 1 );

	for ( j = 0; j < 255; j++ )
		for ( k = 0; k < 255; k++ )
			NOP (  );

	// Init I2C
	OpenI2C ( MASTER, SLEW_OFF );

	// Fetch sensor serial number
	get_si7021_serial ( &serial );

	set_green_led ( 1 );

	for ( j = 0; j < 255; j++ )
		for ( k = 0; k < 255; k++ )
			NOP (  );

	// convert serial to string for usb configuration
	j = 0;
	for ( i = 0; i < 8; i++ ) {
		StringDescriptor3.string[j++] =
			mappingHex[serial.bytes[i] >> 4];
		StringDescriptor3.string[j++] =
			mappingHex[serial.bytes[i] & 0x0f];
	}

	set_green_led ( 0 );

	for ( j = 0; j < 255; j++ )
		for ( k = 0; k < 255; k++ )
			NOP (  );

	// Start USB
	INTCONbits.PEIE = 1;	// Enable Peripheral interrupts
	INTCONbits.GIE = 1;	// General interrupt enabled
	PIE2bits.USBIE = 1;	// Enable USB interrupt

	reset_usb (  );

	// Set GPIO
	ANSELA = 0x00;

	set_yellow_led ( 0 );

	for ( j = 0; j < 255; j++ )
		for ( k = 0; k < 255; k++ )
			NOP (  );

	set_red_led ( 0 );

	// Measurement loop
	while ( 1 ) {

		if (++k > 254)
			if (++j > 254)
				if (++i > 2) {


					INTCONbits.PEIE = 0 ;    // Disable Peripheral interrupts
					INTCONbits.GIE = 0 ;     // Disable General interrupt
					PIE2bits.USBIE = 0 ;     // Disable USB interrupt

					// Measure data
					/*measure_rh   ( &__dev_state.hyg )    ;
					  measure_temp ( &__dev_state.temp ) ; */
					measure_all ( &__dev_state.hyg, &__dev_state.temp ) ;

					if ( __dev_state.green_led == LED_AUTO )
						toggle_green_led (  ) ;
					else
						set_green_led ( __dev_state.green_led );

					if ( __dev_state.red_led == LED_AUTO )
						toggle_red_led (  ) ;
					else
						set_red_led ( __dev_state.red_led );

					if ( __dev_state.yellow_led == LED_AUTO )
						toggle_yellow_led (  ) ;
					else
						set_yellow_led ( __dev_state.yellow_led );

					INTCONbits.PEIE = 1 ;    // Enable Peripheral interrupts
					INTCONbits.GIE = 1 ;     // Enable General interrupt
					PIE2bits.USBIE = 1 ;     // Enable USB interrupt

					i = 0 ;
				}

		if ( usb_out_endpoint_busy ( 1 ) == 0 ) {

			// Process Inbound data
			uint8_t *buf = usb_get_out_buffer ( 1 ) ;
			if ( buf[0] == 65 )
				__dev_state.green_led = LED_ON ;

			if ( buf[0] == 66 )
				__dev_state.green_led = LED_OFF ;

			if ( buf[0] == 67 ) {
				set_all_leds ( 0 ) ;
				__dev_state.green_led = LED_AUTO ;
			}

			if ( buf[1] == 65 )
				__dev_state.yellow_led = LED_ON ;

			if ( buf[1] == 66 )
				__dev_state.yellow_led = LED_OFF ;

			if ( buf[1] == 67 ) {
				set_all_leds ( 0 ) ;
				__dev_state.yellow_led = LED_AUTO ;
			}

			if ( buf[2] == 65 )
				__dev_state.red_led = LED_ON ;

			if ( buf[2] == 66 )
				__dev_state.red_led = LED_OFF ;

			if ( buf[2] == 67 ) {
				set_all_leds ( 0 ) ;
				__dev_state.red_led = LED_AUTO ;
			}

			/* Update CRC */
			__dev_state.crc =
				__dev_state.hyg.msb ^ __dev_state.
				hyg.lsb ^ __dev_state.temp.msb ^ __dev_state.temp.
				lsb ^ __dev_state.green_led ^ __dev_state.
				yellow_led ^ __dev_state.red_led ;

			/* Wait for endpoint available */
			while ( usb_in_endpoint_busy ( 1 ) == 1 ) ;

			usb_arm_in_transfert ( 1, sizeof ( __dev_state ), &__dev_state ) ;

			usb_arm_out_transfert ( ) ;

		}

	}

}

/* Local Variables:    */
/* mode: c             */
/* c-file-style: "k&r" */
/* c-basic-offset: 8   */
/* indent-tabs-mode: t */
/* End:                */
