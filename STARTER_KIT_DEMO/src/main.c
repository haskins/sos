/**
 * \file
 *
 * \brief Starter Kit Demo.
 *
 * Copyright (c) 2013 Atmel Corporation. All rights reserved.
 *
 * \asf_license_start
 *
 * \page License
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. The name of Atmel may not be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * 4. This software may only be redistributed and used in connection with an
 *    Atmel microcontroller product.
 *
 * THIS SOFTWARE IS PROVIDED BY ATMEL "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT ARE
 * EXPRESSLY AND SPECIFICALLY DISCLAIMED. IN NO EVENT SHALL ATMEL BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * \asf_license_stop
 *
 */

/*
 * Simple OS.
 * This is a simple multi-threaded micro-kernal mini-operating system.
 *
 *  Author: Devon Harker, Josh Haskins, Vincent Tennant
 */ 

#include <asf.h>
#include <string.h>
#include "sysnums.h"
#include "data.h"
#include "threads.h"

#define BUFFER_SIZE				128

#define svc(code) asm volatile ("svc %[immediate]"::[immediate] "I" (code))

#define LIGHT_OFF               false //this is correct, true = off, false = on
#define LIGHT_ON				!LIGHT_OFF

#define ENABLED					true
#define DISABLED				false

#define MENU_NO_MENU			2
#define MENU_MAIN				4
#define MENU_APP				6

#define BUTTON_PUSHED			1
#define BUTTON_RELEASED			0

#define TRUE					1
#define FALSE					0

#define MODE_ON					1
#define MODE_OFF				0

/* IRQ priority for PIO (The lower the value, the greater the priority) */
#define IRQ_PRIOR_PIO			0


static void ssd1306_clear_char(void);

/* These settings will force to set and refresh the temperature mode. */
volatile uint32_t menu_screen = 2;
volatile uint32_t screen_extension = 0;
volatile uint32_t menu_screen_switch = 1;
volatile uint32_t sd_update = 0;
volatile uint32_t sd_fs_found = 0;
volatile uint32_t sd_listing_pos = 0;
volatile uint32_t sd_num_files = 0;

FATFS fs;

bool app_mode = DISABLED;
bool volatile temp_mode = DISABLED;
int volatile light_mode = DISABLED;
int menu_mode = MENU_NO_MENU;
int current_temp, current_light;
char value_disp[5], temp_disp[5], light_disp[5];

/*
 * Controls light 1
 */
void controlLight1(bool a) {
    //LED1 off
    if (a)
        svc(3);
        //LED1 on
    else
        svc(2);

}

/*
 * Controls light 2
 */
void controlLight2(bool b) {
    //LED2 off
    if (b)
        svc(5);
        //LED2 on
    else
        svc(4);
}

/*
 * Controls light 3
 */
void controlLight3(bool c) {
    //LED3 off
    if (c)
        svc(7);
        //LED3 on
    else
        svc(6);
}

/*
 * Controls all lights
 */
void controlLights(bool a, bool b, bool c) {
    //LED1 off
    if (a)
        svc(3);
        //LED1 on
    else
        svc(2);

    //LED2 off
    if (b)
        svc(5);
        //LED2 on
    else
        svc(4);

    //LED3 off
    if (c)
        svc(7);
        //LED3 on
    else
        svc(6);

}

/*
 * Prints char to screen.
 */
__attribute__((noinline)) void printChar(char* c) {
    svc(SYSCALL_WRITECHARTOSCREEN);
}

/*
 * Prints string to screen.
 */
__attribute__((noinline)) void printString(char* c, int l) {
    svc(SYSCALL_WRITESTRINGTOSCREEN);
}

/*
 * Prints string to screen, allows position to be set.
 */
__attribute__((noinline)) void printStringPosition(char* c, int l, int x) {
    svc(SYSCALL_WRITESTRINGTOSCREENPOSITION);
}

/*
 * Print 4 lines of text to screen.
 */
void print4screen(char* a, char* b, char* c, char* d) {
    printString(a, 0);
    printString(b, 1);
    printString(c, 2);
    printString(d, 3);
}

/*
 * Print 1 line of text to screen.
 */
void printLine(char* t, int l) {
    ssd1306_set_page_address(l); //changes line number (0-3)
    ssd1306_set_column_address(0); //change line position (128 pixels wide, you can choose 0-127)
    ssd1306_write_text(t);
}

/*
 * Clears specified line on screen.
 */
void clearLine(int l) {
    ssd1306_set_page_address(l); //changes line number (0-3)
    ssd1306_set_column_address(0); //change line position (128 pixels wide, you can choose 0-127)
    ssd1306_write_text("                                                                                                      ");
}

/*
 * Cleans the screen.
 */
void cleanScreen() {
    svc(SYSCALL_CLEARSCREEN);
}

/*
 * Delay. Uses ms. 
 */
void delay(int d) {
    svc(SYSCALL_DELAY);
}

/*
 * Gets temperature from sensor.
 */
void getTemp(double* t) {
    svc(SYSCALL_GETTEMP);
}

/*
 * Gets light info from light sensor.
 */
void getLight() {
    svc(SYSCALL_GETLIGHT);
    light = 100 - (adc_value * 100 / 4096);
}

/*
 * Thread, that prints text to line 0.
 */
void app_text0() {
    delay(330);
    int x = 0;
    int line_num = 0;
    while (x < 10) {
        x++;
        printString("Lights On     ", line_num);
        delay(200);
        printString("Lights Off", line_num);
        delay(200);
    }
    delay(200);
    printString("I QUIT!            ", line_num);
}

/*
 * Thread, that prints text to line 1.
 */
void app_text1() {
    delay(440);
    int x = 0;
    int line_num = 1;
    int int_delay = 190;
    while (x < 10) {
        x++;
        printString("Lights On     ", line_num);
        delay(int_delay);
        printString("Lights Off", line_num);
        delay(int_delay);
    }
    delay(int_delay);
    printString("I QUIT!            ", line_num);
}

/*
 * Thread, that prints text to line 2.
 */
void app_text2() {
    delay(550);
    int x = 0;
    int line_num = 2;
    int int_delay = 180;
    while (x < 10) {
        x++;
        printString("Lights On     ", line_num);
        delay(int_delay);
        printString("Lights Off", line_num);
        delay(int_delay);
    }
    delay(int_delay);
    printString("I QUIT!            ", line_num);
}

/*
 * Thread, that prints text to line 3.
 */
void app_text3() {
    delay(660);
    int x = 0;
    int line_num = 3;
    int int_delay = 170;
    while (x < 10) {
        x++;
        printString("Lights On     ", line_num);
        delay(int_delay);
        printString("Lights Off", line_num);
        delay(int_delay);
    }
    delay(int_delay);
    printString("I QUIT!            ", line_num);
}

/*
 * Thread, that turns on and off light 1.
 */
void app_light1() {
    delay(210);
    int x = 0;
    int int_delay = 130;
    while (x < 4) {
        x++;
        controlLight1(LIGHT_ON);
        delay(int_delay);

        controlLight1(LIGHT_OFF);
        delay(int_delay);
    }
}

/*
 * Thread, that turns on and off light 2.
 */
void app_light2() {
    delay(220);
    int x = 0;
    int int_delay = 120;
    while (x < 4) {
        x++;
        controlLight2(LIGHT_ON);
        delay(int_delay);

        controlLight2(LIGHT_OFF);
        delay(int_delay);
    }
}

/*
 * Thread, that turns on and off light 3.
 */
void app_light3() {
    delay_ms(230);
    int x = 0;
    int int_delay = 110;
    while (x < 7) {
        x++;
        controlLight3(LIGHT_ON);
        delay_ms(int_delay);

        controlLight3(LIGHT_OFF);
        delay_ms(int_delay);
    }
}

/*
 * Thread, prints the temperature sensor to the screen.
 */
void thread_temp() {
    delay_ms(133);
    int itt = 1;
    temp_mode = DISABLED;
    while (1) {
        if (temp_mode == ENABLED) {
            if (itt % 2) {
                getTemp(&temp);
            } else {
                sprintf(temp_disp, "%d", (uint8_t) temp);
                printStringPosition(temp_disp, 1, 106);
                printStringPosition("c", 1, 119);
            }
            itt++;
            delay_ms(65);

            if (temp > 22) {
                printStringPosition("VERY HOT", 2, 87);
            } else if (temp < 21) {
                printStringPosition("TOO COLD", 2, 87);
            } else {
                printStringPosition("__________", 2, 87);
            }

        } else if (temp_mode == DISABLED) {
            printStringPosition("       ", 1, 106);
            printStringPosition("__________", 2, 87);
        }

    }
}

/*
 * App Launcher. Allows different apps to be launched depending on the screen.
 */
void launchApp() {
    switch (menu_screen) {
        case 2:
            //app_mode = ENABLED;
            //menu_mode = MENU_NO_MENU;
            //ssd1306_clear();

            //createThread( &app_light1, "app_light1 ", 128 );
            //createThread( &app_light2, "app_light2 ", 128 );
            //createThread( &app_light3, "app_light3 ", 128 );
            break;
        case 3:
            if (temp_mode == ENABLED) {
                temp_mode = DISABLED;
            } else if (temp_mode == DISABLED) {
                temp_mode = ENABLED;
            }
            break;
        case 4:
            if (light_mode == ENABLED) {
                light_mode = DISABLED;
            } else {
                light_mode = ENABLED;
            }
            break;
        case 5:
            //app_mode = ENABLED;
            //menu_mode = MENU_NO_MENU;
            //ssd1306_clear();

            //createThread(&app_text0, "app_text0 ", 128);
            //createThread(&app_text1, "app_text1 ", 128);
            //createThread(&app_text2, "app_text2 ", 128);
            //createThread(&app_text3, "app_text3 ", 128);
            break;
    }
}

/*
 * Boot animation.
 */

void boot() {
    while (1) {
        print4screen("", "SOS", "           ", "");
        delay_ms(100);
        print4screen("", "   SOS", "", "");
        delay_ms(100);
        print4screen("", "           ", "   SOS", "");
        delay_ms(100);
        print4screen("", "", "SOS             ", "");
        delay_ms(100);
    }

}

/*
 * Process Buttons Events.
 */
static void ProcessButtonEvt(uint8_t uc_button) {

    delay_ms(200);

    //All Menu clicks
    if (menu_mode == MENU_MAIN && !app_mode) {
        if (uc_button == 1) {
            menu_screen_switch = -1;
        } else if (uc_button == 2 && menu_screen == 0) {
            menu_mode = MENU_APP;
            screen_extension = 3;
            menu_screen_switch = 1;
            menu_screen = 2;
        } else if (uc_button == 2 && menu_screen == 2) {
            //launchApp();
        } else if (uc_button == 3) {
            menu_screen_switch = 1;
        }

    }
	//app menu clicks
    else if (menu_mode == MENU_APP && !app_mode) {
        if (uc_button == 1) {
            menu_mode = MENU_MAIN;
            screen_extension = 0;
            menu_screen = 2;
            menu_screen_switch = 1;
        } else if (uc_button == 2) {
            launchApp();
        } else if (uc_button == 3) {
            menu_screen_switch = 1;
        }

    }        
	//All App clicks
    else if (app_mode && menu_mode == MENU_NO_MENU) {
        if (uc_button == 1) {

            menu_mode = MENU_APP;
            app_mode = DISABLED;
            screen_extension = 3;
            menu_screen = 2;
            menu_screen_switch = 1;

        } else if (uc_button == 2) {
            //TODO, add something here
        } else if (uc_button == 3) {
            //TODO, add something here
        }

    }
	//Welcome screen clicks
    else {
        menu_screen_switch = 1;
    }



}

/**
 * Handler for Button 1 rising edge interrupt.
 */
static void Button1_Handler(uint32_t id, uint32_t mask) {
    if ((PIN_PUSHBUTTON_1_ID == id) && (PIN_PUSHBUTTON_1_MASK == mask))
        ProcessButtonEvt(1);
}

/**
 * brief Handler for Button 2 rising edge interrupt.
 */
static void Button2_Handler(uint32_t id, uint32_t mask) {
    if ((PIN_PUSHBUTTON_2_ID == id) && (PIN_PUSHBUTTON_2_MASK == mask))
        ProcessButtonEvt(2);
}

/**
 * brief Handler for Button 3 rising edge interrupt.
 */
static void Button3_Handler(uint32_t id, uint32_t mask) {
    if ((PIN_PUSHBUTTON_3_ID == id) && (PIN_PUSHBUTTON_3_MASK == mask))
        ProcessButtonEvt(3);
}


/**
 * Configure the Pushbuttons.
 *
 * Configure the PIO as inputs and generate corresponding interrupt when
 * pressed or released.
 */
static void configure_buttons(void) {

    /* Configure Pushbutton 1. */
    pmc_enable_periph_clk(PIN_PUSHBUTTON_1_ID);
    pio_set_debounce_filter(PIN_PUSHBUTTON_1_PIO, PIN_PUSHBUTTON_1_MASK, 10);
    pio_handler_set(PIN_PUSHBUTTON_1_PIO, PIN_PUSHBUTTON_1_ID,
            PIN_PUSHBUTTON_1_MASK, PIN_PUSHBUTTON_1_ATTR, Button1_Handler);
    NVIC_EnableIRQ((IRQn_Type) PIN_PUSHBUTTON_1_ID);
    pio_handler_set_priority(PIN_PUSHBUTTON_1_PIO, (IRQn_Type) PIN_PUSHBUTTON_1_ID, IRQ_PRIOR_PIO);
    pio_enable_interrupt(PIN_PUSHBUTTON_1_PIO, PIN_PUSHBUTTON_1_MASK);

    /* Configure Pushbutton 2. */
    pmc_enable_periph_clk(PIN_PUSHBUTTON_2_ID);
    pio_set_debounce_filter(PIN_PUSHBUTTON_2_PIO, PIN_PUSHBUTTON_2_MASK, 10);
    pio_handler_set(PIN_PUSHBUTTON_2_PIO, PIN_PUSHBUTTON_2_ID,
            PIN_PUSHBUTTON_2_MASK, PIN_PUSHBUTTON_2_ATTR, Button2_Handler);
    NVIC_EnableIRQ((IRQn_Type) PIN_PUSHBUTTON_2_ID);
    pio_handler_set_priority(PIN_PUSHBUTTON_2_PIO, (IRQn_Type) PIN_PUSHBUTTON_2_ID, IRQ_PRIOR_PIO);
    pio_enable_interrupt(PIN_PUSHBUTTON_2_PIO, PIN_PUSHBUTTON_2_MASK);

    /* Configure Pushbutton 3. */
    pmc_enable_periph_clk(PIN_PUSHBUTTON_3_ID);
    pio_set_debounce_filter(PIN_PUSHBUTTON_3_PIO, PIN_PUSHBUTTON_3_MASK, 10);
    pio_handler_set(PIN_PUSHBUTTON_3_PIO, PIN_PUSHBUTTON_3_ID,
            PIN_PUSHBUTTON_3_MASK, PIN_PUSHBUTTON_3_ATTR, Button3_Handler);
    NVIC_EnableIRQ((IRQn_Type) PIN_PUSHBUTTON_3_ID);
    pio_handler_set_priority(PIN_PUSHBUTTON_3_PIO, (IRQn_Type) PIN_PUSHBUTTON_3_ID, IRQ_PRIOR_PIO);
    pio_enable_interrupt(PIN_PUSHBUTTON_3_PIO, PIN_PUSHBUTTON_3_MASK);
}

/**
 * Configure the ADC for the light sensor.
 */
static void configure_adc(void) {
    /* Configure ADC pin for light sensor. */
    gpio_configure_pin(LIGHT_SENSOR_GPIO, LIGHT_SENSOR_FLAGS);

    /* Enable ADC clock. */
    pmc_enable_periph_clk(ID_ADC);

    /* Configure ADC. */
    adc_init(ADC, sysclk_get_cpu_hz(), 1000000, ADC_MR_STARTUP_SUT0);
    adc_enable_channel(ADC, ADC_CHANNEL_4);
    adc_configure_trigger(ADC, ADC_TRIG_SW, 1);
}

/**
 * Clear one character at the cursor current position on the OLED screen.
 */
static void ssd1306_clear_char(void) {
    ssd1306_write_data(0x00);
    ssd1306_write_data(0x00);
    ssd1306_write_data(0x00);
    ssd1306_write_data(0x00);
    ssd1306_write_data(0x00);
    ssd1306_write_data(0x00);
}

/**
 * Main. Is the GUI of the entire OS.
 */
int main(void) {

    while (true) {

        if (!app_mode && menu_mode == MENU_NO_MENU) {
            //Welcome Screen
            controlLights(LIGHT_ON, LIGHT_ON, LIGHT_ON);
            ssd1306_clear(); // Clear screen.
            print4screen("              Welcome to", "              deJovi SOS", "________________________________", " click any button to continue");

            menu_screen_switch = 0;

            while (menu_screen_switch == 0) {
                continue;
            }
            menu_screen_switch = 1;
            menu_mode = MENU_MAIN;
        } else if (!app_mode && (menu_mode == MENU_MAIN || menu_mode == MENU_APP)) {
            if (menu_screen_switch != 0) {

                delay_ms(50); //delay needed to stop skipping

                /* Refresh page title only if necessary. */
                if (menu_screen_switch == 1) {
                    menu_screen = ((menu_screen + 1) % 3) + screen_extension;
                } else if (menu_screen_switch == -1) {
                    if (menu_screen == 0) {
                        menu_screen = 2;
                    } else {
                        menu_screen = ((menu_screen - 1) % 3) + screen_extension;
                    }
                }

                // Clear screen.
                cleanScreen();
                ssd1306_set_page_address(0);
                ssd1306_set_column_address(0);

                /* Built in app Mode. */
                if (menu_screen == 0) {
                    controlLights(LIGHT_ON, LIGHT_OFF, LIGHT_OFF);
                    print4screen("Built in Apps", "Choose a built in app", "________________________________", " <-             Launch             ->");
                    //browseApps();
                }
				/* Load app mode. */
                else if (menu_screen == 1) {
                    controlLights(LIGHT_OFF, LIGHT_ON, LIGHT_OFF);
                    print4screen("Load Apps from SD Card", "It's a cheap app store", "________________________________", " <-             Launch             ->");

                }
				/* Thread Demo Mode. */
                else if (menu_screen == 2) {
                    controlLights(LIGHT_OFF, LIGHT_OFF, LIGHT_ON);
                    print4screen("Thread Demo Mode", "See those threads run", "________________________________", " <-             Launch             ->");


                }
				/* Temp Mode. */
                else if (menu_screen == 3) {
                    controlLights(LIGHT_ON, LIGHT_OFF, LIGHT_OFF);
                    print4screen("Temperature Mode", "How cold is it?", "________________________________", " Back          Launch            ->");

                }
				/* Light Mode. */
                else if (menu_screen == 4) {
                    controlLights(LIGHT_ON, LIGHT_ON, LIGHT_OFF);
                    print4screen("Light Mode", "Turn those lights off", "________________________________", " Back          Launch            ->");

                }
				/* Thread Mode. */
                else if (menu_screen == 5) {
                    controlLights(LIGHT_ON, LIGHT_ON, LIGHT_ON);
                    print4screen("Thread Demo Mode", "Demo of threads", "________________________________", " Back          Launch            ->");

                }
                menu_screen_switch = 0;
            }
        }

        /* Wait and stop screen flickers. */
        delay_ms(100);
    }
}

/*
 * Initializes everything.
 */
void Initialize(void) {

    // Initialize clocks.
    sysclk_init();

    irq_initialize_vectors();
    cpu_irq_enable();

    // Initialize GPIO states.
    board_init();

    // Configure ADC for light sensor.
    configure_adc();

    // Initialize temp sensor, at30tse.
    at30tse_init();

    // Configure IO1 buttons.
    configure_buttons();

    // Initialize Serial Peripheral Interface (SPI) and Screen (SSD1306) controller.
    ssd1306_init();
    ssd1306_clear();
}
/**
 * Thread, displays the light percentage on the screen.
 */
void thread_light() {
    delay_ms(100);
    int itt = 1;
    while (1) {
        if (light_mode != DISABLED) {
            if (itt % 2) {
                getLight();
            } else {
                sprintf(light_disp, "%d", light);
                printStringPosition(light_disp, 0, 106);
                printStringPosition("%", 0, 119);
            }
            itt++;
            delay_ms(65);

            if (light < 20) {
                printStringPosition("VERY DARK", 2, 0);
            } else if (light > 80) {
                printStringPosition("TOO BRIGHT", 2, 0);
            } else {
                printStringPosition("_____________", 2, 0);
            }
        } else {
            printStringPosition("      ", 0, 106);
            printStringPosition("_____________", 2, 0);
        }
    }
}
