/*
 * test.c
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
 *     but WITHOUT ANY WARRANTY; without even the implied warranty of
 *     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *     GNU General Public License for more details.
 * 
 *     You should have received a copy of the GNU General Public License
 *     along with TDDD56. If not, see <http://www.gnu.org/licenses/>.
 * 
 */

#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <stddef.h>
#include <semaphore.h>

#include "test.h"
#include "stack.h"
#include "non_blocking.h"

#define test_run(test)\
  printf("[%s:%s:%i] Running test '%s'... ", __FILE__, __FUNCTION__, __LINE__, #test);\
  test_setup();\
  if(test())\
  {\
    printf("passed\n");\
  }\
  else\
  {\
    printf("failed\n");\
  }\
  test_teardown();

/* Helper function for measurement */
double timediff(struct timespec *begin, struct timespec *end)
{
	double sec = 0.0, nsec = 0.0;
   if ((end->tv_nsec - begin->tv_nsec) < 0)
   {
      sec  = (double)(end->tv_sec  - begin->tv_sec  - 1);
      nsec = (double)(end->tv_nsec - begin->tv_nsec + 1000000000);
   } else
   {
      sec  = (double)(end->tv_sec  - begin->tv_sec );
      nsec = (double)(end->tv_nsec - begin->tv_nsec);
   }
   return sec + nsec / 1E9;
}

typedef int data_t;
#define DATA_SIZE sizeof(data_t)
#define DATA_VALUE 5

#ifndef NDEBUG
int assert_fun(int expr, const char *str, const char *file, const char* function, size_t line)
{
	if(!(expr))
	{
		fprintf(stderr, "[%s:%s:%zu][ERROR] Assertion failure: %s\n", file, function, line, str);
		abort();
		// If some hack disables abort above
		return 0;
	}
	else
		return 1;
}
#endif

stack_t *stack;
data_t data;

#if MEASURE != 0
struct stack_measure_arg
{
	int id;
};
typedef struct stack_measure_arg stack_measure_arg_t;

struct timespec t_start[NB_THREADS], t_stop[NB_THREADS], start, stop;

#if MEASURE == 1
void* stack_measure_pop(void* arg)
{
    stack_measure_arg_t *args = (stack_measure_arg_t*) arg;
    int i;

    clock_gettime(CLOCK_MONOTONIC, &t_start[args->id]);
    for (i = 0; i < MAX_PUSH_POP / NB_THREADS; i++)
	{
		node *n;
		stack_pop(stack,&n);
	}
    clock_gettime(CLOCK_MONOTONIC, &t_stop[args->id]);

    return NULL;
}

#elif MEASURE == 2
void* stack_measure_push(void* arg)
{
	stack_measure_arg_t *args = (stack_measure_arg_t*) arg;
	int i;

	clock_gettime(CLOCK_MONOTONIC, &t_start[args->id]);
	for (i = 0; i < MAX_PUSH_POP / NB_THREADS; i++)
	{
		node *n;
		stack_push(stack, n);
	}
	clock_gettime(CLOCK_MONOTONIC, &t_stop[args->id]);

	return NULL;
}

#elif MEASURE == 3
void* stack_measure_push_pop(void* arg)
{
	stack_measure_arg_t *args = (stack_measure_arg_t*) arg;
	int i;

	clock_gettime(CLOCK_MONOTONIC, &t_start[args->id]);
	for (i = 0; i < MAX_PUSH_POP / NB_THREADS; i++)
	{	
		node *n;
		stack_push(stack, &n);
	}
	for (i = 0; i < MAX_PUSH_POP / NB_THREADS; i++)
	{	
		node *n;
		stack_pop(stack, n);
	}
	clock_gettime(CLOCK_MONOTONIC, &t_stop[args->id]);

	return NULL;
}



#endif
#endif

/* A bunch of optional (but useful if implemented) unit tests for your stack */
void test_init()
{
	// Initialize your test batch
}

void test_setup()
{
	// Allocate and initialize your test stack before each test
	data = DATA_VALUE;

	// Allocate a new stack and reset its values
	stack = stack_alloc();
}

void test_teardown()
{
	// Do not forget to free your stacks after each test
	// to avoid memory leaks
	free(stack);
}

void test_finalize()
{
	// Destroy properly your test batch
}

int test_push_safe()
{
	// Make sure your stack remains in a good state with expected content when
	// several threads push concurrently to it

	// Do some work
	//stack_push(stack,shared);

	// check if the stack is in a consistent state
	int res = assert(stack_check(stack));

	// check other properties expected after a push operation
	// (this is to be updated as your stack design progresses)
	// Now, the test succeeds
	return res && assert(stack->first == 0);
}

int test_pop_safe()
{
	// Same as the test above for parallel pop operation

	// For now, this test always fails
	return 0;
}




#define ABA_NB_THREADS 3
node *n1, *n2, *shr, *temp;
sem_t semaphore[3];


int stack_pop_custom(stack_t *stack, node** elem)
{
	node *old;
	node *old_next = malloc(sizeof(node*));

	do
	{
		old = stack->first;
		memcpy(old_next, old->next, sizeof(node));

		printf("old is %p, first is %p, next is %p\n", old, stack->first, old->next);

		// Dire à Thread 1 de demarrer
		printf("Thread 0 libere sem1\n");
		sem_post(&semaphore[1]);
		// Attendre jusqu'à que ça soit bon
		printf("Thread 0 attend sem0\n");
		sem_wait(&semaphore[0]);

		printf("old is %p, first is %p, next is %p\n", old, stack->first, old->next);
	} while (cas((size_t *)&stack->first, (size_t)old, (size_t)old_next) != (size_t)old) ;

	*elem = old;

	return 0;
}

void *thread_aba(void* arg)
{
	int thread_id = *(int*)arg;

	switch(thread_id)
	{
		case 0:
			printf("START Thread 0\n");
			printf("Thread 0 pops A\n");
			stack_pop_custom(stack,&temp);
			printf("END Thread 0\n");
			break;
		case 1:
			printf("Thread 1 attend sem1\n");
			sem_wait(&semaphore[1]);
	
			printf("START Thread 1\n");
			printf("Thread 1 pops A\n");
			stack_pop(stack,&shr);
			printf("%p\n\n", shr);

			printf("Thread 1 libere sem2\n");
			sem_post(&semaphore[2]);
			printf("Thread 1 attend sem1\n");
			sem_wait(&semaphore[1]);

			printf("t1 pushes A : %p ", shr);
			stack_push(stack, shr);
			printf("Thread 1 libere sem0\n");
			printf("END Thread 1\n");
			sem_post(&semaphore[0]);
			break;

		case 2:
			printf("Thread 2 attend sem2\n");
			sem_wait(&semaphore[2]);
			printf("START Thread 2\n");
			printf("Thread 2 pops B\n");
			stack_pop(stack,&temp);
			printf("Thread 2 libere sem1\n");
			printf("END Thread 2\n");
			sem_post(&semaphore[1]);
			break;
	}
	return NULL;
}

int test_aba()
{
	pthread_t thread[ABA_NB_THREADS];
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

	// Nodes and stack initialization
	n1 = malloc(sizeof(node));
	n2 = malloc(sizeof(node));

	stack_push(stack,n1);
	stack_push(stack,n2);

	// Semaphore initialization
	for(int i=0; i<3; i++)
		sem_init(&semaphore[i], 0, 0);

	printf("\n\n");


	// Thread creation
	int id[ABA_NB_THREADS];
	for(int i=0; i<ABA_NB_THREADS; i++)
	{
		id[i] = i;
		pthread_create(&thread[i], &attr, &thread_aba, (void*) &id[i]);
	}
	
	// Thread join
	for(int i=0; i<ABA_NB_THREADS; i++)
		pthread_join(thread[i], NULL);

	if((stack->first != NULL))
	{
		printf("\nTest ABA success\n");
		return 1;
	}
	else
	{
		printf("\nTest ABA fail\n");
		return 0;
	}
}

// We test here the CAS function
struct thread_test_cas_args
{
	int id;
	size_t* counter;
	pthread_mutex_t *lock;
};
typedef struct thread_test_cas_args thread_test_cas_args_t;

void* thread_test_cas(void* arg)
{
#if NON_BLOCKING != 0
	thread_test_cas_args_t *args = (thread_test_cas_args_t*) arg;
	int i;
	size_t old, local;

	for (i = 0; i < MAX_PUSH_POP; i++)
    {
		do
		{
			old = *args->counter;
			local = old + 1;
			
		#if NON_BLOCKING == 1
		} while (cas(args->counter, old, local) != old);
		
		#elif NON_BLOCKING == 2
		} while (software_cas(args->counter, old, local, args->lock) != old);
#endif
    }
#endif

	return NULL;
}

// Make sure Compare-and-swap works as expected
int test_cas()
{
#if NON_BLOCKING == 1 || NON_BLOCKING == 2
	pthread_attr_t attr;
	pthread_t thread[NB_THREADS];
	thread_test_cas_args_t args[NB_THREADS];
	pthread_mutexattr_t mutex_attr;
	pthread_mutex_t lock;

	size_t counter;

	int i, success;

	counter = 0;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE); 
	pthread_mutexattr_init(&mutex_attr);
	pthread_mutex_init(&lock, &mutex_attr);

	for (i = 0; i < NB_THREADS; i++)
    {
		args[i].id = i;
		args[i].counter = &counter;
		args[i].lock = &lock;
		pthread_create(&thread[i], &attr, &thread_test_cas, (void*) &args[i]);
    }

	for (i = 0; i < NB_THREADS; i++)
    {
		pthread_join(thread[i], NULL);
    }

	success = assert(counter == (size_t)(NB_THREADS * MAX_PUSH_POP));

	if (!success)
	{
		printf("Got %ti, expected %i. ", counter, NB_THREADS * MAX_PUSH_POP);
    }

	return success;
#else
	return 1;
#endif
}

int main(int argc, char **argv)
{
	setbuf(stdout, NULL);
	// MEASURE == 0 -> run unit tests
#if MEASURE == 0
	test_init();

	//test_run(test_cas);

	//test_run(test_push_safe);
	//test_run(test_pop_safe);
	test_run(test_aba);

	test_finalize();
#else
	int i;
	pthread_t thread[NB_THREADS];
	pthread_attr_t attr;
	stack_measure_arg_t arg[NB_THREADS];
	pthread_attr_init(&attr);
	
	stack = malloc(sizeof(stack_t));
	stack->first = NULL;
	
	node* tmp_node = malloc(sizeof(node*));

	#if MEASURE == 1
		for (i = 0; i < MAX_PUSH_POP; i++)
		{
			stack_push(stack, tmp_node);
		}
	#endif
	
	clock_gettime(CLOCK_MONOTONIC, &start);
	for (i = 0; i < NB_THREADS; i++)
	{
		arg[i].id = i;
	#if MEASURE == 1
		pthread_create(&thread[i], &attr, stack_measure_pop, (void*)&arg[i]);
	#else
	#if MEASURE == 2
		pthread_create(&thread[i], &attr, stack_measure_push, (void*)&arg[i]);
	#else
		pthread_create(&thread[i], &attr, stack_measure_push_pop, (void*)&arg[i]);
	#endif
	#endif
    }

	for (i = 0; i < NB_THREADS; i++)
    {
		pthread_join(thread[i], NULL);
    }
	clock_gettime(CLOCK_MONOTONIC, &stop);
		
	// Print out sum
	double sum=0.0;
	for (i = 0; i < NB_THREADS; i++)
	{
		sum += timediff(&t_start[i], &t_stop[i]);
	}
	printf("%f\n",sum);
	
	
#endif

	return 0;
}
