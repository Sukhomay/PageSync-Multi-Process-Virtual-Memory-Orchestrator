#include "header.h"

void create_sem_done(int *id)
{
    int sm_key = ftok("master.c", 1);
    *id = semget(sm_key, 1, 0777 | IPC_CREAT);
    semctl(*id, 0, SETVAL, 0);
}

int make_illegal(float p)
{
    float random_number = (float)(rand() % 1000) / 1000;

    if (random_number < p)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

int main()
{
    srand(time(NULL));

    // populating sembuf appropiately for P(s) and V(s)
    pop.sem_num = vop.sem_num = 0;
    pop.sem_flg = vop.sem_flg = 0;
    pop.sem_op = -1;
    vop.sem_op = 1;

    int k, m, f;
    printf("Enter the number of processes (k): ");
    scanf("%d", &k);
    printf("Enter the maximum virtual address space size (m): ");
    scanf("%d", &m);
    printf("Enter the total number of frames (f): ");
    scanf("%d", &f);
    if (k > f)
    {
        printf("Insufficient number of frames\n");
        exit(1);
    }

    // Create and initialize data structures
    key_t key_stat = ftok("master.c", 64); // statistics of each process
    int stat_id = shmget(key_stat, k * sizeof(stat), IPC_CREAT | 0666);
    stat *pro_stat = shmat(stat_id, NULL, 0);

    for (int i = 0; i < k; i++)
    {
        pro_stat[i].tot_invalid_access = 0;
        pro_stat[i].tot_page_faults = 0;
    }

    key_t key_sm1 = ftok("master.c", 65); // page table
    int sm1_id = shmget(key_sm1, k * m * sizeof(page), IPC_CREAT | 0666);
    page *sm1 = shmat(sm1_id, NULL, 0);

    for (int i = 0; i < k; i++)
    {
        for (int j = 0; j < m; j++)
        {
            sm1[i * m + j].is_valid = 0;
            sm1[i * m + j].fno = -1;
        }
    }

    key_t key_sm2 = ftok("master.c", 66); // free frame list
    int sm2_id = shmget(key_sm2, f * sizeof(int), IPC_CREAT | 0666);
    int *sm2 = shmat(sm2_id, NULL, 0);

    for (int i = 0; i < f; i++)
    {
        sm2[i] = -1;
    }

    key_t key_start_alloc = ftok("master.c", 67); // start alloc list
    int start_id = shmget(key_start_alloc, k * sizeof(int), IPC_CREAT | 0666);
    int *start_alloc = shmat(start_id, NULL, 0);

    key_t key_end_alloc = ftok("master.c", 68); // end alloc list
    int end_id = shmget(key_end_alloc, k * sizeof(int), IPC_CREAT | 0666);
    int *end_alloc = shmat(end_id, NULL, 0);

    key_t key_shared_frame_no = ftok("master.c", 80); // shared frame no
    int shared_frame_no_id = shmget(key_shared_frame_no, sizeof(int), IPC_CREAT | 0666);
    int *shared_frame_no = shmat(shared_frame_no_id, NULL, 0);

    key_t key_MQ1 = ftok("master.c", 61);
    int MQ1 = msgget(key_MQ1, IPC_CREAT | 0666);

    key_t key_MQ2 = ftok("master.c", 62);
    int MQ2 = msgget(key_MQ2, IPC_CREAT | 0666);

    key_t key_MQ3 = ftok("master.c", 63);
    int MQ3 = msgget(key_MQ3, IPC_CREAT | 0666);

    int sem_process[k];
    int key_ref = 100;
    for (int i = 0; i < k; i++)
    {
        key_t key = ftok(".", key_ref + i);
        sem_process[i] = semget(key, 1, 0777 | IPC_CREAT);
        semctl(sem_process[i], 0, SETVAL, 0);
    }

    int lock;
    key_t key_lock = ftok("master.c", 2);
    lock = semget(key_lock, 1, 0777 | IPC_CREAT);
    semctl(lock, 0, SETVAL, 1);

    int sem_done;
    create_sem_done(&sem_done);

    // Generate M(number of allocated frames for each page) and P(length of reference string of each page)
    key_t key_M = ftok("master.c", 72);
    int M_id = shmget(key_M, k * sizeof(int), IPC_CREAT | 0666);
    int *M = shmat(M_id, NULL, 0);

    key_t key_P = ftok("master.c", 73);
    int P_id = shmget(key_P, k * sizeof(int), IPC_CREAT | 0666);
    int *P = shmat(P_id, NULL, 0);

    // Create Scheduler
    pid_t pid_sched = fork();
    if (pid_sched == 0)
    {
        char MQ1_str[15], MQ2_str[15], k_str[15];
        sprintf(MQ1_str, "%d", MQ1);
        sprintf(MQ2_str, "%d", MQ2);
        sprintf(k_str, "%d", k);
        printf("Master: Starting Scheduler\n");
        execlp("./sched", "./sched", k_str, MQ1_str, MQ2_str, NULL);
    }

    // Create MMU
    pid_t pid_mmu = fork();
    if (pid_mmu == 0)
    {
        char MQ2_str[15], MQ3_str[15], sm1_str[15], sm2_str[15], k_str[15], m_str[15], f_str[15];
        sprintf(k_str, "%d", k);
        sprintf(m_str, "%d", m);
        sprintf(f_str, "%d", f);
        sprintf(MQ2_str, "%d", MQ2);
        sprintf(MQ3_str, "%d", MQ3);
        sprintf(sm1_str, "%d", sm1_id);
        sprintf(sm2_str, "%d", sm2_id);
        printf("Master: Starting MMU\n");
        execlp("xterm", "xterm", "-T", "MMU", "-e", "./mmu", k_str, m_str, f_str, MQ2_str, MQ3_str, sm1_str, sm2_str, NULL);
        // execlp("./mmu","./mmu", k_str, m_str, f_str, MQ2_str, MQ3_str, sm1_str, sm2_str, NULL);
    }

    int sum_M = 0;
    for (int i = 0; i < k; i++)
    {
        M[i] = (rand() % m) + 1;
        sum_M += M[i];
        P[i] = (rand() % (9 * M[i])) + (2 * M[i]);
    }

    int en = 0;
    for (int i = 0; i < k; i++)
    {
        start_alloc[i] = en;
        end_alloc[i] = en + (f * M[i]) / sum_M - 1;
        if (end_alloc[i] < start_alloc[i])
            end_alloc[i] = start_alloc[i];
        en = end_alloc[i] + 1;
    }

    // return 0;

    // Generate the reference string for each process and finally crete each process
    int ref_start_key = 1000;
    for (int i = 0; i < k; i++)
    {
        key_t key_r_i = ftok(".", ref_start_key + i); // end alloc list
        int r_i_id = shmget(key_r_i, P[i] * sizeof(int), IPC_CREAT | 0666);
        int *ref_string = shmat(r_i_id, NULL, 0);

        for (int j = 0; j < P[i]; j++)
        {
            int is_illegal = make_illegal((float)prob_illegal_access);
            if (is_illegal)
            {
                ref_string[j] = (rand() % (5 * M[i])) + (M[i]);
            }
            else
            {
                ref_string[j] = (rand() % M[i]);
            }
        }

        printf("Process %d: M[i] = %d, P[i] = %d\n", i, M[i], P[i]);
        for (int j = 0; j < P[i]; j++)
        {
            printf("%d ", ref_string[j]);
        }
        printf("\n");

        pid_t pid_proc = fork();
        if (pid_proc == 0)
        {
            char i_str[15], r_i_str[15], MQ1_str[15], MQ3_str[15], pi_str[15];
            sprintf(MQ1_str, "%d", MQ1);
            sprintf(MQ3_str, "%d", MQ3);
            sprintf(r_i_str, "%d", r_i_id);
            sprintf(i_str, "%d", i);
            sprintf(pi_str, "%d", P[i]);
            printf("Master: Starting process %d\n", i);
            execlp("./process", "./process", i_str, r_i_str, MQ1_str, MQ3_str, pi_str, NULL);
        }

        float sleep_time = 0.25;
        usleep(sleep_time * 1000000);
    }

    down(sem_done);

    down(lock);
    mq3_buffer MQ3_msg;
    MQ3_msg.msg_type = 1;
    MQ3_msg.process_id = -99;
    MQ3_msg.page_id = -99;
    if (msgsnd(MQ3, (void *)&MQ3_msg, sizeof(MQ3_msg), 0) == -1)
    {
        perror("msgsnd");
    }
    up(lock);

    down(sem_done);

    // Clean up shared memory and message queues
    shmdt(sm1);
    shmdt(sm2);
    shmdt(start_alloc);
    shmdt(end_alloc);
    shmdt(M);
    shmdt(P);
    shmdt(pro_stat);
    shmctl(stat_id, IPC_RMID, NULL);
    shmctl(sm1_id, IPC_RMID, NULL);
    shmctl(sm2_id, IPC_RMID, NULL);
    shmctl(start_id, IPC_RMID, NULL);
    shmctl(end_id, IPC_RMID, NULL);
    shmctl(M_id, IPC_RMID, NULL);
    shmctl(P_id, IPC_RMID, NULL);
    msgctl(MQ1, IPC_RMID, NULL);
    msgctl(MQ2, IPC_RMID, NULL);
    msgctl(MQ3, IPC_RMID, NULL);

    printf("Master: Done with all the processes! Cleaned everything. Exiting...\n");
}
