/*******************************************************************
 * ex789-prod-con-threads.cpp
 * Producer-consumer synchronisation problem in C++
 *******************************************************************/
#include <cerrno>

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <iostream>

#include <fcntl.h>
#include <unistd.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <semaphore.h>
#include <time.h>
constexpr int PRODUCERS = 2;
constexpr int CONSUMERS = 1;
constexpr int PRODUCED = 10;
int producer_buffer[10];
int curr_index = 0;

struct shared
{
	int buf[10];
	int pos;
};

int main(int argc, char *argv[])
{
	// Seed the random number generator with the current time
	srand((unsigned)time(NULL));
	// Get a random number to use as the shared memory key
	// This ensures that the shared memory key will be different across users and runs
	int shmrand = rand();
	// Generate the final shared memory key
	key_t shmkey = ftok("/dev/null", shmrand); /* valid directory name and a number */
	printf("shmkey for p = %d\n", shmkey);

	int shmid = shmget(shmkey, sizeof(shared), 0644 | IPC_CREAT);
	if (shmid < 0)
	{
		perror("shmget\n");
		exit(1);
	}

	// attach p to shared memory
	shared *c = reinterpret_cast<shared *>(shmat(shmid, NULL, 0));

	auto sem_spaces_name = "sp_pSem-" + std::to_string(shmrand);
	auto sem_items_name = "it_pSem-" + std::to_string(shmrand);

	auto sem_mutex_name = "mut_pSem-" + std::to_string(shmrand);
	auto SEM_SP_CNAME = sem_spaces_name.c_str();
	auto SEM_IT_CNAME = sem_items_name.c_str();

	auto SEM_MUT_CNAME = sem_mutex_name.c_str();
	// initialise a semaphore named "pSem"
	sem_t *sem_sp = sem_open(SEM_SP_CNAME, O_CREAT | O_EXCL, 0644, 10);
	sem_t *sem_it = sem_open(SEM_IT_CNAME, O_CREAT | O_EXCL, 0644, 0);
	sem_t *sem_mut = sem_open(SEM_MUT_CNAME, O_CREAT | O_EXCL, 0644, 1);
	int times = 0;
	int sum = 0;
	if (fork() == 0 || fork() == 0)
	{
		for (size_t i = 0; i < PRODUCED; i++)
		{
			sem_wait(sem_sp);
			sem_wait(sem_mut);
			int produced = rand() % 10 + 1;
			printf("Produce %i\n", produced);
			c->buf[c->pos++] = produced;
			sem_post(sem_mut);
			sem_post(sem_it);
		}
		
			exit(0);
	}
	else
	{
		for (size_t i = 0; i < PRODUCERS * PRODUCED; i++)
		{
			sem_wait(sem_it);
			sem_wait(sem_mut);
			printf("SUMMING....\n");
			sum += c->buf[--c->pos];
			sem_post(sem_mut);
			sem_post(sem_sp);
		}
	}

	while (waitpid(-1, NULL, 0))
	{
		if (errno == ECHILD)
			break;
	}

	printf("\nParent: All children have exited. final sum=%i\n", sum);

	// shared memory detach
	shmdt(c);
	shmctl(shmid, IPC_RMID, 0);

	// cleanup semaphores
	sem_unlink(SEM_IT_CNAME);
	sem_unlink(SEM_MUT_CNAME);
	sem_unlink(SEM_SP_CNAME);
	sem_close(sem_it);
	sem_close(sem_mut);
	sem_close(sem_sp);
	exit(0);
}

void handle_sigint(int sig)
{
	printf("\nCaught SIGINT signal (%d). Exiting gracefully...\n", sig);
	exit(0); // Clean exit
}