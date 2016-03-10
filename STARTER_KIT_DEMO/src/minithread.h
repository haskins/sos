/*
* Minithread
*
* Data structure for thread.
*
*  Authors: Devon Harker, Josh Haskins, Vincent Tennant
*/


#include <asf.h>

typedef struct{
	char* name;
	uint32_t* sp;
	uint32_t* bp;
	bool execFirstTime;
	bool alive;
}Minithread;