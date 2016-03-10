/*
* minios.h
*
* Contains constant definitions and new types for MiniOS
*
*  Original Author: Rafael Otero
*  Modifiers: Devon Harker, Josh Haskins, Vincent Tennant
*/


#ifndef MINIOS_H
#define MINIOS_H

#include <stdbool.h>

typedef int mStdioType;
typedef int mResult;

mResult openStdio( mStdioType );
mResult closeStdio( mStdioType );
mResult putChar( char );
char getChar( void );
bool receivedChar( void );
void read( char*, int );
void write( const char*, int );
void LEDSet( int, bool );
void LEDToggle( int );
void delayMilli( int );
void delayMicro( int );
void delaySecs( int );
void timerSet(int, void (*) (void) );
void timerStop(void);
long int timerRead(void);

#define LED0			87
#define LED_ON			false
#define LED_OFF			!LED_ON

#define MOS_OK					100
#define MOS_ERROR_UNIMPLEMENTED	101
#define MOS_ERROR_STDIO			102

#define STDIO_USB_CDC	200
#define STDIO_SLCD1		201
#define STDIO_OLED1		202
#define STDIO_UART1		203
#define STDIO_UART2		204

#endif
