#include <xc.h>

#include "i2c.h"
#include "si7021A10.h"


signed char measure_rh( unsigned char* hygro_MSB, unsigned char* hygro_LSB ) {

	signed char ret ;
	ret = 0 ;

	IdleI2C();
	StartI2C() ;
	IdleI2C();
	ret |= WriteI2C( 0x80 ) ;
	IdleI2C();
	ret |= WriteI2C( 0xE5 ) ;
	IdleI2C();
	RestartI2C() ;
	IdleI2C() ;
	ret |= WriteI2C( 0x81 ) ;
	IdleI2C() ;
	
	if ( !ret ) {
	
		*hygro_MSB=ReadI2C() ;
		AckI2C() ;
		IdleI2C() ;
		*hygro_LSB=ReadI2C() ;
		NotAckI2C() ;

	} else {

		*hygro_MSB = 0 ;
		*hygro_LSB = 0 ;

	}

	IdleI2C();
	StopI2C();

	return ret ;
}


signed char measure_temp( unsigned char* temp_MSB, unsigned char* temp_LSB ) {



	signed char ret ;
	ret = 0 ;

	IdleI2C();
	StartI2C() ;
	IdleI2C();
	ret |= WriteI2C( 0x80 ) ;
	IdleI2C();
	ret |= WriteI2C( 0xE3 ) ;
	IdleI2C();
	RestartI2C() ;
	IdleI2C() ;
	ret |= WriteI2C( 0x81 ) ;
	IdleI2C() ;
	
	if ( !ret ) {
	
		*temp_MSB=ReadI2C() ;
		AckI2C() ;
		IdleI2C() ;
		*temp_LSB=ReadI2C() ;
		NotAckI2C() ;

	} else {

		*temp_MSB = 0 ;
		*temp_LSB = 0 ;

	}

	IdleI2C();
	StopI2C();

	return ret ;


}


