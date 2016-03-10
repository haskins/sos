/**
 * \file
 *
 * \brief Startup file for SAM4S.
 *
 * Copyright (c) 2011 - 2012 Atmel Corporation. All rights reserved.
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
 * Startup
 * Configures the SAM4S on boot.
 *
 *  Author: Devon Harker, Josh Haskins, Vincent Tennant
 */ 

#include "exceptions.h"
#include "sam4s.h"
#include "system_sam4s.h"

/* TEMPORARY PATCH FOR SCB */
#define SCB_VTOR_TBLBASE_Pos               29                            /*!< SCB VTOR: TBLBASE Position */
#define SCB_VTOR_TBLBASE_Msk               (1UL << SCB_VTOR_TBLBASE_Pos) /*!< SCB VTOR: TBLBASE Mask */

/* Initialize segments */
extern uint32_t _sfixed;
extern uint32_t _efixed;
extern uint32_t _etext;
extern uint32_t _srelocate;
extern uint32_t _erelocate;
extern uint32_t _szero;
extern uint32_t _ezero;
extern uint32_t _sstack1;
extern uint32_t _estack1;
extern uint32_t _sstack2;
extern uint32_t _estack2;
extern STACK1_SIZE;
extern STACK2_SIZE;
extern void startScheduler(void);
int createThread(void (*startAddress) (void), char* name, int stackSize);
extern void Initialize(void);

/** \cond DOXYGEN_SHOULD_SKIP_THIS */
int main(void);
void thread_light(void);
void thread_temp(void);
/** \endcond */

void __libc_init_array(void);

/* Exception Table */
__attribute__((section(".vectors")))
IntFunc exception_table[] = {

    /* Configure Initial Stack Pointer, using linker-generated symbols */
    (IntFunc) (&_estack1),
    Reset_Handler,

    NMI_Handler,
    HardFault_Handler,
    MemManage_Handler,
    BusFault_Handler,
    UsageFault_Handler,
    0, 0, 0, 0, /* Reserved */
    SVC_Handler,
    DebugMon_Handler,
    0, /* Reserved  */
    PendSV_Handler,
    SysTick_Handler,

    /* Configurable interrupts  */
    SUPC_Handler, /* 0  Supply Controller */
    RSTC_Handler, /* 1  Reset Controller */
    RTC_Handler, /* 2  Real Time Clock */
    RTT_Handler, /* 3  Real Time Timer */
    WDT_Handler, /* 4  Watchdog Timer */
    PMC_Handler, /* 5  PMC */
    EFC0_Handler, /* 6  EFC0 */
	#if defined(ID_EFC1)
    EFC1_Handler, /* 7  EFC1 */
	#else
    Dummy_Handler, /* 7  Reserved */
	#endif
    UART0_Handler, /* 8  UART0 */
    UART1_Handler, /* 9  UART1 */
	#ifdef ID_SMC
    SMC_Handler, /* 10 SMC */
	#else
    Dummy_Handler,
	#endif
    PIOA_Handler, /* 11 Parallel IO Controller A */
    PIOB_Handler, /* 12 Parallel IO Controller B */
	#ifdef ID_PIOC
    PIOC_Handler, /* 13 Parallel IO Controller C */
	#else
    Dummy_Handler,
	#endif
    USART0_Handler, /* 14 USART 0 */
	#ifdef ID_USART1
    USART1_Handler, /* 15 USART 1 */
	#else
    Dummy_Handler,
	#endif
    Dummy_Handler, /* 16 Reserved */
    Dummy_Handler, /* 17 Reserved */
	#ifdef ID_HSMCI
    HSMCI_Handler, /* 18 HSMCI */
	#else
    Dummy_Handler,
	#endif
    TWI0_Handler, /* 19 TWI 0 */
    TWI1_Handler, /* 20 TWI 1 */
    SPI_Handler, /* 21 SPI */
    SSC_Handler, /* 22 SSC */
    TC0_Handler, /* 23 Timer Counter 0 */
    TC1_Handler, /* 24 Timer Counter 1 */
    TC2_Handler, /* 25 Timer Counter 2 */
	#ifdef ID_TC3
    TC3_Handler, /* 26 Timer Counter 3 */
	#else
    Dummy_Handler,
	#endif
	#ifdef ID_TC4
    TC4_Handler, /* 27 Timer Counter 4 */
	#else
    Dummy_Handler,
	#endif
	#ifdef ID_TC5
    TC5_Handler, /* 28 Timer Counter 5 */
	#else
    Dummy_Handler,
	#endif
    ADC_Handler, /* 29 ADC controller */
    DACC_Handler, /* 30 DACC controller */
    PWM_Handler, /* 31 PWM */
    CRCCU_Handler, /* 32 CRC Calculation Unit */
    ACC_Handler, /* 33 Analog Comparator */
    UDP_Handler, /* 34 USB Device Port */
    Dummy_Handler /* 35 not used */
};

/**
 * This is the code that gets called on processor reset.
 * To initialize the device, and call the main() routine.
 */
void Reset_Handler(void) {

    //Configure the process stack pointer
    __set_PSP(&_estack2); //stack grows downwards,
    __ISB();

    // Sets the CPU to use the process stack
	#define MASK_USE_PSP  0x02
    __set_CONTROL(__get_CONTROL() | MASK_USE_PSP);
    __ISB();

    uint32_t *pSrc, *pDest;

    /* Initialize the relocate segment */
    pSrc = &_etext;
    pDest = &_srelocate;

    if (pSrc != pDest) {
        for (; pDest < &_erelocate;) {
            *pDest++ = *pSrc++;
        }
    }

    /* Clear the zero segment */
    for (pDest = &_szero; pDest < &_ezero;) {
        *pDest++ = 0;
    }

    /* Set the vector table base address */
    pSrc = (uint32_t *) & _sfixed;
    SCB->VTOR = ((uint32_t) pSrc & SCB_VTOR_TBLOFF_Msk);

    if (((uint32_t) pSrc >= IRAM_ADDR) && ((uint32_t) pSrc < IRAM_ADDR + IRAM_SIZE)) {
        SCB->VTOR |= 1 << SCB_VTOR_TBLBASE_Pos;
    }

    /* Initialize the C library */
    __libc_init_array();

    //Initializes the hardware for OS use.
    Initialize();

    //Creates threads
    createThread(&main, "main ", 128);
    createThread(&thread_temp, "thread_temp ", 128);
    createThread(&thread_light, "thread_light ", 128);

    //Starts scheduler
    startScheduler();

    //Sets thread mode to execute in unprivileged mode
	#define MASK_UNPRIVILEGED_MODE  0x01
    __set_CONTROL(__get_CONTROL() | MASK_UNPRIVILEGED_MODE);
    __ISB();

    while (1);
}