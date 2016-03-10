/*
 * apps.c
 *
 * Created: 11/18/2013 1:18:08 PM
 *  Author: Josh Haskins
 */ 


#include <asf.h>
#include <string.h>











void temp_app(){

		ssd1306_set_page_address(0); //changes line number (0-3)
		ssd1306_set_column_address(0); //change line position (128 pixels wide, you can choose 0-127)
		ssd1306_write_text("                                                                                                    ");
	
		double temp;
		at30tse_read_temperature(&temp);
	
		//ssd1306_set_page_address(0); //changes line number (0-3)
		//ssd1306_set_column_address(0); //change line position (128 pixels wide, you can choose 0-127)
		//ssd1306_write_text((uint8_t)temp);
		



}