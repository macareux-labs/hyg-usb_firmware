#ifndef SENSOR_7021
#define SENSOR_7021

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

uint8_t get_si7021_serial ( si7021serial * ) ;
#endif
