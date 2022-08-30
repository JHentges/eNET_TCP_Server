#include <string>
#include <vector>
#include <queue>
#include <sstream>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>	   //close
#include <arpa/inet.h> //close
#include <sys/types.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/time.h> //FD_SET, FD_ISSET, FD_ZERO macros
#include <semaphore.h>
#include <pthread.h>
#include <sys/mman.h>
#include <signal.h>
#include <math.h>
#include <thread>
#include <mutex>
#include <netdb.h>
#include <fcntl.h>

#include "safe_queue.h"
#include "TMessage.h"
#include "TError.h"
#include "eNET-AIO.h"
#include "apcilib.h"
#include "adc.h"

static uint32_t ring_buffer[RING_BUFFER_SLOTS][SAMPLES_PER_TRANSFER];

volatile int AdcStreamTerminate;

pthread_t worker_thread;
pthread_t logger_thread;

pthread_mutex_t mutex;
sem_t empty;
sem_t full;

timespec AdcLogTimeout;

int AdcStreamingConnection = -1;

int AdcLoggerThreadID = -1;
int AdcWorkerThreadID = -1;

int AdcLoggerTerminate = 0;

void *log_main(void *arg)
{
	Trace("Thread started");
	AdcLogTimeout.tv_sec = 1;
	int conn = *(int *)arg;
	int ring_read_index = 0;

	while (! AdcLoggerTerminate)
	{
		clock_gettime(CLOCK_REALTIME, &AdcLogTimeout);
		AdcLogTimeout.tv_sec++;
		if (-1 == sem_timedwait(&full, &AdcLogTimeout)) // TODO: check for other Errors
		{
			if (errno == ETIMEDOUT)
				continue;
			Error("Unexpected error from sem_timedwait(), errno: " + std::to_string(errno) + ", " + strerror(errno));
			break;
		}
		pthread_mutex_lock(&mutex);

		ssize_t sent = send(conn, ring_buffer[ring_read_index], (sizeof(uint32_t) * SAMPLES_PER_TRANSFER), 0);
		pthread_mutex_unlock(&mutex);
		sem_post(&empty);
		Trace("Sent ADC Data "+std::to_string(sent)+" bytes, on ConnectionID: "+std::to_string(conn));

		ring_read_index++;
		ring_read_index %= RING_BUFFER_SLOTS;
	};
	AdcLoggerThreadID = -1;
	Trace("Thread ended");
	return 0;
}

void *worker_main(void *arg)
{
	Trace("Thread started");
	int *conn_fd = (int *)arg;
	int num_slots, first_slot, data_discarded, status = 0;

	status = sem_init(&empty, 0, 255);
	status |= sem_init(&full, 0, 0);
	status |= pthread_mutex_init(&mutex, NULL);
	if (status)
	{
		Error("Mutex failed");
		return (void *)(size_t)status;
	}

	void *mmap_addr = (void *)mmap(NULL, DMA_BUFF_SIZE, PROT_READ, MAP_SHARED, apci, 0);
	if (mmap_addr == NULL)
	{
		Error("mmap failed");
		return (void *)-1;
	}
	try
	{
		if (AdcLoggerThreadID == -1){
			AdcLoggerTerminate = 0;
			Trace("No Logger Thread Found: Starting Logger thread.");
			AdcLoggerThreadID = pthread_create(&logger_thread, NULL, &log_main, conn_fd);
		}
		while (1)
		{
			status = apci_dma_data_ready(apci, 1, &first_slot, &num_slots, &data_discarded);
			if ((data_discarded != 0) || status)
			{
				Error("first_slot: "+std::to_string(first_slot)+ "num_slots:" +
				       std::to_string(num_slots)+ "+data_discarded:"+std::to_string(data_discarded) +"; status: " + std::to_string(status));
			}

			if (num_slots == 0) // Worker Thread: No data pending; Waiting for IRQ
			{
				//Log("no data yet, blocking");
				status = apci_wait_for_irq(apci, 1); // thread blocking
				if (status)
				{
					status = errno;
					if (status != ECANCELED)  // "canceled" is not an error but we do want to close this thread
						Error("  Worker Thread: Error waiting for IRQ; status: " + std::to_string(status) + ", " + strerror(status));
					else
						Trace("  Thread canceled.");

					AdcStreamTerminate = 1;
					AdcLoggerTerminate = 1;
					break;
				}
				continue;
			}
			Trace("Taking ADC Data block(s)");
			for (int i = 0; i < num_slots; i++)
			{
				sem_wait(&empty);
				pthread_mutex_lock(&mutex);
				memcpy(ring_buffer[(first_slot + i) % RING_BUFFER_SLOTS], ((__u8 *)mmap_addr + (BYTES_PER_TRANSFER * ((first_slot + i) % RING_BUFFER_SLOTS))),
					   BYTES_PER_TRANSFER);
				pthread_mutex_unlock(&mutex);
				sem_post(&full);
				apci_dma_data_done(apci, 1, 1);
			}
		}
		Trace("Thread ended");
	}
	catch(std::exception e)
	{
		Error(e.what());
	}
	Trace("Setting AdcStreamingConnection to idle");
	// pthread_cancel(logger_thread);
	pthread_join(logger_thread, NULL);
	Trace("ADC Log Thread exiting.");
	return 0;
}