#include "header.h"

int timestamp = 0;
int fd;
page *sm1;
int *sm2, *M, *start_alloc, *end_alloc, *shared_frame_no;
stat *pro_stat;
int k, m, f;
int sem_done;

void PFH(int m, int st_f, int en_f, int process_id, int page_id, int curr_timestamp)
{
    int least_fno = st_f;
    for (int i = st_f; i <= en_f; i++)
    {
        if (sm2[i] < sm2[least_fno])
        {
            least_fno = i;
        }
    }

    for (int i = 0; i < m; i++)
    {
        if (sm1[process_id * m + i].fno == least_fno)
        {
            sm1[process_id * m + i].is_valid = 0;
        }
    }

    sm2[least_fno] = curr_timestamp;
    sm1[process_id * m + page_id].fno = least_fno;
    sm1[process_id * m + page_id].is_valid = 1;
}

int main(int argc, char *argv[])
{
    if (argc != 8)
    {
        fprintf(stderr, "Error: Insufficient/more number arguments to MMU.\nExiting...\n");
        exit(1);
    }

    // populating sembuf appropiately for P(s) and V(s)
    pop.sem_num = vop.sem_num = 0;
    pop.sem_flg = vop.sem_flg = 0;
    pop.sem_op = -1;
    vop.sem_op = 1;

    k = atoi(argv[1]);
    m = atoi(argv[2]);
    f = atoi(argv[3]);

    int MQ2 = atoi(argv[4]);

    int MQ3 = atoi(argv[5]);

    int sm1_id = atoi(argv[6]); // page table

    sm1 = shmat(sm1_id, NULL, 0);

    int sm2_id = atoi(argv[7]); // free frame list
    sm2 = shmat(sm2_id, NULL, 0);

    key_t key_M = ftok("master.c", 72); // number of pages allocation
    int M_id = shmget(key_M, k * sizeof(int), IPC_CREAT | 0666);
    M = shmat(M_id, NULL, 0);

    key_t key_start_alloc = ftok("master.c", 67); // start alloc list
    int start_id = shmget(key_start_alloc, k * sizeof(int), IPC_CREAT | 0666);
    start_alloc = shmat(start_id, NULL, 0);

    key_t key_end_alloc = ftok("master.c", 68); // end alloc list
    int end_id = shmget(key_end_alloc, k * sizeof(int), IPC_CREAT | 0666);
    end_alloc = shmat(end_id, NULL, 0);

    key_t key_shared_frame_no = ftok("master.c", 80); // shared frame no
    int shared_frame_no_id = shmget(key_shared_frame_no, sizeof(int), IPC_CREAT | 0666);
    shared_frame_no = shmat(shared_frame_no_id, NULL, 0);

    key_t key_stat = ftok("master.c", 64); // page table
    int stat_id = shmget(key_stat, k * sizeof(stat), IPC_CREAT | 0666);
    pro_stat = shmat(stat_id, NULL, 0);

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

    int sm_key = ftok("master.c", 1);
    sem_done = semget(sm_key, 1, 0777 | IPC_CREAT);

    fd = open("result.txt", O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fd == -1)
    {
        perror("open");
        exit(1);
    }

    printf("MMU: I am ready.\n");

    while (1)
    {
        int process_id = -1, page_id = -1;

        mq3_buffer MQ3_msg;

        msgrcv(MQ3, (void *)&MQ3_msg, sizeof(MQ3_msg), 0, 0);
        process_id = MQ3_msg.process_id;
        page_id = MQ3_msg.page_id;

        char text[50];
        sprintf(text, "GLOBAL ORDERING (%d, %d, %d)\n", timestamp, process_id, page_id);
        ssize_t bytes_written = write(fd, text, strlen(text));
        if (bytes_written == -1)
        {
            perror("write");
            exit(1);
        }

        if (process_id == -99)
        {
            break;
        }
        else if (page_id == -9)
        {
            for (int i = start_alloc[process_id]; i <= end_alloc[process_id]; i++)
            {
                sm2[i] = -1;
            }
            down(lock);
            msg_buffer msg;
            msg.msg_type = 1;
            msg.msg_data = 2;
            if (msgsnd(MQ2, (void *)&msg, sizeof(msg), 0) == -1)
            {
                perror("mmu.c line 132");
            }
            up(lock);
        }
        else if (page_id > M[process_id])
        {
            pro_stat[process_id].tot_invalid_access += 1;
            printf(ANSI_COLOR_RED "TRYING TO ACCESS INVALID PAGE REFERENCE\n" ANSI_COLOR_RESET);
            printf(ANSI_COLOR_RED "Process %d trying to access invalid page %d\n" ANSI_COLOR_RESET, process_id, page_id);

            char text[50];
            sprintf(text, "\t\t\t\tILLEGAL PAGE ACCESS (%d, %d)\n", process_id, page_id);
            ssize_t bytes_written = write(fd, text, strlen(text));
            if (bytes_written == -1)
            {
                perror("write");
                exit(1);
            }

            *shared_frame_no = -2;
            up(sem_process[process_id]);
            down(lock);
            msg_buffer msg;
            msg.msg_type = 1;
            msg.msg_data = 2;
            if (msgsnd(MQ2, (void *)&msg, sizeof(msg), 0) == -1)
            {
                perror("mmu.c line 116");
            }
            up(lock);
        }
        else if (sm1[process_id * m + page_id].is_valid == 1)
        {
            printf(ANSI_COLOR_GREEN "Process %d requesting page %d, getting frame %d\n" ANSI_COLOR_RESET, process_id, page_id, sm1[process_id * m + page_id].fno);
            *shared_frame_no = sm1[process_id * m + page_id].fno;
            up(sem_process[process_id]);
        }
        else if (sm1[process_id * m + page_id].is_valid == 0)
        {
            pro_stat[process_id].tot_page_faults += 1;
            down(lock);
            printf("Page Fault for process %d trying to access page %d\n", process_id, page_id);

            char text[50];
            sprintf(text, "\t\tPAGE FAULT (%d, %d)\n", process_id, page_id);
            ssize_t bytes_written = write(fd, text, strlen(text));
            if (bytes_written == -1)
            {
                perror("write");
                exit(1);
            }

            *shared_frame_no = -1;
            PFH(m, start_alloc[process_id], end_alloc[process_id], process_id, page_id, timestamp);

            msg_buffer msg;
            msg.msg_type = 1;
            msg.msg_data = 1;
            if (msgsnd(MQ2, (void *)&msg, sizeof(msg), 0) == -1)
            {
                perror("msgsnd");
            }
            up(sem_process[process_id]);
            up(lock);
        }
        else
        {
            fprintf(stderr, "MMU: Error! Unknown message received in MQ3. Exiting...\n");
            exit(1);
        }
        timestamp++;
    }

    char buf[200];
    sprintf(buf, "\n----------------------------------------------------------------\n");
    printf("%s", buf);
    write(fd, buf, strlen(buf));
    sprintf(buf, "Process No.    Total Page Faults         Total Illegal Access\n");
    printf("%s", buf);
    write(fd, buf, strlen(buf));

    for (int i = 0; i < k; i++)
    {
        char buf1[200];
        sprintf(buf1, "%-16d%-27d%-60d\n", i, pro_stat[i].tot_page_faults, pro_stat[i].tot_invalid_access);
        printf("%s", buf1);
        write(fd, buf1, strlen(buf1));
    }

    sprintf(buf, "----------------------------------------------------------------\n");
    printf("%s", buf);
    write(fd, buf, strlen(buf));
    close(fd);
    shmdt(sm1);
    shmdt(sm2);
    shmdt(M);
    shmdt(start_alloc);
    shmdt(end_alloc);
    shmdt(shared_frame_no);
    sleep(2);
    up(sem_done);
    exit(0);
}
