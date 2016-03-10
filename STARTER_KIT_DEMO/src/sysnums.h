/*
 * System Call Numbers
 *
 * This contains all system wide call numbers.
 *
 * Authors: Devon Harker, Josh Haskins, Vincent Tennant
 *
 */

#ifndef SYSNUMS_H_
#define SYSNUMS_H_

//SVC Params
#define SYSCALL_LED0_OFF		0
#define SYSCALL_LED0_ON			1
#define SYSCALL_LED1_OFF		2
#define SYSCALL_LED1_ON			3
#define SYSCALL_LED2_OFF		4
#define SYSCALL_LED2_ON			5
#define SYSCALL_LED3_OFF		6
#define SYSCALL_LED3_ON			7
#define SYSCALL_WRITECHARTOSCREEN 18
#define SYSCALL_WRITESTRINGTOSCREEN 19
#define SYSCALL_GETTEMP				8
#define SYSCALL_GETLIGHT			9
#define SYSCALL_WRITESTRINGTOSCREENPOSITION 20
#define SYSCALL_DELAY			21
#define SYSCALL_CLEARSCREEN			22

#endif /* SYSNUMS_H_ */