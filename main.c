
#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sched.h>
#include <stdint.h>
#include <math.h>
#include <semaphore.h>
#include <sys/timerfd.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

struct timeval tv;

#define MAX 1000
char location[MAX];
long long timestamp;
long buffer[1];
long long time_buffer[1];
sem_t mySem;
int printPipe[2];

long prevLoc=0, prevTS=0;
long nextLoc=0, nextTS = 0;
long buttonTS = 0;


void* Thread0(void*);
void* Child_Thread(void*);
void* Thread1(void*);


int main(void){
	// Initialize Sempaphore for printing
	sem_init(&mySem, 0, 1);

	// Create thread0
	pthread_t thrd0;

	pthread_create(&thrd0, NULL, &Thread0, NULL);

	// Create simple pipe for printing and thread 1 for printing
	pipe(printPipe);

	pthread_t thrd1;

	pthread_create(&thrd1, NULL, &Thread1, NULL);
	
	// Open up named pipe to read GPS data
	int pfd = open("/tmp/N_pipe1", O_RDONLY);

	
	while (1) {

		// Read from N_Pipe1 and store the values in buffer sizeof(char)
		//Store the reading in location[0]
		read(pfd, &location[0], sizeof(char));
		//get time convert it to milliseconds
		struct timespec tspec;
        clock_gettime(CLOCK_MONOTONIC, &tspec);
		timestamp = (long long)((tspec.tv_sec * 1000) + (tspec.tv_nsec / 1000000));
	}

	// Join thread -- won't reach here anyways!
	pthread_join(thrd0, NULL);

	pthread_exit(NULL);
	return 0;
}

// Responsible for reading the real time button press timestamp
void* Thread0(void* ptr) {

    // Read from N_Pipe2 and store the values in time_buffer sizeof(long)
	int pfd = open("/tmp/N_pipe2", O_RDONLY);

	while(1){
		// Thread for child thread
		pthread_t childthrd;
		read(pfd, &buttonTS, sizeof(long));
		prevTS = (long)timestamp;
		prevLoc = (long)location[0];
		pthread_create(&childthrd, NULL, &Child_Thread, NULL);
	}

	pthread_exit(NULL);
}

/**	@brief
 * 	This function is going to interpolate the the position of the button press
 * 	event based on the previous and next GPS time and location information. It
 * 	is going to use the slope intercept form to interpolate the value.
 */
void* Child_Thread(void* ptr){
	long buttonBuf = buttonTS;
	while (1)
	{
		if((long)location[0] != prevLoc) {
			break;
		}
	}
	nextLoc = (long)location[0];
	
	while (1)
	{
		if(prevTS != timestamp) {
			break;
		}
	}
	nextTS 	= (long)timestamp;


	// interpolate the location of the button press event based on the info
	// received through all the threads.
	long double deltaLoc 	= (long double)(nextLoc - prevLoc);
	long double deltaTS		= (long double)(nextTS - prevTS);
	long double slope 		= (long double)(deltaLoc/deltaTS);
	long double tmp 		= ((long double)slope)*((long double)(buttonBuf - prevTS));
	long double y0 			= (long double)tmp + (long double)prevLoc;

	// Now print the information on the screen. We need to be careful in printing
	// the data. As we have 5 threads running concurrently and all of the them 
	// are trying to print on the screen at the same time, we need to restrict
	// the access so that while only one thread printing the info on the screen
	// the rest of the threads will wait for it to finish its printing job. We
	// need a semaphore for that.
	long double output[6];
	output[0] = (long double)prevTS;
	output[1] = (long double)prevLoc;
	output[2] = (long double)buttonBuf;
	output[3] = (long double)y0;
	output[4] = (long double)nextTS;
	output[5] = (long double)nextLoc;
	sem_wait(&mySem);
	write(printPipe[1], output, sizeof(long double) * 6);
	sem_post(&mySem);


	pthread_exit(NULL);
}

// Responsible for printing the interpolation results after reading data from child thread through a simple pipe
void* Thread1(void* ptr) {
	long double print[6];
	while(1){
		read(printPipe[0], print, sizeof(long double) * 6);
		printf("\nInterpolation Results\n");
		printf("\n---------------------\n");
		printf("GPS Previous TS = %lf \t GPS Previous Location = %lf\n", 
			(long double)print[0], (long double)print[1]);
		printf("Button TS = %lf \t Button Location = %lf\n", \
			(long double)print[2], (long double)print[3]);
		printf("GPS Next TS = %lf \t GPS Next Location = %lf\n", \
			(long double)print[4], (long double)print[5]);
		printf("\n\n");
	}

	pthread_exit(NULL);
}