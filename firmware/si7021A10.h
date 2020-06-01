#ifndef SENSOR_7021
#define SENSOR_7021

#define   CRC_POLY			0b100110001	/* CRC Polynome x^8 + x^5 + x^4 + 1 */

typedef union {
	uint8_t bytes[8] ;
	struct {
		uint32_t sna ;
		uint32_t snb ;
	} ;
} si7021serial ;

signed char measure_rh ( __WORD * ) ;
signed char measure_temp ( __WORD * ) ;
signed char measure_all ( __WORD *, __WORD * ) ;
signed char check_si7021_crc ( uint8_t *, uint8_t, uint8_t ) ;

uint8_t get_si7021_serial ( si7021serial * ) ;
#endif

/* Local Variables:    */
/* mode: c             */
/* c-file-style: "k&r" */
/* c-basic-offset: 8   */
/* indent-tabs-mode: t */
/* End:                */
