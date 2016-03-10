/*
 * Threads
 *
 * Contains method to create threads.
 *
 * Original Author: Rafael Otero
 * Modifiers: Devon Harker, Josh Haskins, Vincent Tennant
 *
 */

#ifndef THREADS_H_
#define THREADS_H_

// As of now, implementation is in scheduler.c
// which is system software. Ideally, we need a user-level code
// in a file named thread.c (to keep consistency with naming of other files)
// that call functions from scheduler.c via SVCs

int createThread(  void (*startAddress) (void), char* name, int stackSize );

#endif /* THREADS_H_ */