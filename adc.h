#pragma once

#define RING_BUFFER_SLOTS 255
#define DMA_BUFF_SIZE (BYTES_PER_TRANSFER * RING_BUFFER_SLOTS)
extern volatile int terminate;
void *worker_main(void *arg);
extern pthread_t worker_thread;
extern int AdcStreamingConnection;