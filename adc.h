#pragma once

// ADC Streaming-related stuff for eNET-AIO Family hardware

#define RING_BUFFER_SLOTS 255
#define DMA_BUFF_SIZE (BYTES_PER_TRANSFER * RING_BUFFER_SLOTS)
extern volatile int AdcStreamTerminate;
void *worker_main(void *arg);
extern pthread_t worker_thread;
extern pthread_t logger_thread;
extern int AdcStreamingConnection;

extern int AdcWorkerThreadID;