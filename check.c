
#define logToFile 0
#define sendViaTCP 1


#include <stdio.h>
#include <string.h> //strlen
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>    //close
#include <arpa/inet.h> //close
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/time.h> //FD_SET, FD_ISSET, FD_ZERO macros

#include <signal.h>
#include <math.h>

#define __HELLO__ "eNET Server 0.01\r\n"

#define TRUE 1
#define FALSE 0
//#define PORT 8080

int port = 0;
int samples = 0;
///--------------FPGA---------------------------------------------------

#include <netdb.h>
#include <fcntl.h>
#include <strings.h>

#define AM 1

#ifdef AM
#include "apcilib.h"
#include "eNET-AIO.h"

#define DEVICEPATH "/dev/apci/pcie_adio16_16f_0"
#define BAR_REGISTER 1

int apci = 0;

void setReg_32(int reg, int value)
{

    int rdata = 0;
    int wdata = 0;

    wdata = value;

    apci_read32(apci, 1, BAR_REGISTER, reg, &rdata);
    printf("Before writing  %08X\n", rdata);
    apci_write32(apci, 1, BAR_REGISTER, reg, wdata);
    apci_read32(apci, 1, BAR_REGISTER, reg, &rdata);
    printf("After writing  %08X\n", rdata);
}

int readReg_32(int reg)
{

    uint32_t rdata = 0;

    apci_read32(apci, 1, BAR_REGISTER, reg, &rdata);
    printf("pcie read value is   %08X\n", rdata);
    return rdata;
}

void setReg_8(int reg, int value)
{
    __u8 rdata = 0;
    __u8 wdata = value;

    apci_read8(apci, 1, BAR_REGISTER, reg, &rdata);
    printf("Before writing  %08X\n", rdata);
    apci_write8(apci, 1, BAR_REGISTER, reg, wdata);
    apci_read8(apci, 1, BAR_REGISTER, reg, &rdata);
    printf("After writing  %08X\n", rdata);
}

int readReg_8(int reg)
{

    uint8_t rdata = 0;

    apci_read8(apci, 1, BAR_REGISTER, reg, &rdata);
    printf("pcie read value is   %08X\n", rdata);
    return rdata;
}
#endif

//---------------Fpga->End---------------------------------------------

//---------------DMA----------------------------------------------------
//
//
#include <semaphore.h>
#include <pthread.h>
#include <sys/mman.h>

#define SAMPLE_RATE 10000.0 /* Hz. Note: This is the overall sample rate, sample rate of each channel is SAMPLE_RATE / CHANNEL_COUNT */
#define LOG_FILE_NAME "samples.bin"
#define SECONDS_TO_LOG 40.0
#define START_CHANNEL 0
#define END_CHANNEL 15
#define ADC_RANGE (bmSingleEnded | bmAdcRange_u10V)

/* The rest of this is internal for the sample to use and should not be changed until you understand it all */
double Hz;
uint8_t CHANNEL_COUNT = END_CHANNEL - START_CHANNEL + 1;
#define NUM_CHANNELS (CHANNEL_COUNT)
#define AMOUNT_OF_SAMPLES_TO_LOG (SECONDS_TO_LOG * SAMPLE_RATE)

#define RING_BUFFER_SLOTS 255
static uint32_t ring_buffer[RING_BUFFER_SLOTS][SAMPLES_PER_TRANSFER];
static sem_t ring_sem;
volatile static int terminate;

#define DMA_BUFF_SIZE (BYTES_PER_TRANSFER * RING_BUFFER_SLOTS)
#define NUMBER_OF_DMA_TRANSFERS ((__u32)((AMOUNT_OF_SAMPLES_TO_LOG + SAMPLES_PER_TRANSFER - 1) / SAMPLES_PER_TRANSFER))

volatile uint32_t dmaT = 0;
uint32_t dmaTransfers(int seconds)
{

    uint32_t amount_of_samples_to_log = 0;
    uint32_t dma_trs = 0;
    uint32_t samples_per_transfer = 2046;

    amount_of_samples_to_log = seconds * SAMPLE_RATE;

    dma_trs = (amount_of_samples_to_log + SAMPLES_PER_TRANSFER - 1) / SAMPLES_PER_TRANSFER;

    return dma_trs;
}

pthread_t logger_thread;
pthread_t worker_thread;

void abort_handler(int s)
{
    printf("Caught signal %d\n", s);
    apci_write8(apci, 1, BAR_REGISTER, ofsReset, bmResetEverything);
    usleep(5);

    terminate = 2;
    pthread_join(logger_thread, NULL);
    exit(1);
}
//---------------------------------------------Producer-Consumer---------------------------------------------
//
//
pthread_mutex_t mutex;
sem_t empty;
sem_t full;
__u64 indexcounter = 0;

//--------------------------------------------End->Producer-consumer----------------------------------------
void *log_main(void *arg)
{
    int *conn_fd = (int *)arg;
    int conn = *conn_fd;
#if logToFile == 1
    FILE *out = fopen(LOG_FILE_NAME, "wb");
#endif
    int ring_read_index = 0;
    while (1)
    {
        if (terminate == 2)
        {
            printf("logger thread finished, hence finishing logger thread\n");
            break;
        }

        sem_wait(&full);
        pthread_mutex_lock(&mutex);
#if logToFile == 1
        fwrite(ring_buffer[ring_read_index], sizeof(uint32_t), SAMPLES_PER_TRANSFER, out);
#endif
#if sendViaTCP == 1
        send(conn, ring_buffer[ring_read_index], (sizeof(uint32_t) * SAMPLES_PER_TRANSFER), 0);
#endif
        samples += SAMPLES_PER_TRANSFER;

        pthread_mutex_unlock(&mutex);
        sem_post(&empty);

        ring_read_index++;
        ring_read_index %= RING_BUFFER_SLOTS;
    };
    printf("Recorded %d samples on %d channels at rate %f\n", samples, NUM_CHANNELS, SAMPLE_RATE);
#if logToFile == 1
	fflush(out);
	fclose(out);
#endif
    printf("Reseting FPGA\n");
    apci_write8(apci, 1, BAR_REGISTER, ofsReset, bmResetEverything);
    usleep(5);
}

void *worker_main(void *arg)
{

    int *conn_fd = (int *)arg;

    printf("paras connection fd is %d\n", *conn_fd);

    int status = 0;

    status = sem_init(&empty, 0, 255);
    status |= sem_init(&full, 0, 0);
    status |= pthread_mutex_init(&mutex, NULL);
    if (status)
    {
        printf("  Worker Thread: Unable to init semaphore\n");
        return (void *)(size_t)status;
    }

    // map the DMA destination buffer
    void *mmap_addr = (void *)mmap(NULL, DMA_BUFF_SIZE, PROT_READ, MAP_SHARED, apci, 0);
    if (mmap_addr == NULL)
    {
        printf("  Worker Thread: mmap_addr is NULL\n");
        return (void *)-1;
    }

    pthread_create(&logger_thread, NULL, &log_main, conn_fd);
    printf("  Worker Thread: launched Logging Thread\n");

    volatile int transfer_count = 0;
    int num_slots;
    int first_slot;
    int data_discarded;

    do
    {
        if (terminate == 2)
        {
            printf("terminate command received , hence finishing worker thread\n");
            break;
        }

        status = apci_dma_data_ready(apci, 1, &first_slot, &num_slots, &data_discarded);

        if (data_discarded != 0)
        {
            printf("  Worker Thread: first_slot = %d, num_slots = %d, data_discarded = %d\n", first_slot, num_slots, data_discarded);
        }

        if (num_slots == 0)
        {
            // printf("  Worker Thread: No data pending; Waiting for IRQ\n");
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

        transfer_count += num_slots;
        if (!(transfer_count % (dmaT / 20)))
            printf("  Worker Thread: transfer count == %d / %d\n", transfer_count, dmaT);
    } while (transfer_count < dmaT);

    printf("  Worker Thread: exiting; data acquisition complete.\n");
    terminate = 2;
}

void SetAdcStartRate(int fd, double *Hz)
{
    uint32_t base_clock = AdcBaseClock;
    double targetHz = *Hz;
    uint32_t divisor;
    uint32_t divisor_readback;

    divisor = round(base_clock / targetHz);
    *Hz = base_clock / divisor; /* actual Hz achieved, based on the limitation caused by integer divisors */

    apci_write32(apci, 1, BAR_REGISTER, ofsAdcRateDivisor, divisor);
    apci_read32(apci, 1, BAR_REGISTER, ofsAdcRateDivisor, &divisor_readback);
    printf("  Target ADC Rate is %f\n  Actual rate will be %f (%d÷%d)\n", targetHz, *Hz, base_clock, divisor_readback);
}

void initAdc()
{

    struct sigaction sigIntHandler;
    int status = 0;
    double rate = SAMPLE_RATE;
    uint32_t depth_readback;

    sigIntHandler.sa_handler = abort_handler;
    sigemptyset(&sigIntHandler.sa_mask);
    sigIntHandler.sa_flags = 0;
    sigaction(SIGINT, &sigIntHandler, NULL);
    sigaction(SIGABRT, &sigIntHandler, NULL);




    apci_write8(apci, 1, BAR_REGISTER, ofsReset, bmResetEverything); usleep(5);
    samples = 0;
    uint32_t rev;
    apci_read32(apci, 1, BAR_REGISTER, ofsFPGARevision, &rev);
    printf("FPGA reports Revision = 0x%08X\n", rev);

    apci_write32(apci, 1, BAR_REGISTER, ofsAdcFifoIrqThreshold, FIFO_SIZE);

    SetAdcStartRate(apci, &rate);

    for (int channel = START_CHANNEL; channel <= END_CHANNEL; channel++)
        apci_write8(apci, 1, BAR_REGISTER, ofsAdcRange + channel, ADC_RANGE);

    apci_write8(apci, 1, BAR_REGISTER, ofsAdcStartChannel, START_CHANNEL);
    apci_write8(apci, 1, BAR_REGISTER, ofsAdcStopChannel, END_CHANNEL);

    apci_write32(apci, 1, BAR_REGISTER, ofsSubMuxSelect, bmNoSubMux);

    // since Rate and Trigger are configured already, this will start taking data
    apci_write8(apci, 1, BAR_REGISTER, ofsAdcTriggerOptions, bmAdcTriggerTimer);
}
//
//------------- DMA->End------------------------------------------------

//---------------Tcp/ip-----------------------------------------

void sendAck(int connfd)
{

    char wbuff[3];

    wbuff[0] = '$';
    wbuff[1] = 'A';
    wbuff[2] = '#';

    ssize_t result = write(connfd, wbuff, sizeof(wbuff));
    if (result== -1)
        printf("---ERROR sendAck()'s write() returned %ld", result);
}

void sendNack(int connfd, char code[])
{

    char wbuff[5];

    wbuff[0] = '$';
    wbuff[1] = 'N';
    wbuff[2] = code[0];
    wbuff[3] = code[1];
    wbuff[4] = '#';

    ssize_t result = write(connfd, wbuff, sizeof(wbuff));
    if (result== -1)
        printf("---ERROR sendNack()'s write() returned %ld", result);
}

int validate_hex(char *cbuff, int buff_size)
{

    char *search;
    char valid[] = {'A', 'B', 'C', 'D', 'E', 'F', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9'};

    for (int i = 0; i < buff_size; i++)
    {

        search = strchr(valid, cbuff[i]);

        if (search == NULL)
        {
            printf("Invalid register\n");
            return -1;
        }
    }

    return 0;
}

int validate_packet(char rbuff[], int conn)
{
    int status;
    if (rbuff[0] == '$')
    {

        char reg_offset[2] = {0};
        int register_offset_int = 0;

        char reg_value[12] = {0};
        int register_value_int = 0;

        char adc_offset[2] = {0};
        int adc_offset_int = 0;

        switch (rbuff[1])
        {

        case 'C':

            printf("Set config\n");

            reg_offset[0] = rbuff[4];
            reg_offset[1] = rbuff[5];

            if (validate_hex(reg_offset, 2) == -1)
            {

                return -1;
            }

            sscanf(reg_offset, "%x", &register_offset_int);

            reg_value[0] = rbuff[6];
            reg_value[1] = rbuff[7];
            /*                reg_value[2] = rbuff[8];
                            reg_value[3] = rbuff[9];
                            reg_value[4] = rbuff[10];
                            reg_value[5] = rbuff[11];
                            reg_value[6] = rbuff[12];
                            reg_value[7] = rbuff[13];;
            */
            /*if(validate_hex(reg_value,8)==-1){

                return -1;
            }

            sscanf(reg_value, "%x", &register_value_int);

*/
            printf("Register is 0x%x\n", register_offset_int);
            // printf("Register_value is 0x%8X\n",register_value_int);

            switch (rbuff[3])
            {

            case 'P':
                printf("8 bit write\n");

                if (validate_hex(reg_value, 2) == -1)
                {

                    return -1;
                }

                sscanf(reg_value, "%x", &register_value_int);

                printf("8bit value is %x\n", register_value_int);

                // apci_write8(apci, 1, BAR_REGISTER, reg, wdata);
                setReg_8(register_offset_int, register_value_int);
                sendAck(conn);
                break;

            case 'J':
                printf("32 bit wtite \n");

                reg_value[0] = rbuff[6];
                reg_value[1] = rbuff[7];
                reg_value[2] = rbuff[8];
                reg_value[3] = rbuff[9];
                reg_value[4] = rbuff[10];
                reg_value[5] = rbuff[11];
                reg_value[6] = rbuff[12];
                reg_value[7] = rbuff[13];
                ;

                if (validate_hex(reg_value, 8) == -1)
                {

                    return -1;
                }

                sscanf(reg_value, "%x", &register_value_int);

                setReg_32(register_offset_int, register_value_int);
                sendAck(conn);
                break;

            default:
                printf("invalid packet size\n");
            }

            break;

        case 'R':
            printf("Read config\n");

            reg_offset[0] = rbuff[4];
            reg_offset[1] = rbuff[5];

            if (validate_hex(reg_offset, 2) == -1)
            {

                return -1;
            }

            sscanf(reg_offset, "%x", &register_offset_int);

            printf("Register is %x\n", register_offset_int);

            switch (rbuff[3])
            {

            case 'P':
                printf("8 bit read\n");

                uint8_t reg_read_Value8 = 0;
                reg_read_Value8 = readReg_8(register_offset_int);

                printf("Read value of register %x is %x\n", register_offset_int, reg_read_Value8);
                char sbuff8[12] = {0};
                sprintf(sbuff8, "%x", reg_read_Value8);

                char cbuff8[3] = {'$', 'B', 'F'};
                char end8 = '#';

                send(conn, cbuff8, sizeof(cbuff8), 0);
                send(conn, sbuff8, sizeof(sbuff8), 0);
                send(conn, &end8, sizeof(end8), 0);

                break;

            case 'J':
                printf("32 bit read \n");
                int reg_read_Value = 0;
                reg_read_Value = readReg_32(register_offset_int);

                printf("Read value of register %x is %x\n", register_offset_int, reg_read_Value);
                char sbuff[12] = {0};
                sprintf(sbuff, "%x", reg_read_Value);

                char cbuff[3] = {'$', 'B', 'F'};
                char end = '#';

                send(conn, cbuff, sizeof(cbuff), 0);
                send(conn, sbuff, sizeof(sbuff), 0);
                send(conn, &end, sizeof(end), 0);
                break;

            default:
                printf("invalid packet size\n");
            } // END SWITCH for CFx where x is bit-depth specifier

            break;

        case 'Y':
            printf("Read adc\n");

            printf("conn fd from parser %d\n", conn);

            adc_offset[0] = rbuff[2];
            adc_offset[1] = rbuff[3];

            if (validate_hex(adc_offset, 2) == -1)
            {
                return -1;
            }

            sscanf(adc_offset, "%x", &adc_offset_int);

            dmaT = dmaTransfers(adc_offset_int);
            status = apci_dma_transfer_size(apci, 1, RING_BUFFER_SLOTS, BYTES_PER_TRANSFER);
            if (status)
            {
                printf("Error setting apci_dma_transfer_size=%d\n", status);
                return -1;
            }
            initAdc();
            terminate = 0;

            pthread_create(&worker_thread, NULL, &worker_main, &conn);
            sleep(1);
            apci_start_dma(apci);

            break;
        case 'Z':
            printf("Read adc\n");

            printf("conn fd from parser %d\n", conn);

            adc_offset[0] = rbuff[2];
            adc_offset[1] = rbuff[3];

            if (validate_hex(adc_offset, 2) == -1)
            {
                return -1;
            }

            sscanf(adc_offset, "%x", &adc_offset_int);

            dmaT = dmaTransfers(adc_offset_int);
            status = apci_dma_transfer_size(apci, 1, RING_BUFFER_SLOTS, BYTES_PER_TRANSFER);
            if (status)
            {
                printf("Error setting apci_dma_transfer_size=%d\n", status);
                return -1;
            }
            //initAdc();
            terminate = 0;

            pthread_create(&worker_thread, NULL, &worker_main, &conn);
            sleep(1);
            apci_start_dma(apci);

            break;
        case 'T':
            printf("stop continious adc\n");
            terminate = 2;
            break;

        default:
            printf("Invalid Request packet \n");
            break;
        }
    }
    else
    {
        printf("Error first value is not $\n");
    }
}

//------------------- Signal---------------------------

int client_socket[30];
int max_clients = 30;

static void sig_handler(int sig)
{
    printf("i am going\n");

    close(apci);

    exit(0);
}

//---------------------End signal -----------------------

//-----------------------Tcp/ip->End--------------------------------

int main(int argc, char *argv[])
{

    printf(__HELLO__);
    if (argc < 2)
    {

        printf("Error no tcp port specified\n");

        exit(-1);
    }
    sscanf(argv[1], "%d", &port);

    // signal(SIGINT, sig_handler);

    apci = open(DEVICEPATH, O_RDONLY);

    if (apci < 0)
    {

        printf("Error cannot open Device file Please ensure the APCI driver module is loaded \n");
    }

    initAdc();

    int opt = TRUE;
    int master_socket, addrlen, new_socket, activity, i, valread, sd;
    int max_sd;
    struct sockaddr_in address;

    char buffer[1025]; // data buffer of 1K

    // set of socket descriptors
    fd_set readfds;

    // a message
    char *message = __HELLO__;

    // initialise all client_socket[] to 0 so not checked
    for (i = 0; i < max_clients; i++)
    {
        client_socket[i] = 0;
    }

    // create a master socket
    if ((master_socket = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // set master socket to allow multiple connections ,
    // this is just a good habit, it will work without this
    if (setsockopt(master_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt,
                   sizeof(opt)) < 0)
    {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    // type of socket created
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    // bind the socket to localhost port 8888
    if (bind(master_socket, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    printf("Listener on port %d \n", port);

    // try to specify maximum of 3 pending connections for the master socket
    if (listen(master_socket, 3) < 0)
    {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    // accept the incoming connection
    addrlen = sizeof(address);
    puts("Waiting for connections ...");

    while (TRUE)
    {
        // clear the socket set
        FD_ZERO(&readfds);

        // add master socket to set
        FD_SET(master_socket, &readfds);
        max_sd = master_socket;

        // add child sockets to set
        for (i = 0; i < max_clients; i++)
        {
            // socket descriptor
            sd = client_socket[i];

            // if valid socket descriptor then add to read list
            if (sd > 0)
                FD_SET(sd, &readfds);

            // highest file descriptor number, need it for the select function
            if (sd > max_sd)
                max_sd = sd;
        }

        // wait for an activity on one of the sockets , timeout is NULL ,
        // so wait indefinitely
	for(int i=0; i<max_sd+1; i++) printf("before fds_bits[%d] = %ld\n", i, readfds.__fds_bits[i]);

        activity = select(max_sd + 1, &readfds, NULL, NULL, NULL);

        if ((activity < 0) && (errno != EINTR))
        {
            printf("select error");
        }

        // If something happened on the master socket ,
        // then its an incoming connection
        if (FD_ISSET(master_socket, &readfds))
        {
            if ((new_socket = accept(master_socket,
                                     (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0)
            {
                perror("accept");
                exit(EXIT_FAILURE);
            }

            // inform user of socket number - used in send and receive commands
            printf("New connection , socket fd is %d , ip is : %s , port : %d \n", new_socket, inet_ntoa(address.sin_addr), ntohs(address.sin_port));

            // add new socket to array of sockets
            for (i = 0; i < max_clients; i++)
            {
                // if position is empty
                if (client_socket[i] == 0)
                {
                    client_socket[i] = new_socket;
                    printf("Adding to list of sockets as %d\n", i);

                    break;
                }
            }
        }

        // else its some IO operation on some other socket
        for (i = 0; i < max_clients; i++)
        {
            sd = client_socket[i];

            if (FD_ISSET(sd, &readfds))
            {
                // Check if it was for closing , and also read the
                // incoming message
                if ((valread = read(sd, buffer, 1024)) == 0)
                {
                    // Somebody disconnected , get his details and print
                    getpeername(sd, (struct sockaddr *)&address,
                                (socklen_t *)&addrlen);
                    printf("Host disconnected , ip %s , port %d \n",
                           inet_ntoa(address.sin_addr), ntohs(address.sin_port));

                    // Close the socket and mark as 0 in list for reuse
                    close(sd);
                    client_socket[i] = 0;
                }

                // Echo back the message that came in
                else
                {
                    if (validate_packet(buffer, sd) == -1)
                    {

                        printf("packet validation failed hence sending NACK\n");
                        sendNack(sd, "00");
                    }
                    else
                    {

                        printf("Packet is valid hence sending Respose \n");
                        // sendAck(sd);
                    }
                }
            }
        }
    }

    return 0;
}
