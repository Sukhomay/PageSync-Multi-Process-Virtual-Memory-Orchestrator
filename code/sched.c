#include "header.h"

int main(int argc, char *argv[])
{
    if (argc != 4)
    {
        fprintf(stderr, "Error: Insufficientmore number arguments to Scheduler. Exiting...\n");
        exit(1);
    }

    // populating sembuf appropiately for P(s) and V(s)
    pop.sem_num = vop.sem_num = 0;
    pop.sem_flg = vop.sem_flg = 0;
    pop.sem_op = -1;
    vop.sem_op = 1;

    int k = atoi(argv[1]);

    int MQ1 = atoi(argv[2]);

    int MQ2 = atoi(argv[3]);

    int sem_process[k];
    int key_ref = 100;
    for (int i = 0; i < k; i++)
    {
        key_t key = ftok(".", key_ref + i);
        sem_process[i] = semget(key, 1, 0777 | IPC_CREAT);
    }

    int lock;
    key_t key_lock = ftok("master.c", 2);
    lock = semget(key_lock, 1, 0777 | IPC_CREAT);

    int ndone = 0;
    int curr_process = -1;
    printf("Scheduler: I am ready.\n");
    while (ndone < k)
    {
        msg_buffer MQ1_msg;
        msgrcv(MQ1, (void *)&MQ1_msg, sizeof(MQ1_msg), 0, 0);
        curr_process = MQ1_msg.msg_data;
        printf("Scheduler: Scheduling process %d\n", curr_process);
        up(sem_process[curr_process]);

        msg_buffer MQ2_msgs;
        msgrcv(MQ2, (void *)&MQ2_msgs, sizeof(MQ2_msgs), 0, 0);
        int MQ2_msg = MQ2_msgs.msg_data;
        if (MQ2_msg == 1)
        {
            down(lock);
            msg_buffer msg;
            msg.msg_type = 1;
            msg.msg_data = curr_process;
            if (msgsnd(MQ1, (void *)&msg, sizeof(msg), 0) == -1)
            {
                perror("msgsnd");
            }
            up(lock);
        }
        else if (MQ2_msg == 2)
        {
            ndone++;
            if (ndone == k)
            {
                printf("Scheduler: All processes done. Exiting...\n");
                break;
            }
        }
        else
        {
            fprintf(stderr, "Error: Unknown message received in MQ2. Exiting...\n");
            exit(1);
        }
    }

    int sm_key = ftok("master.c", 1);
    int sem_done = semget(sm_key, 1, 0777 | IPC_CREAT);

    up(sem_done);
}