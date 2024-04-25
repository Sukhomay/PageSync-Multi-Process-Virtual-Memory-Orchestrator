#include "header.h"

int main(int argc, char *argv[])
{
    if (argc != 6)
    {
        fprintf(stderr, "Error: Insufficient/more number arguments to Process. Exiting...\n");
        exit(1);
    }

    // populating sembuf appropiately for P(s) and V(s)
    pop.sem_num = vop.sem_num = 0;
    pop.sem_flg = vop.sem_flg = 0;
    pop.sem_op = -1;
    vop.sem_op = 1;

    int process_id = atoi(argv[1]);
    int p = atoi(argv[5]);

    int r_i_id = atoi(argv[2]);
    int *ref_string = shmat(r_i_id, NULL, 0);

    int MQ1 = atoi(argv[3]);

    int MQ3 = atoi(argv[4]);

    key_t key = ftok(".", 100 + process_id);
    int sempro_id = semget(key, 1, 0777 | IPC_CREAT);

    key_t key_shared_frame_no = ftok("master.c", 80); // shared frame no
    int shared_frame_no_id = shmget(key_shared_frame_no, sizeof(int), IPC_CREAT | 0666);
    int *shared_frame_no = shmat(shared_frame_no_id, NULL, 0);

    int lock;
    key_t key_lock = ftok("master.c", 2);
    lock = semget(key_lock, 1, 0777 | IPC_CREAT);

    down(lock);
    msg_buffer MQ1_msg;
    MQ1_msg.msg_type = 1;
    MQ1_msg.msg_data = process_id;
    if (msgsnd(MQ1, (void *)&MQ1_msg, sizeof(MQ1_msg), 0) == -1)
    {
        perror("msgsnd");
    }
    up(lock);

    printf("Process %d: I am live.\n", process_id);

    down(sempro_id);

    int idx = 0;
    while (idx < p)
    {
        down(lock);
        int page_id = ref_string[idx];
        mq3_buffer MQ3_msg;
        MQ3_msg.msg_type = 1;
        MQ3_msg.process_id = process_id;
        MQ3_msg.page_id = page_id;
        if (msgsnd(MQ3, (void *)&MQ3_msg, sizeof(MQ3_msg), 0) == -1)
        {
            perror("msgsnd");
        }
        up(lock);

        down(sempro_id);

        if (*shared_frame_no == -2)
        {
            printf("Process %d: Illegal page access. Exiting...\n", process_id);
            shmdt(ref_string);
            shmdt(shared_frame_no);
            exit(1);
        }
        else if (*shared_frame_no == -1)
        {
            down(sempro_id);
        }
        else
        {
            idx++;
        }
    }

    down(lock);

    mq3_buffer MQ3_msg;
    MQ3_msg.msg_type = 1;
    MQ3_msg.process_id = process_id;
    MQ3_msg.page_id = -9;
    if (msgsnd(MQ3, (void *)&MQ3_msg, sizeof(MQ3_msg), 0) == -1)
    {
        perror("msgsnd");
    }
    up(lock);

    printf("Process %d: I am exiting...\n", process_id);

    shmdt(ref_string);
    shmdt(shared_frame_no);

    return 0;
}
