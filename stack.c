/*
 * stack.c
 *
 *  Created on: 18 Oct 2011
 *  Copyright 2011 Nicolas Melot
 *
 * This file is part of TDDD56.
 * 
 *     TDDD56 is free software: you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation, either version 3 of the License, or
 *     (at your option) any later version.
 * 
 *     TDDD56 is distributed in the hope that it will be useful,
 *     but WITHOUT ANY WARRANTY without even the implied warranty of
 *     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *     GNU General Public License for more details.
 * 
 *     You should have received a copy of the GNU General Public License
 *     along with TDDD56. If not, see <http://www.gnu.org/licenses/>.
 * 
 */

#ifndef DEBUG
#define NDEBUG
#endif

#include <assert.h>
#include <pthread.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#include "stack.h"
#include "non_blocking.h"

#if NON_BLOCKING == 0
	#warning Stacks are synchronized through locks
	pthread_mutex_t mutex_stack = PTHREAD_MUTEX_INITIALIZER;
#else
#if NON_BLOCKING == 1
	#warning Stacks are synchronized through hardware CAS
#else
	#warning Stacks are synchronized through lock-based CAS
#endif
#endif

int stack_check(stack_t *stack)
{
// Do not perform any sanity check if performance is bein measured
#if MEASURE == 0
	// Use assert() to check if your stack is in a state that makes sens
	// This test should always pass 
	assert(1 == 1);

	// This test fails if the task is not allocated or if the allocation failed
	assert(stack != NULL);
#endif
	// The stack is always fine
	return 1;
}


stack_t* stack_alloc()
{
	stack_t *res;

	res = malloc(sizeof(stack_t));
	assert(res != NULL);

	if (res == NULL)
		return NULL;

	return res;
}


int stack_push(stack_t *stack, node *elem)
{
#if NON_BLOCKING == 0
	// Implement a lock_based stack

	pthread_mutex_lock(&mutex_stack);
	
	elem->next = stack->first;
	stack->first = elem;
		
	pthread_mutex_unlock(&mutex_stack);
	
#elif NON_BLOCKING == 1
	// Implement a harware CAS-based stack
	
	node *old;

	do{
		old = stack->first;
		elem->next = old;
	}while(cas( (size_t*)&(stack->first), (size_t) old,  (size_t)elem) != (size_t)old);
	
#else
	/*** Optional ***/
	// Implement a software CAS-based stack
	#warning Not implemented
#endif

	return 0;
}

int stack_pop(stack_t *stack,node **elem)
{
	if(stack->first == NULL)
	{
    	return -1;
	}

#if NON_BLOCKING == 0
	// Implement a lock_based stack
	
	pthread_mutex_lock(&mutex_stack);
	
	*elem = stack->first;
	stack->first = stack->first->next;
	
	pthread_mutex_unlock(&mutex_stack);
	
#elif NON_BLOCKING == 1
	// Implement a harware CAS-based stack
	
	node *old;
	node *next;
	
	do{
		old = stack->first;
		next = old->next;		
	}while(cas((size_t*)&(stack->first), (size_t) old, (size_t) next) != (size_t)old);
	
	*elem = old;

#else
	/*** Optional ***/
	// Implement a software CAS-based stack
	#warning Not implemented
#endif

	return 0;
}

