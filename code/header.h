#include <stdio.h> 
#include <sys/ipc.h> 
#include <sys/msg.h> 
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <sys/sem.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <errno.h>
#include <sys/shm.h>
#include <signal.h>
#include <fcntl.h>
#include <semaphore.h>

// Definition of probability for illegal memory access
#define prob_illegal_access 0.001

// Structure representing a page in memory
typedef struct
{
    int is_valid; // Flag indicating if the page is valid or not
    int fno;      // Frame number associated with the page
} page;

// Structure representing a message for the third message queue
typedef struct 
{
    long msg_type;    // Type of message
    int process_id;   // Process ID
    int page_id;      // Page ID
} mq3_buffer;

// Structure representing a general message buffer
typedef struct
{
    long msg_type;    // Type of message
    int msg_data;     // Data associated with the message
} msg_buffer;

// Structure representing statistics for page faults and invalid accesses
typedef struct{
    int tot_page_faults;      // Total number of page faults
    int tot_invalid_access;   // Total number of invalid memory accesses
} stat;

// Semaphore operations
struct sembuf pop, vop;
#define down(s) semop(s, &pop, 1) // wait(s)
#define up(s) semop(s, &vop, 1)   // signal(s)

// ANSI color codes for printing
#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_RESET   "\x1b[0m"

