#include <xc.h>
#include <string.h>

#include "types.h"
#include "i2c.h"
#include "si7021A10.h"

signed char get_i2c_word_at ( unsigned char addr, __WORD * value, bool check_crc ) {
	signed char ret ;
	uint8_t crc ;
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

#ifdef ENABLE_CRC
		if ( check_crc ) {
			AckI2C (  ) ;
			IdleI2C (  ) ;
			crc = ReadI2C (  ) ;

			if ( !check_si7021_crc ( (uint8_t*) value, 2, crc ) ) {
				value->msb = 0xee ;
				value->lsb = 0xee ;
			}
		}
#endif

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
	return get_i2c_word_at ( 0xE5, value, true ) ;
}

/* Measure Temperature */
signed char measure_temp ( __WORD * value ) {
	return get_i2c_word_at ( 0xE3, value, true ) ;
}

/* Measure Relative Humidity and fetch measured temp */
signed char measure_all ( __WORD * rh, __WORD * temp ) {
	uint8_t r1 = get_i2c_word_at ( 0xE5, rh, true ) ;
	uint8_t r2 = get_i2c_word_at ( 0xE0, temp, false ) ; // No CRC available when fetching temp from relative humidity
	return r1 && r2 ;
}

#ifdef ENABLE_CRC
signed char check_si7021_crc ( uint8_t *values, uint8_t len, uint8_t check ) {
    unsigned char crc = 0u;
    char i, j;

    for (i = 0; i < len; i++ ) {
	    crc ^= values[i];
	    for (j = 8u; j > 0u; j--)
	    {
		    if (crc & 0x80u)
			    crc = (crc << 1u) ^ 0x131u;
		    else
			    crc = (crc << 1u);
	    }
    }

    return (crc == check);
}
#else
#define check_si7021_crc(v,l,c) (true)
#endif

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

		if ( !check_si7021_crc ( serial->bytes, 1, crc ) ) {
			serial->bytes[0] = 0xee ;
		}

		AckI2C (  ) ;
		IdleI2C (  ) ;

		serial->bytes[1] = ReadI2C (  );	//SNA_2
		AckI2C (  ) ;
		IdleI2C (  ) ;
		crc = ReadI2C (  ) ;

		if ( !check_si7021_crc ( serial->bytes, 2, crc ) ) {
			serial->bytes[1] = 0xee ;
		}

		AckI2C (  ) ;
		IdleI2C (  ) ;

		serial->bytes[2] = ReadI2C (  );	//SNA_1
		AckI2C (  ) ;
		IdleI2C (  ) ;
		crc = ReadI2C (  ) ;

		if ( !check_si7021_crc ( serial->bytes, 3, crc ) ) {
			serial->bytes[2] = 0xee ;
		}

		AckI2C (  ) ;
		IdleI2C (  ) ;

		serial->bytes[3] = ReadI2C (  );	//SNA_0
		AckI2C (  ) ;
		IdleI2C (  ) ;
		crc = ReadI2C (  ) ;

		if ( !check_si7021_crc ( serial->bytes, 4, crc ) ) {
			serial->bytes[3] = 0xee ;
		}

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

			if ( !check_si7021_crc ( serial->bytes+4, 2, crc ) ) {
				serial->bytes[4] = 0xee ;
			}

			serial->bytes[6] = ReadI2C (  );	//SNB_1
			AckI2C (  ) ;
			IdleI2C (  ) ;
			serial->bytes[7] = ReadI2C (  );	//SNB_0
			AckI2C (  ) ;
			IdleI2C (  ) ;
			crc = ReadI2C (  ) ;
			NotAckI2C (  ) ;

			if ( !check_si7021_crc ( serial->bytes+4, 4, crc ) ) {
				serial->bytes[6] = 0xee ;
			}

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
