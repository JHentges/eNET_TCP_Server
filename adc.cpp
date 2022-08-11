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

#include "eNET-AIO.h"
#include "apcilib.h"

#define RING_BUFFER_SLOTS 255
#define DMA_BUFF_SIZE (BYTES_PER_TRANSFER * RING_BUFFER_SLOTS)

static uint32_t ring_buffer[RING_BUFFER_SLOTS][SAMPLES_PER_TRANSFER];

volatile static int terminate;

pthread_t logger_thread;
pthread_t worker_thread;

pthread_mutex_t mutex;
sem_t empty;
sem_t full;

bool done = false;

void *log_main(void *arg)
{
	int conn = *(int *)arg;
	int ring_read_index = 0;

	while (1)
	{
		sem_wait(&full);
		pthread_mutex_lock(&mutex);

		send(conn, ring_buffer[ring_read_index], (sizeof(uint32_t) * SAMPLES_PER_TRANSFER), 0);

		pthread_mutex_unlock(&mutex);
		sem_post(&empty);

		ring_read_index++;
		ring_read_index %= RING_BUFFER_SLOTS;
	};
}

void *worker_main(void *arg)
{
	int *conn_fd = (int *)arg;
	int num_slots, first_slot, data_discarded, status = 0;

	status = sem_init(&empty, 0, 255);
	status |= sem_init(&full, 0, 0);
	status |= pthread_mutex_init(&mutex, NULL);
	if (status)
	{
		return (void *)(size_t)status;
	}

	void *mmap_addr = (void *)mmap(NULL, DMA_BUFF_SIZE, PROT_READ, MAP_SHARED, apci, 0);
	if (mmap_addr == NULL)
	{
		return (void *)-1;
	}
	try
	{
		pthread_create(&logger_thread, NULL, &log_main, conn_fd);

		while (!done)
		{
			status = apci_dma_data_ready(apci, 1, &first_slot, &num_slots, &data_discarded);
			if ((data_discarded != 0) || status)
			{
				printf("  Worker Thread: first_slot = %d, num_slots = %d, data_discarded = %d; status = %d\n", first_slot, num_slots, data_discarded, status);
			}

			if (num_slots == 0) // Worker Thread: No data pending; Waiting for IRQ
			{
				status = apci_wait_for_irq(apci, 1); // thread blocking
				if (status)
				{
					printf("  Worker Thread: Error waiting for IRQ\n");
					break;
				}
				continue;
			}

			for (int i = 0; i < num_slots; i++)
			{
				sem_wait(&empty);
				pthread_mutex_lock(&mutex);
				memcpy(ring_buffer[(first_slot + i) % RING_BUFFER_SLOTS],
					   mmap_addr + (BYTES_PER_TRANSFER * ((first_slot + i) % RING_BUFFER_SLOTS)),
					   BYTES_PER_TRANSFER);
				pthread_mutex_unlock(&mutex);
				sem_post(&full);
				apci_dma_data_done(apci, 1, 1);
			}
		}
	}
	catch(std::exception e)
	{
		printf(e.what());
	}
	pthread_cancel(logger_thread);
	pthread_join(logger_thread, NULL);
}