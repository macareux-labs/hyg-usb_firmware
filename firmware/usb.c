#include <xc.h>
#include "usb.h"
#include "leds.h"
#include "descriptors.h"
#include "main.h"

#define UOWN 		0x80 
#define DTSEN  		0x08
#define DTS  		0x40
#define BSTALL          0x04

typedef struct {
	unsigned char Stat ;
	unsigned char Cnt ;
	unsigned int ADDR ;
} __BufferDescriptor;

typedef struct {
	__BufferDescriptor Output ;
	__BufferDescriptor Input ;
} Interface ;


volatile __at(0x2000) Interface Interfaces[2] ;					// MUST be stored at 0x2000

unsigned char* addr_to_send ;
unsigned int   cnt_to_send  ;

unsigned char addresse ;


// --- End Point 0

volatile __at(0x203F) unsigned char   EP0_OUTbuffer[8] ;
volatile __at(0x2071) unsigned char   EP0_INbuffer[8] ;

// --- End Point 1

volatile __at(0x207B) unsigned char   EP1_OUTbuffer[8] ;
volatile __at(0x2084) unsigned char   EP1_INbuffer[8] ;



// Predecla


void setup_endpoints() ;



void setup_usb() {

	UCFG = 0x10 ;	// Enable Pullups ; Low speed device ; no ping pong
	UADDR = 0x00 ;	// Reset usb adress
	UEIR = 0x00 ;	// Clear all usb error interrupts

	addresse = 0x00 ;

	// Reset PP buffers 
	
	do {
		UCONbits.PPBRST = 1 ;
		UCONbits.PPBRST = 0 ;

	} while(0) ;


	// Enable Packet transfers

	UCONbits.PKTDIS = 0 ;	


	// Enable USB module

	if ( UCONbits.USBEN == 0 ) {
		UCON = 0 ;
		UIE = 0 ;
		UCONbits.USBEN = 1 ;
	}

	while( UCONbits.SE0 ) ;

	UIR = 0 ;
	UIE = 0 ;
	UIEbits.URSTIE = 1 ;
	UIEbits.IDLEIE = 1 ;


	// Enable interrupts

	UIE = 0x4B ;		// Enable TRNIF, SOFIF, ERROR, RESET
	
	INTCONbits.PEIE = 1;	// Enable Peripheral interrupts
	INTCONbits.GIE = 1;	// General interrupt enabled 
	PIE2bits.USBIE = 1;	// USB interrupt enabled
}


void setup_endpoints() {

	UEP1 = 0x1E ;			

	Interfaces[1].Output.Cnt  = 8 ;
	Interfaces[1].Output.ADDR = 0x207B ;	
	Interfaces[1].Output.Stat = UOWN | DTSEN ;
	
	Interfaces[1].Input.Stat  = 0x00 ;

}



void reset_usb() {

	UEIR = 0x00 ;
	UIR = 0x00 ;
	UEIE = 0x9f ;
	UIE = 0x7b ;
	UADDR = 0x00 ;
	
	addresse = 0x00 ;
	cnt_to_send = 0 ;
	
	UEP0 = 0x16 ;

	UIRbits.TRNIF = 0 ;
	UIRbits.TRNIF = 0 ;
	UIRbits.TRNIF = 0 ;
	UIRbits.TRNIF = 0 ;

	UCONbits.PKTDIS = 0 ;

	// WaitForPacket

	Interfaces[0].Output.Cnt  = 8 ;
	Interfaces[0].Output.ADDR = 0x203F ;
	Interfaces[0].Output.Stat = UOWN | DTSEN ;

	Interfaces[0].Input.Stat  = 0x00 ;

}


void fill_in_buffer() {

	unsigned int i ;
	unsigned int byte_count ;

	if ( cnt_to_send < 8 )
		byte_count = cnt_to_send ;
	else
		byte_count = 8 ;

	Interfaces[0].Input.Stat &= ~( 0x03 ) ;
	Interfaces[0].Input.Cnt = byte_count ;
	Interfaces[0].Input.ADDR = 0x2071 ;

	for ( i=0; i<byte_count; i++ )
		EP0_INbuffer[i] = addr_to_send[i] ;

	cnt_to_send  -= byte_count ;
	addr_to_send += byte_count ;

}


void process_setup_packets() {

		
			if ( USTATbits.DIR == 0 ) {						// This is an OUT or a SETUP transaction
			
				unsigned char PID = ( Interfaces[0].Output.Stat & 0x3C ) >> 2 ;

				if ( PID == 0x0D ) {						// This is a SETUP packet
					
					Interfaces[0].Input.Stat  &= ~UOWN ;
					Interfaces[0].Output.Stat &= ~UOWN ;

					SetupPacketStruct* SetupPacket ;
					SetupPacket = (SetupPacketStruct*) EP0_OUTbuffer ;

					if ( SetupPacket->bmRequestType == 0x80 ) { 		// Data will go from Device to host

						if ( SetupPacket->bRequest == 0x06 ) { 		// GET_DESCRIPTOR
							
							switch ( SetupPacket->wValue1 ) {
							
								case 0x01: 			// Device Descriptor		
						
									addr_to_send = (unsigned char*) DeviceDescriptor ; 
									cnt_to_send  = SetupPacket->wLength ;
									Interfaces[0].Output.Cnt  = 8 ;
									Interfaces[0].Output.ADDR = 0x203F ;
									Interfaces[0].Output.Stat = UOWN | DTSEN ;
									fill_in_buffer() ;
									Interfaces[0].Input.Stat = UOWN | DTS | DTSEN ;
									UCONbits.PKTDIS = 0 ;
									break ;

								case 0x02 :			// Configuration Descriptor
								
									addr_to_send = (unsigned char*) ConfigurationDescriptor ; 
									cnt_to_send  = SetupPacket->wLength ;		
									Interfaces[0].Output.Cnt  = 8 ;
									Interfaces[0].Output.ADDR = 0x203F ;
									Interfaces[0].Output.Stat = UOWN | DTSEN ;
									fill_in_buffer() ;
									Interfaces[0].Input.Stat = UOWN | DTS | DTSEN ;
									UCONbits.PKTDIS = 0 ;
									break ;

								case 0x03 :			// String descriptor

									switch( SetupPacket->wValue0 ) {

										case 0x00:
											addr_to_send = (unsigned char*) &StringDescriptor0 ; 
											cnt_to_send  = sizeof(StringDescriptor0) ; 
											break ;
										case 0X01:
											addr_to_send = (unsigned char*) &StringDescriptor1 ;
											cnt_to_send = sizeof(StringDescriptor1) ;
											break ;

										case 0X02:
											addr_to_send = (unsigned char*) &StringDescriptor2 ;
											cnt_to_send = sizeof(StringDescriptor2) ;
											break ;

									}

									Interfaces[0].Output.Cnt  = 8 ;
									Interfaces[0].Output.ADDR = 0x203F ;
									Interfaces[0].Output.Stat = UOWN | DTSEN ;
									fill_in_buffer() ;
									Interfaces[0].Input.Stat = UOWN | DTS | DTSEN ;	
									UCONbits.PKTDIS = 0 ;
									
									break ;

								default:
									
									Interfaces[0].Output.Cnt = 8 ;
									Interfaces[0].Output.ADDR = 0x203F ;
									Interfaces[0].Output.Stat = UOWN | BSTALL;
									
									Interfaces[0].Input.Cnt = 0 ;
									Interfaces[0].Input.ADDR = 0x2071 ;
									Interfaces[0].Input.Stat  = UOWN | BSTALL;
									UCONbits.PKTDIS = 0 ;

									break ;

							}
					       }

					       if ( SetupPacket->bRequest == 0x00 ) {		// GET_STATUS
					
							EP0_INbuffer[0] = 0x00 ;
							EP0_INbuffer[1] = 0x00 ;
							Interfaces[0].Output.Cnt  = 8 ;
						        Interfaces[0].Output.ADDR = 0x203F ;
						        Interfaces[0].Output.Stat = UOWN | DTSEN ;
							Interfaces[0].Input.Cnt = 2 ; 
							Interfaces[0].Input.ADDR = 0x2071 ;
							Interfaces[0].Input.Stat = UOWN | DTS | DTSEN ;

							UCONbits.PKTDIS = 0 ;

					      }
					}

					if ( SetupPacket->bmRequestType == 0x00 ) {		// data will go from Host to Device


						switch ( SetupPacket->bRequest ) {
						
							case 0x05 :							// SET ADDRESS
								addresse = SetupPacket->wValue0 ;
								
								Interfaces[0].Input.Cnt = 0 ;
								Interfaces[0].Input.ADDR = 0x2071 ;
								Interfaces[0].Input.Stat = UOWN | DTS | DTSEN ;
								
								Interfaces[0].Output.Cnt  = 8 ;
								Interfaces[0].Output.ADDR = 0x203F ;
								Interfaces[0].Output.Stat = UOWN | DTSEN ;
								
								UCONbits.PKTDIS = 0 ;
								
								break ;

							case 0x09 :							// SET CONFIGURATION
								Interfaces[0].Input.Cnt = 0 ;
								Interfaces[0].Input.ADDR = 0x2071 ;
								Interfaces[0].Input.Stat = UOWN | DTS | DTSEN ;
								
								Interfaces[0].Output.Cnt = 8 ;
								Interfaces[0].Output.ADDR = 0x203F ;
								Interfaces[0].Output.Stat = UOWN | DTSEN ;
								
								UCONbits.PKTDIS = 0 ;
							
								setup_endpoints() ;

								break ;

							default:
								break ;


						}

					}
					
					
				
				}

			} else {								// This is an In packet


				if (( addresse != 0x00 ) && ( UADDR != addresse ) )
					UADDR = addresse ;

				fill_in_buffer() ;

				if ( Interfaces[0].Input.Stat & DTS )
					Interfaces[0].Input.Stat = UOWN | DTSEN ;
				else
					Interfaces[0].Input.Stat = UOWN | DTS | DTSEN ;

			}






}



void usb_interrupt_handler() {


	if (!PIR2bits.USBIF)
		return;

	if (UIRbits.TRNIF) {									// Transaction complete
		
		if ( USTATbits.ENDP == 0 ) {							// This is for endpoint 0

			process_setup_packets() ;


		} else {


			if ( USTATbits.ENDP == 1 ) {							// This is for endpoint 1
			
				if ( USTATbits.DIR == 0 ) {						// This is an OUT transaction

					if ( EP1_OUTbuffer[0] == 65 ) 		
						green_led = LED_ON ;
					
					if ( EP1_OUTbuffer[0] == 66 )
						green_led = LED_OFF ;
					
					if ( EP1_OUTbuffer[0] == 67 ) {
						set_all_leds(0) ;
						green_led = LED_AUTO ; }

					if ( EP1_OUTbuffer[1] == 65 ) 
						yellow_led = LED_ON ;
					
					if ( EP1_OUTbuffer[1] == 66 )
						yellow_led = LED_OFF ;
					
					if ( EP1_OUTbuffer[1] == 67 ) {
						set_all_leds(0) ;
						yellow_led = LED_AUTO ; }

					if ( EP1_OUTbuffer[2] == 65 ) 
						red_led = LED_ON ;
					
					if ( EP1_OUTbuffer[2] == 66 )
						red_led = LED_OFF ;
					
					if ( EP1_OUTbuffer[2] == 67 ) {
						set_all_leds(0) ;
						red_led = LED_AUTO ; }

					if ( EP1_OUTbuffer[3] == 65 ) {

						Interfaces[1].Input.Cnt = 8 ;
						Interfaces[1].Input.ADDR = 0x2084 ;

						EP1_INbuffer[0] = hyg_MSB ;			
						EP1_INbuffer[1] = hyg_LSB ;
						EP1_INbuffer[2] = temp_MSB ;
						EP1_INbuffer[3] = temp_LSB ;
						EP1_INbuffer[4] = green_led ;
						EP1_INbuffer[5] = yellow_led ;
						EP1_INbuffer[6] = red_led ;
						EP1_INbuffer[7] = hyg_MSB ^ hyg_LSB ^ temp_MSB ^ temp_LSB ^ green_led ^ yellow_led ^ red_led ;

						Interfaces[1].Input.Stat = UOWN | DTS | DTSEN ;
					
					}


					Interfaces[1].Output.Cnt  = 8 ;
					Interfaces[1].Output.ADDR = 0x207B ;	

					if ( Interfaces[1].Output.Stat & DTS )
						Interfaces[1].Output.Stat = UOWN | DTSEN ;
					else
						Interfaces[1].Output.Stat = UOWN | DTS | DTSEN ;


				} else {								// Last one was an IN transaction

					EP1_INbuffer[0] = hyg_MSB ;			
					EP1_INbuffer[1] = hyg_LSB ;
					EP1_INbuffer[2] = temp_MSB ;
					EP1_INbuffer[3] = temp_LSB ;
					EP1_INbuffer[4] = green_led ;
					EP1_INbuffer[5] = yellow_led ;
					EP1_INbuffer[6] = red_led ;
					EP1_INbuffer[7] = hyg_MSB ^ hyg_LSB ^ temp_MSB ^ temp_LSB ^ green_led ^ yellow_led ^ red_led ;

					if ( Interfaces[1].Input.Stat & DTS )
						Interfaces[1].Input.Stat = UOWN | DTSEN ;
					else
						Interfaces[1].Input.Stat = UOWN | DTS | DTSEN ;


				}
			
			
			}

		}
		
		UIRbits.TRNIF = 0;
	}

	if (UIRbits.SOFIF) {				// Start of Frame ( not used )

		UIRbits.SOFIF = 0 ;
	}

	if (UIRbits.STALLIF) {				// Stall 

		if (UEP0bits.EPSTALL == 1) {
		
			Interfaces[0].Output.Cnt  = 8 ;
			Interfaces[0].Output.ADDR = 0x203F ;
			Interfaces[0].Output.Stat = UOWN | DTSEN ;
			
			Interfaces[0].Input.Stat  = 0x00 ;

			UEP0bits.EPSTALL = 0;
		}

		if (UEP1bits.EPSTALL == 1) {
		
			set_green_led(0) ;		
			set_yellow_led(0) ;
			set_red_led(1) ;
			

			UEP1bits.EPSTALL = 0;
		}

		UIRbits.STALLIF = 0;
	}

	if (UIRbits.URSTIF) {				// Reset
		reset_usb();
		UIRbits.URSTIF = 0;
	}
	
	UIR = 0x00 ;
	PIR2bits.USBIF = 0;
						
}
