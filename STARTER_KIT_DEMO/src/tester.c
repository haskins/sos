/*
 * tester.c
 *
 * Created: 11/20/2013 1:39:46 PM
 *  Author: Josh Haskins
 */ 
#include <asf.h>
#include "data.h"

void getTemp(void );


void getTemp(){
	adc_value = adc_get_channel_value(ADC, ADC_CHANNEL_4);
}
