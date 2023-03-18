#include <fcntl.h>
#include <getopt.h>
#include <linux/types.h>
#include <sched.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/timerfd.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>
#include <semaphore.h>
#include <wiringPi.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>

// Path name to named pipe for writing timestamps
const char timestamp[] = "/tmp/N_pipe2";

// Process priority
#define BTN_PRIORITY    (49)

// Timer Functions
// ===============
// Timer related structures and functions:
struct period_info {
    struct timespec next_period;        // sec, nano-sec
    long period_ns;
};


// Thread related Real-time functions
// ========================

static void periodic_task_init(struct period_info *pinfo);
static void inc_period(struct period_info *pinfo);
static void wait_rest_of_period(struct period_info *pinfo);

int main() {
    // Set-up of GPIO button pin using WiringPi library
    wiringPiSetupGpio();

    pinMode(16, INPUT);
    pullUpDnControl(16, PUD_DOWN);

    // Attempt to create pipe
    if (mkfifo(timestamp,0666) != 0)
    {
        if (errno==EEXIST)
        {
            printf("FIFO already exists!\n");
        }
        else
        {
            printf("Timestamp FIFO Creation failed\n");
            exit(-1);
        }
    }
    
    // Create priority for task in scheduler
    struct sched_param param;
    param.sched_priority = BTN_PRIORITY;
    if (sched_setscheduler(0, SCHED_FIFO, &param) == -1) {
        printf("Run the program as a sudo user\n");
 	    perror("sched_setscheduler failed, thread 1");
    	exit(20);
    }

    // Open pipe
    int fd = open(timestamp, O_RDWR);

    // Initialize the periodic Task
	struct period_info pinfo;
	periodic_task_init(&pinfo);
	while(1) {
		if(digitalRead(16) == 1) {
            printf("Button Pressed\n");
            struct timespec tspec;
            clock_gettime(CLOCK_MONOTONIC, &tspec);
            long time = (long)((tspec.tv_sec * 1000) + (tspec.tv_nsec / 1000000));
            write(fd, &time, sizeof(long));
        }
		wait_rest_of_period(&pinfo);
	}
    
    
    return 0;
}

//Write a function to determine the starting time of the thread
static void periodic_task_init(struct period_info *pinfo)
{
        /* for simplicity, hardcoding a 60ms period */
        pinfo->period_ns = 60000000;
 
        clock_gettime(CLOCK_MONOTONIC, &(pinfo->next_period));
}

// Write a function to the determine the ending time of the thread based on the initialized time
static void inc_period(struct period_info *pinfo) 
{
        pinfo->next_period.tv_nsec += pinfo->period_ns;
 
        while (pinfo->next_period.tv_nsec >= 1000000000) {
                /* timespec nsec overflow */
                pinfo->next_period.tv_sec++;
                pinfo->next_period.tv_nsec -= 1000000000;
        }
}

// Write a function to sleep for the remaining time of the period after finishing the task
static void wait_rest_of_period(struct period_info *pinfo)
{
        inc_period(pinfo);
 
        /* for simplicity, ignoring possibilities of signal wakes */
        clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &pinfo->next_period, NULL);
}