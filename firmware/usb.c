#include <xc.h>
#include <string.h>
#include "types.h"
#include "usb.h"
#include "leds.h"
#include "descriptors.h"
#include "main.h"

#define UOWN		0x80	// USB Ownership Bit
#define DTSEN		0x08	// Data Toggle Sync Enable
#define DTS		0x40	// Data Toggle Sync
#define BSTALL          0x04	// Buffer Stall

typedef struct {
	union {
		struct {
			uint8_t bch:2 ;
			uint8_t pid:4 ;
			uint8_t reserved:1 ;
			uint8_t uown:1 ;
		} ;
		struct {
			uint8_t :2 ;
			uint8_t bstall:1 ;
			uint8_t dtsen:1 ;
			uint8_t :2 ;
			uint8_t dts:1 ;
			uint8_t :1 ;
		} ;
		uint8_t BDnSTAT ;
	} ;
	uint8_t BDnCNT ;
	uint16_t BDnADR ;	/* BDnADRL and BDnADRH; */
} __BufferDescriptor ;

typedef struct {
	__BufferDescriptor Output ;	// Host to Device
	__BufferDescriptor Input ;	// Device to Host
} Interface ;

// USB Module Memory Mapping start at 0x2000
volatile __at ( 0x2000 )
Interface Interfaces[2] ;

struct {
	// --- End Point 0
	uint8_t OUT0[EP0_BUFLEN] ;
	uint8_t IN0[EP0_BUFLEN] ;

	// --- End Point 1
	uint8_t OUT1[EP1_BUFLEN] ;
	uint8_t IN1[EP1_BUFLEN] ;
} ep_buffers __at ( 0x2080 ) ;

// Local data
uint8_t *addr_to_send ;
uint16_t cnt_to_send ;

uint8_t addresse ;

// Predecla

void setup_endpoints (  ) {

	// Reset Buffer Descriptors and re-init
	memset ( Interfaces, 0x0, sizeof ( Interfaces ) ) ;

	// * Setup Buffers
	Interfaces[0].Output.BDnADR = PHYS_ADDR ( ep_buffers.OUT0 ) ;
	Interfaces[0].Output.BDnCNT = EP0_BUFLEN ;
	Interfaces[0].Output.BDnSTAT = UOWN | DTSEN ;

	Interfaces[0].Input.BDnADR = PHYS_ADDR ( ep_buffers.IN0 ) ;
	Interfaces[0].Input.BDnCNT = 0 ;
	Interfaces[0].Input.BDnSTAT = 0 ;

	Interfaces[1].Output.BDnADR = PHYS_ADDR ( ep_buffers.OUT1 ) ;
	Interfaces[1].Output.BDnCNT = EP1_BUFLEN ;
	Interfaces[1].Output.BDnSTAT = UOWN | DTSEN ;

	Interfaces[1].Input.BDnADR = PHYS_ADDR ( ep_buffers.IN1 ) ;
	Interfaces[1].Input.BDnCNT = 0 ;
	Interfaces[1].Input.BDnSTAT = 0 ;

}

void reset_usb (  ) {

#if defined(_USB_LOWSPEED)
	UCFG = 0x10 ;		// Enable Pullups : Low speed device ; no ping pong
#else
	UCFG = 0x14 ;		// Enable Pullups : FS device ; no ping pong
#endif

	UADDR = 0 ;		// Reset usb address

	UIE = 0 ;		// Reset USB Interrupts

#if defined(_USB_ERROR_INTERRUPT)
	UIE = 0b00101011 ;	// Enable interrupts
	//UIEbits.URSTIE = 1 ;   // USB Reset Interrupt
	//UIEbits.UERRIE = 1 ;   // USB Error Interrupt
	//UIEbits.TRNIE  = 1 ;   // USB Transaction Complete
	//UIEbits.IDLEIE = 0 ;   // USB IDLE Detect Interrupt
	//UIEbits.STALLIE= 1 ;   // USB STALL Handshake Interrupt
	//UIEbits.SOFIE  = 0 ;   // USB Start Of Frame

	UEIE = 0b100111111 ;	// ErrorInterrupt: BTSEE | BTOEE | DFN8EE | CRC16EE | CRC5EE | PIDEE
#else
	UIE = 0b00101001 ;	// Enable interrupts
	//UIEbits.URSTIE = 1 ;   // USB Reset Interrupt
	//UIEbits.UERRIE = 0 ;   // USB Error Interrupt
	//UIEbits.TRNIE  = 1 ;   // USB Transaction Complete
	//UIEbits.IDLEIE = 0 ;   // USB IDLE Detect Interrupt
	//UIEbits.STALLIE= 1 ;   // USB STALL Handshake Interrupt
	//UIEbits.SOFIE  = 0 ;   // USB Start Of Frame

	UEIE = 0 ;		// USB Error Interrupt: disabled
#endif

	// Clear flags
	UEIR = 0 ;		// Clear all usb error interrupts
	UIR = 0 ;		// Clear all usb status interrupts

	addresse = 0 ;
	cnt_to_send = 0 ;

	/* If the PLL is being used, wait until the
	   PLLRDY bit is set in the OSCSTAT register
	   before attempting to set the USBEN bit. */

	if ( OSCCONbits.SPLLEN ) {
		while ( !OSCSTATbits.PLLRDY ) ;
	}

	/* If USB module not enabled */
	if ( UCONbits.USBEN == 0 ) {
		UCON = 0 ;

		UCONbits.USBEN = 1 ;	// Enable USB module

		// Wait for single-ended zero condition to clear.
		while ( UCONbits.SE0 ) ;
	}

	// Empty FIFO
	UIRbits.TRNIF = 0 ;
	UIRbits.TRNIF = 0 ;
	UIRbits.TRNIF = 0 ;
	UIRbits.TRNIF = 0 ;

	// * Reset ALL Endpoints
	UEP0 = 0 ;
	UEP1 = 0 ;
	UEP2 = 0 ;
	UEP3 = 0 ;
	UEP4 = 0 ;
	UEP5 = 0 ;
	UEP6 = 0 ;
	UEP7 = 0 ;

	// * Configure Endpoint 0 (Main USB Init)
	UEP0 = 0b00010110 ;
	//UEP0bits.EPHSHK  = 1 ; // Enable Handshake
	//UEP0bits.EPOUTEN = 1 ; // Outbound
	//UEP0bits.EPINEN  = 1 ; // Inbound

	// * Configure Endpoint 1 (Application)
	UEP1 = 0b00011110 ;
	//UEP1bits.EPHSHK   = 1 ; // Enable Handshake
	//UEP1bits.EPCONDIS = 1 ; // Enable control operations
	//UEP1bits.EPOUTEN  = 1 ; // Outound
	//UEP1bits.EPINEN   = 1 ; // Inbound

	// Setup Endpoints:
	setup_endpoints (  ) ;

	// Enable Packet transfers
	UCONbits.PKTDIS = 0 ;
}

void fill_in_buffer (  ) {

	uint16_t i ;
	uint16_t byte_count ;

	byte_count = ( cnt_to_send < EP0_BUFLEN ) ? cnt_to_send : EP0_BUFLEN ;

	Interfaces[0].Input.bch = 0 ;
	Interfaces[0].Input.BDnCNT = byte_count ;

	for ( i = 0; i < byte_count; i++ )
		ep_buffers.IN0[i] = addr_to_send[i] ;

	cnt_to_send -= byte_count ;
	addr_to_send += byte_count ;

}

/* Handle inital USB setup from Host */
void process_setup_packets (  ) {
	uint8_t dts ;

	if ( USTATbits.DIR == 0 ) {	// This is an OUT or a SETUP transaction

		uint8_t PID = Interfaces[0].Output.pid ;

		if ( PID == USB_PID_SETUP ) {	// This is a SETUP packet

			// Clear UOWN & STALL (Get memory access)
			Interfaces[0].Input.BDnSTAT = 0 ;
			Interfaces[0].Output.BDnSTAT = 0 ;

			Interfaces[0].Output.BDnCNT = EP0_BUFLEN ;
			Interfaces[0].Input.BDnCNT = 0 ;

			SetupPacketStruct *SetupPacket ;
			SetupPacket = ( SetupPacketStruct * ) ep_buffers.OUT0 ;

			if ( SetupPacket->bmRequestType == 0x80 ) {	// Data will go from Device to host

				if ( SetupPacket->bRequest == USB_REQ_GET_DESCRIPTOR ) {

					switch ( SetupPacket->wValue1 ) {

					case USB_GET_DEVICE_DESCRIPTOR:
						addr_to_send =
							( uint8_t * )
							&DeviceDescriptor ;
						cnt_to_send =
							SetupPacket->wLength ;

						fill_in_buffer (  ) ;
						break ;

					case USB_GET_CONFIG_DESCRIPTOR:
						addr_to_send =
							( uint8_t * )
							ConfigurationDescriptor ;
						cnt_to_send =
							SetupPacket->wLength ;

						fill_in_buffer (  ) ;
						break ;

					case USB_GET_STRING_DESCRIPTOR:
						addr_to_send =
							USBStringDescriptorsPtr
							[SetupPacket->wValue0] ;
						cnt_to_send = *addr_to_send ;

						fill_in_buffer (  ) ;
						break ;

					default:	// Inconnu: stall ep0
						Interfaces[0].Output.BDnSTAT =
							UOWN | BSTALL ;
						Interfaces[0].Input.BDnSTAT =
							UOWN | BSTALL ;
						UCONbits.PKTDIS = 0 ;
						return ;

					}
				}

				if ( SetupPacket->bRequest == USB_REQ_GET_STATUS ) {

					ep_buffers.IN0[0] = 0x00 ;
					ep_buffers.IN0[1] = 0x00 ;

					Interfaces[0].Input.BDnCNT = 2 ;
				}
			}

			if ( SetupPacket->bmRequestType == 0x00 ) {	// data will go from Host to Device

				switch ( SetupPacket->bRequest ) {

				case USB_REQ_SET_ADDRESS:
					addresse = SetupPacket->wValue0 ;
					break ;

				case USB_REQ_SET_CONFIGURATION:
					setup_endpoints (  ) ;
					break ;

				default:
					break ;

				}

			}

			/* Give back buffesr to the USB module */
			Interfaces[0].Output.BDnSTAT = UOWN | DTSEN ;
			Interfaces[0].Input.BDnSTAT = UOWN | DTS | DTSEN ;

			/* Reset the Packet disable bit which is automatically
			 * set for processing SETUP Packets */
			UCONbits.PKTDIS = 0 ;
		}

	} else {		// This is an In packet

		if ( ( addresse != 0x00 ) && ( UADDR != addresse ) )
			UADDR = addresse ;

		fill_in_buffer (  ) ;
		dts = Interfaces[0].Input.dts ;
		Interfaces[0].Input.BDnSTAT = 0 ;	// Reset BDnSTAT before set

		/* NB: The firmware should not set the UOWN bit
		   in the same instruction cycle as any other
		   modifications to the BDnSTAT soft
		   register. The UOWN bit should only be set
		   in a separate instruction cycle, only after
		   all other bits in BDnSTAT (and address/
		   count registers) have been fully updated. */
		if ( dts )
			Interfaces[0].Input.BDnSTAT = UOWN | DTSEN ;
		else
			Interfaces[0].Input.BDnSTAT = UOWN | DTS | DTSEN ;

	}
}

/* Handle HYG-USB processing (EP1) */
void process_command_packets (  ) {
	uint8_t dts ;

	if ( USTATbits.DIR == 0 ) {	// This is an OUT transaction

		if ( ep_buffers.OUT1[0] == 65 )
			__dev_state.green_led = LED_ON ;

		if ( ep_buffers.OUT1[0] == 66 )
			__dev_state.green_led = LED_OFF ;

		if ( ep_buffers.OUT1[0] == 67 ) {
			set_all_leds ( 0 ) ;
			__dev_state.green_led = LED_AUTO ;
		}

		if ( ep_buffers.OUT1[1] == 65 )
			__dev_state.yellow_led = LED_ON ;

		if ( ep_buffers.OUT1[1] == 66 )
			__dev_state.yellow_led = LED_OFF ;

		if ( ep_buffers.OUT1[1] == 67 ) {
			set_all_leds ( 0 ) ;
			__dev_state.yellow_led = LED_AUTO ;
		}

		if ( ep_buffers.OUT1[2] == 65 )
			__dev_state.red_led = LED_ON ;

		if ( ep_buffers.OUT1[2] == 66 )
			__dev_state.red_led = LED_OFF ;

		if ( ep_buffers.OUT1[2] == 67 ) {
			set_all_leds ( 0 ) ;
			__dev_state.red_led = LED_AUTO ;
		}

		// Received OUT transaction: arm IN buffer for a request.
		// If not already done in main loop
		if ( !( Interfaces[1].Input.uown ) ) {
			usb_arm_in_transfert (  ) ;
		}

		Interfaces[1].Output.BDnSTAT = 0 ;
		Interfaces[1].Output.BDnCNT = 0 ;
		Interfaces[1].Output.BDnSTAT = UOWN | DTS | DTSEN ;

	} else {
		// Completed IN transaction: rearm for a next request.
		// If not already done in main loop
		if ( !( Interfaces[1].Input.uown ) ) {
			usb_arm_in_transfert (  ) ;
		}
	}
}

/* Main USB interrupt handler */
void usb_interrupt_handler (  ) {

	if ( !PIR2bits.USBIF )
		return ;

#if defined( _USB_ERROR_INTERRUPT )
	if ( UIRbits.UERRIF ) {	// Detection d'erreur
		if ( UEIRbits.BTSEF ) {	// Bit Stuff Error Flag
		}
		if ( UEIRbits.BTOEF ) {	// Bus Turnaround Time-out Error Flag
		}
		if ( UEIRbits.DFN8EF ) {	// Data Field Size Error Flag
		}
		if ( UEIRbits.CRC16EF ) {	//CRC16 Failure Flag
		}
		if ( UEIRbits.CRC5EF ) {	// CRC5 Host Error Flag
		}
		if ( UEIRbits.PIDEF ) {	// PID Check Failure Flag bit
		}
		UEIR = 0 ;
	}
#endif

	if ( UIRbits.TRNIF ) {	// Transaction complete

		switch ( USTATbits.ENDP ) {
		case 0:
			process_setup_packets (  ) ;
			break ;	// EP0: Setup
		case 1:
			process_command_packets (  ) ;
			break ;	// EP1: Command v1
		default:
			break ;
		}

		// Clear TRNIF, Advance USTAT FIFO
		//UIRbits.TRNIF = 0 ;
	}

	if ( UIRbits.STALLIF ) {	// Stall

		if ( UEP0bits.EPSTALL == 1 ) { // Stall on Endpoint 0
			// Reset
			Interfaces[0].Output.BDnCNT = EP0_BUFLEN ;
			Interfaces[0].Output.BDnSTAT = UOWN | DTSEN ;

			Interfaces[0].Input.BDnSTAT = 0x00 ;

			UEP0bits.EPSTALL = 0 ;
		}

		if ( UEP1bits.EPSTALL == 1 ) { // Stall on Endpoint 1
			// Reset
			Interfaces[0].Output.BDnCNT = EP0_BUFLEN ;
			Interfaces[0].Output.BDnSTAT = UOWN | DTSEN ;

			Interfaces[0].Input.BDnSTAT = 0x00 ;

			// Toggle LEDs to notify
			__dev_state.red_led = LED_ON;
			__dev_state.green_led = LED_OFF;
			__dev_state.yellow_led = LED_OFF;

			UEP1bits.EPSTALL = 0 ;
		}
		//UIRbits.STALLIF = 0 ;
	}

	if ( UIRbits.URSTIF ) {	// Reset
		reset_usb (  ) ;
		//UIRbits.URSTIF = 0 ;
	}
	// Clear all bits ;
	UIR = 0x00 ;
	PIR2bits.USBIF = 0 ;
}

/* Allow checking Input Buffer ownership from outside */
bool usb_in_endpoint_busy ( uint8_t endpoint ) {
	return Interfaces[endpoint].Input.uown ;
}

/* Update IN1 Buffer data for next IN request from host */
void usb_arm_in_transfert (  ) {
	uint8_t dts ;

	Interfaces[1].Input.BDnCNT = sizeof ( __dev_state ) ;

	memcpy ( ep_buffers.IN1, &__dev_state, sizeof ( __dev_state ) ) ;

	dts = Interfaces[1].Input.dts ;
	Interfaces[1].Input.BDnSTAT = 0 ;

	if ( dts )
		Interfaces[1].Input.BDnSTAT = UOWN | DTSEN ;
	else
		Interfaces[1].Input.BDnSTAT = UOWN | DTS | DTSEN ;
}

/* Local Variables:    */
/* mode: c             */
/* c-file-style: "k&r" */
/* c-basic-offset: 8   */
/* indent-tabs-mode: t */
/* End:                */
