/*******************************************************************
 * ex789-prod-con-threads.cpp
 * Producer-consumer synchronisation problem in C++
 *******************************************************************/

#include <cstdio>
#include <cstdlib>
#include <pthread.h>
#include <signal.h>

constexpr int PRODUCERS = 2;
constexpr int CONSUMERS = 1;
constexpr int PRODUCTION = 10;

int producer_buffer[10];
int curr_index = 0;
int consumer_sum = 0;

pthread_mutex_t buf_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t nonempty;
pthread_cond_t nonfull;

void *producer(void *threadid)
{
	
	long tid = *(long *)threadid;
	for (int i = 0; i < PRODUCTION; i++)
	{

		pthread_mutex_lock(&buf_lock);
		while (curr_index == 10)
		{
			printf("nonfull: thread %zu curr= %d. Going into wait...\n", tid, curr_index);
			pthread_cond_wait(&nonfull, &buf_lock);
			printf("nonfull: thread %zu curr= %d. signal\n", tid, curr_index);
		}
		int prod = rand() % 10 + 1;
		printf("Produced %i @ %zu\n", prod, tid);
		producer_buffer[curr_index++] = prod;
		pthread_cond_signal(&nonempty);
		if (curr_index != 10)
		{
			pthread_cond_signal(&nonfull);
		}
		pthread_mutex_unlock(&buf_lock);
	}

	// Write producer code here
	pthread_exit(NULL); // terminate thread
}

void *consumer(void *threadid)
{
	
	long tid = *(long *)threadid;
	for (size_t i = 0; i < PRODUCERS * PRODUCTION; i++)
	{
		pthread_mutex_lock(&buf_lock);
		while (curr_index == 0)
		{
			printf("nonempty: thread %zu curr= %d. Going into wait...\n", tid, curr_index);
			pthread_cond_wait(&nonempty, &buf_lock);
			printf("nonempty: thread %zu curr= %d. Going into wait...\n", tid, curr_index);
		}
		consumer_sum += producer_buffer[--curr_index];
		printf("added [%i] -> %i @ %zu\n", curr_index+1, consumer_sum, tid);
		pthread_cond_signal(&nonfull);
		if (curr_index != 0)
		{
			pthread_cond_signal(&nonempty);
		}
		pthread_mutex_unlock(&buf_lock);
		// Write producer code here
	}

	pthread_exit(NULL); // terminate thread
}

void handle_sigint(int sig)
{
	printf("\nCaught SIGINT signal (%d). Exiting gracefully...\n", sig);
	exit(0); // Clean exit
}
int main(int argc, char *argv[])
{
	signal(SIGINT, handle_sigint);
	pthread_t producer_threads[PRODUCERS];
	pthread_t consumer_threads[CONSUMERS];
	int producer_threadid[PRODUCERS];
	int consumer_threadid[CONSUMERS];

	int rc;
	int t1, t2;
	for (t1 = 0; t1 < PRODUCERS; t1++)
	{
		int tid = t1;
		producer_threadid[tid] = tid;
		printf("Main: creating producer %d\n", tid);
		rc = pthread_create(&producer_threads[tid], NULL, producer,
							(void *)&producer_threadid[tid]);
		if (rc)
		{
			printf("Error: Return code from pthread_create() is %d\n", rc);
			exit(-1);
		}
	}

	for (t2 = 0; t2 < CONSUMERS; t2++)
	{
		int tid = t2;
		consumer_threadid[tid] = tid;
		printf("Main: creating consumer %d\n", tid);
		rc = pthread_create(&consumer_threads[tid], NULL, consumer,
							(void *)&consumer_threadid[tid]);
		if (rc)
		{
			printf("Error: Return code from pthread_create() is %d\n", rc);
			exit(-1);
		}
	}

	for (auto i : consumer_threads)
		pthread_join(i, NULL);
	for (auto i : producer_threads)
		pthread_join(i, NULL);

	printf("Final consumer sum: %i\n", consumer_sum);
	pthread_exit(NULL);

	/*
					some tips for this exercise:

					1. you may want to handle SIGINT (ctrl-C) so that your program
									can exit cleanly (by killing all threads, or just calling
		 exit)

					1a. only one thread should handle the signal (POSIX does not define
									*which* thread gets the signal), so it's wise to mask out the
		 signal on the worker threads (producer and consumer) and let the main
		 thread handle it
	*/
}
