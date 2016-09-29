#include <xc.h>
#include <string.h>

#include "types.h"
#include "i2c.h"
#include "si7021A10.h"

signed char get_i2c_word_at ( unsigned char addr, __WORD * value ) {
	signed char ret ;
	ret = 0 ;

	IdleI2C (  ) ;
	StartI2C (  ) ;
	IdleI2C (  ) ;
	ret |= WriteI2C ( 0x80 ) ;
	IdleI2C (  ) ;
	ret |= WriteI2C ( addr ) ;
	IdleI2C (  ) ;
	RestartI2C (  ) ;
	IdleI2C (  ) ;
	ret |= WriteI2C ( 0x81 ) ;
	IdleI2C (  ) ;

	if ( !ret ) {

		value->msb = ReadI2C (  ) ;
		AckI2C (  ) ;
		IdleI2C (  ) ;
		value->lsb = ReadI2C (  ) ;
		NotAckI2C (  ) ;

	} else {

		value->msb = 0 ;
		value->lsb = 0 ;

	}

	IdleI2C (  ) ;
	StopI2C (  ) ;

	return ret ;
}

/* Measure Relative Humidity */
signed char measure_rh ( __WORD * value ) {
	return get_i2c_word_at ( 0xE5, value ) ;
}

/* Measure Temperature */
signed char measure_temp ( __WORD * value ) {
	return get_i2c_word_at ( 0xE0, value ) ;
}

/* Measure Relative Humidity and fetch measured temp */
signed char measure_all ( __WORD * rh, __WORD * temp ) {
	uint8_t r1 = get_i2c_word_at ( 0xE5, rh ) ;
	uint8_t r2 = get_i2c_word_at ( 0xE0, temp ) ;
	return r1 && r2 ;
}

/* Fetch 64bit serial number of sensor */
uint8_t get_si7021_serial ( si7021serial * serial ) {
	signed char ret ;
	uint8_t crc ;
	ret = 0 ;

	memset ( serial, 0x00, sizeof ( si7021serial ) ) ;

	IdleI2C (  ) ;
	StartI2C (  ) ;
	IdleI2C (  ) ;
	ret |= WriteI2C ( 0x80 ) ;
	IdleI2C (  ) ;
	ret |= WriteI2C ( 0xFA ) ;
	IdleI2C (  ) ;
	ret |= WriteI2C ( 0x0F ) ;
	IdleI2C (  ) ;
	RestartI2C (  ) ;
	IdleI2C (  ) ;
	ret |= WriteI2C ( 0x81 ) ;
	IdleI2C (  ) ;

	if ( !ret ) {
		serial->bytes[0] = ReadI2C (  );	//SNA_3
		AckI2C (  ) ;
		IdleI2C (  ) ;
		crc = ReadI2C (  ) ;
		AckI2C (  ) ;
		IdleI2C (  ) ;

		serial->bytes[1] = ReadI2C (  );	//SNA_2
		AckI2C (  ) ;
		IdleI2C (  ) ;
		crc = ReadI2C (  ) ;
		AckI2C (  ) ;
		IdleI2C (  ) ;

		serial->bytes[2] = ReadI2C (  );	//SNA_1
		AckI2C (  ) ;
		IdleI2C (  ) ;
		crc = ReadI2C (  ) ;
		AckI2C (  ) ;
		IdleI2C (  ) ;

		serial->bytes[3] = ReadI2C (  );	//SNA_0
		AckI2C (  ) ;
		IdleI2C (  ) ;
		crc = ReadI2C (  ) ;
		NotAckI2C (  ) ;

		// Read Second DWORD
		IdleI2C (  ) ;
		RestartI2C (  ) ;
		IdleI2C (  ) ;
		ret |= WriteI2C ( 0x80 ) ;
		IdleI2C (  ) ;
		ret |= WriteI2C ( 0xFC ) ;
		IdleI2C (  ) ;
		ret |= WriteI2C ( 0xC9 ) ;
		IdleI2C (  ) ;
		RestartI2C (  ) ;
		IdleI2C (  ) ;
		ret |= WriteI2C ( 0x81 ) ;
		IdleI2C (  ) ;

		if ( !ret ) {
			serial->bytes[4] = ReadI2C (  );	//SNB_3
			AckI2C (  ) ;
			IdleI2C (  ) ;
			serial->bytes[5] = ReadI2C (  );	//SNB_2
			AckI2C (  ) ;
			IdleI2C (  ) ;
			crc = ReadI2C (  ) ;
			AckI2C (  ) ;
			IdleI2C (  ) ;

			serial->bytes[6] = ReadI2C (  );	//SNB_1
			AckI2C (  ) ;
			IdleI2C (  ) ;
			serial->bytes[7] = ReadI2C (  );	//SNB_0
			AckI2C (  ) ;
			IdleI2C (  ) ;
			crc = ReadI2C (  ) ;
			NotAckI2C (  ) ;

		}
	}

	IdleI2C (  ) ;
	StopI2C (  ) ;

	return ret ;
}

/* Local Variables:    */
/* mode: c             */
/* c-file-style: "k&r" */
/* c-basic-offset: 8   */
/* indent-tabs-mode: t */
/* End:                */
