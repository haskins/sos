/*
 * System Calls
 *
 * This contains all system wide calls.
 *
 * Authors: Devon Harker, Josh Haskins, Vincent Tennant
 *
 */

#include "minios.h"
#include "conf_usb.h"
#include <asf.h>
#include "sysnums.h"
#include "data.h"

void MOSTimerSet(int, void (*) (void));
void MOSTimerStop(void);
long int MOSTimerRead(void);
void MOSLEDSet(int, bool);
void MOSLEDToggle(int);
void MOSDelayMilli(int);
void MOSDelayMicro(int);
void MOSDelaySecs(int);
mResult MOSOpenStdio(mStdioType);
mResult MOSCloseStdio(mStdioType);
mResult MOSPutc(char);
char MOSGetc(void);
bool MOSReceivedChar(void);
void MOSRead(char*, int);
void MOSWrite(const char*, int);
void SVC_Switch(unsigned int *);
void SVC_Error(int);

//////////////////////////////////////////////////////////////////////////
//							SVC Handler									//
//////////////////////////////////////////////////////////////////////////

/*  
 * Copies either the PSP or the MSP address into the stack, then calls
 * SVC_Switch.
 */
void SVC_Handler(void) {
    asm (
                "tst lr, #4\t\n" /* Check EXC_RETURN[2] */
                "ite eq\t\n"
                "mrseq r0, msp\t\n"
                "mrsne r0, psp\t\n"
                "b %[SVC_Switch]\t\n"
                : /* no output */
                : [SVC_Switch] "i" (SVC_Switch) /* input */
                : "r0" /* clobber */
                );
}

/*  
 * Uses the passed argument (which is actually the R0 register value)
 * to switch on and call the corresponding case.
 */
void SVC_Switch(unsigned int * svc_args) {

    /* The value that this switch statement switches on is svc_args, but using only 
     * the value stored in the R0 register.
     */
    switch (((char *) svc_args[6])[-2]) {
            //for LED0 on/off
        case SYSCALL_LED0_OFF: MOSLEDSet(LED0, LED_OFF);
            break;
        case SYSCALL_LED0_ON: MOSLEDSet(LED0, LED_ON);
            break;

            //for LED1 on/off
        case SYSCALL_LED1_OFF: ioport_set_pin_level(IO1_LED1_PIN, LED_OFF);
            break;
        case SYSCALL_LED1_ON: ioport_set_pin_level(IO1_LED1_PIN, LED_ON);
            break;

            //for LED2 on/off
        case SYSCALL_LED2_OFF: ioport_set_pin_level(IO1_LED2_PIN, LED_OFF);
            break;
        case SYSCALL_LED2_ON: ioport_set_pin_level(IO1_LED2_PIN, LED_ON);
            break;

            //for LED3 on/off
        case SYSCALL_LED3_OFF: ioport_set_pin_level(IO1_LED3_PIN, LED_OFF);
            break;
        case SYSCALL_LED3_ON: ioport_set_pin_level(IO1_LED3_PIN, LED_ON);
            break;

        case SYSCALL_WRITECHARTOSCREEN:
            ssd1306_set_page_address(0); //changes line number (0-3)
            ssd1306_set_column_address(0); //change line position (128 pixels wide, you can choose 0-127)
            ssd1306_write_text((char*) svc_args[0]);
            break;

        case SYSCALL_WRITESTRINGTOSCREEN:
            ssd1306_set_page_address((int) svc_args[1]); //changes line number (0-3)
            ssd1306_set_column_address(0); //change line position (128 pixels wide, you can choose 0-127)
            ssd1306_write_text((char*) svc_args[0]);
            break;

        case SYSCALL_GETTEMP:
            at30tse_read_temperature((double*) svc_args[0]);
            break;

        case SYSCALL_GETLIGHT:
            adc_value = adc_get_channel_value(ADC, ADC_CHANNEL_4);
            break;

        case SYSCALL_WRITESTRINGTOSCREENPOSITION:
            ssd1306_set_page_address((int) svc_args[1]); //changes line number (0-3)
            ssd1306_set_column_address((int) svc_args[2]); //change line position (128 pixels wide, you can choose 0-127)
            ssd1306_write_text((char*) svc_args[0]);
            break;

        case SYSCALL_DELAY:
            delay_ms((int) svc_args[0]);
            break;

        case SYSCALL_CLEARSCREEN:
            ssd1306_clear();
            break;

        default: 
            ssd1306_set_page_address(0); //changes line number (0-3)
            ssd1306_set_column_address(0); //change line position (128 pixels wide, you can choose 0-127)
            ssd1306_write_text("Some SVC Error Happened");
            break;
    }
}

/*  
 * Signals to the user that there is a problem with the SVC call and that
 * the default case has been tripped.
 */
void SVC_Error(int ledNum) {
    int c;
    for (c = 0; c < 10; c++) {
        MOSLEDToggle(ledNum);
        MOSDelaySecs(1);
    }
}



//////////////////////////////////////////////////////////////////////////
//							REAL-TIME TIMER								//
//////////////////////////////////////////////////////////////////////////


//declaration of callback function
void (*pTimerCallback) (void);

RAMFUNC;

void MOSTimerSet(int tickPeriodMs, void (*pFunc) (void)) {

    //registers the callback function
    pTimerCallback = pFunc;


    // Sets the RTT to generate a tick which triggers
    // the Real-Time Increment Interrupt (RTTINC)

    // The prescaler period (the tick) is equal to:
    //               tickPeriod * SCLK period.
    //			=>  (tickPeriodMs/1000) * SCLK Period

    // Slow Clock (SCLK) = 32Khz (Generated by the Crystal or RC OScillator)
    // Max prescaler value (tickePeriod) is 2^16 = 65536
    //			=> Max tickPeriod = 2 seconds
    uint32_t ul_previous_time;

    rtt_sel_source(RTT, false); //source is not rtc
    rtt_init(RTT, (int) (((double) tickPeriodMs / (double) 1000) * BOARD_FREQ_SLCK_XTAL)); //Configure tick time

    //Not sure why this has to be here... but I'll leave it
    //Looks as it's waiting for 1 tick to occur
    //Check this later....
    ul_previous_time = rtt_read_timer_value(RTT);
    while (ul_previous_time == rtt_read_timer_value(RTT));

    //Clear previous on-going ints?
    NVIC_DisableIRQ(RTT_IRQn);
    NVIC_ClearPendingIRQ(RTT_IRQn);
    //Sets new int
    NVIC_SetPriority(RTT_IRQn, 0);
    NVIC_EnableIRQ(RTT_IRQn);
    rtt_enable_interrupt(RTT, RTT_MR_RTTINCIEN);
}

RAMFUNC;

void RTT_Handler(void) {
    uint32_t ul_status;

    //get rtt status
    ul_status = rtt_get_status(RTT);

    //Check whether the Timer has been incremented since the last read
    if ((ul_status & RTT_SR_RTTINC) == RTT_SR_RTTINC) {
        //It doesn't mutates any register!!
        //Why do we need this if-statement then? I dont know!!
        //but if we dont have it, the USB communication dies...	
        pTimerCallback();
    }
}

void MOSTimerStop(void) {
    //Not too sure the different between rtt_disable and NVUC_Disable IRQ
    //Check this later....
    NVIC_DisableIRQ(RTT_IRQn);
    NVIC_ClearPendingIRQ(RTT_IRQn);

    rtt_disable_interrupt(RTT, RTT_MR_RTTINCIEN);
}

long int MOSTimerRead(void) {
    return rtt_read_timer_value(RTT);
}

//////////////////////////////////////////////////////////////////////////
//								LED0									//
//////////////////////////////////////////////////////////////////////////

static volatile bool ledState = LED_OFF; //LED starts off

void MOSLEDSet(int ledNum, bool state) {
    return ioport_set_pin_level(ledNum, ledState = state);
}

void MOSLEDToggle(int ledNum) {
    return ioport_set_pin_level(ledNum, ledState = !ledState);
}

//////////////////////////////////////////////////////////////////////////
//								DELAY									//
//////////////////////////////////////////////////////////////////////////

void MOSDelayMilli(int timeM) {
    return delay_ms(timeM);
}

void MOSDelayMicro(int timeU) {
    return delay_us(timeU);
}

void MOSDelaySecs(int timeS) {
    return delay_s(timeS);
}

//////////////////////////////////////////////////////////////////////////
//								STDIO									//
//////////////////////////////////////////////////////////////////////////

static bool my_flag_autorize_cdc_transfert = false;

mResult MOSOpenStdio(mStdioType type) {
    switch (type) {
        case STDIO_USB_CDC: udc_start();
            UDC_VBUS_EVENT(true);
            break;
        default: return MOS_ERROR_UNIMPLEMENTED;
    }
    return MOS_OK;
}

mResult MOSCloseStdio(mStdioType type) {
    switch (type) {
        case STDIO_USB_CDC: UDC_VBUS_EVENT(false);
            break;
        default: return MOS_ERROR_UNIMPLEMENTED;
    }
    return MOS_OK;
}

mResult MOSPutc(char c) {
    if (my_flag_autorize_cdc_transfert) {
        while (!udi_cdc_is_tx_ready); //waits till tx is ready
        return udi_cdc_putc(c) ? MOS_OK : MOS_ERROR_STDIO;
    }
    return MOS_ERROR_STDIO;
}

char MOSGetc(void) {
    return udi_cdc_getc(); //Halts till a character is received
}

bool MOSReceivedChar(void) {
    return udi_cdc_is_rx_ready();
}

void MOSRead(char* buf, int bufSz) {
    udi_cdc_read_buf(buf, bufSz); //Halts till bufSz characters are received	
}

void MOSWrite(const char* buf, int bufSz) {
    udi_cdc_write_buf(buf, bufSz);
}

//These functions are specific to the USB Stack implementation
//------------------------------------------------------------

bool my_callback_cdc_enable(void) {
    my_flag_autorize_cdc_transfert = true;
    return true;
}

void my_callback_cdc_disable(void) {
    my_flag_autorize_cdc_transfert = false;
}

void user_callback_vbus_action(bool b_high) {
    if (b_high) {
        // Attach USB Device
        udc_attach();
    } else {
        // Vbus not present
        udc_detach();
    }
}