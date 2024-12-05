#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <time.h>

#define PROC_COUNT 9
#define SIGNAL_COUNT 101

pid_t arrpid[PROC_COUNT];
volatile sig_atomic_t sigusr1_count = 0;
volatile sig_atomic_t sigusr2_count = 0;

void signal_handler(int signum) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);

    if (signum == SIGUSR1) {
        sigusr1_count++;
        printf("PID=%d, PPID=%d received SIGUSR1 #%d at %ld.%06ld msec\n", getpid(), getppid(), sigusr1_count, ts.tv_sec, ts.tv_nsec / 1000);
    } else if (signum == SIGUSR2) {
        sigusr2_count++;
        printf("PID=%d, PPID=%d received SIGUSR2 #%d at %ld.%06ld msec\n", getpid(), getppid(), sigusr2_count, ts.tv_sec, ts.tv_nsec / 1000);
    }
}

void terminate_handler(int signum) {
    printf("PID=%d, PPID=%d finished work after %d SIGUSR1 and %d SIGUSR2 signals\n", getpid(), getppid(), sigusr1_count, sigusr2_count);
    exit(0);
}

void create_processes() {
    for (int i = 2; i <= 5; i++) {
        pid_t pid = fork();
        if (pid == 0) {
            printf("Process %d created: PID=%d, PPID=%d\n", i, getpid(), getppid());
            arrpid[i] = getpid();
            if (i == 5) {
                for (int j = 6; j <= 8; j++) {
                    pid = fork();
                    if (pid == 0) {
                        printf("Process %d created: PID=%d, PPID=%d\n", j, getpid(), getppid());
                        arrpid[j] = getpid();
                        return;
                    } else {
                        arrpid[j] = pid;
                    }
                }
            }
            return;
        } else {
            arrpid[i] = pid;
        }
    }
}

void send_signals(int sender, int *targets, int count) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);

    for (int i = 0; i < count; i++) {
        kill(arrpid[targets[i]], SIGUSR1);
        printf("PID=%d sent SIGUSR1 to PID=%d at %ld.%06ld msec\n", sender, arrpid[targets[i]], ts.tv_sec, ts.tv_nsec / 1000);
    }
}

int main() {
    signal(SIGUSR1, signal_handler);
    signal(SIGUSR2, signal_handler);
    signal(SIGTERM, terminate_handler);

    arrpid[1] = getpid();
    printf("Process 1 created: PID=%d, PPID=%d\n", getpid(), getppid());

    create_processes();
    sleep(1); // Wait for all processes to initialize

    if (getpid() == arrpid[1]) {
        for (int i = 0; i < SIGNAL_COUNT; i++) {
            int targets[] = {2, 3, 4, 5};
            send_signals(1, targets, 4);
            pause();
        }
        for (int i = 2; i <= 5; i++) {
            kill(arrpid[i], SIGTERM);
        }
    } else if (getpid() == arrpid[5]) {
        for (int i = 0; i < SIGNAL_COUNT; i++) {
            int targets[] = {6, 7, 8};
            send_signals(5, targets, 3);
        }
        for (int i = 6; i <= 8; i++) {
            kill(arrpid[i], SIGTERM);
        }
    } else if (getpid() == arrpid[8]) {
        for (int i = 0; i < SIGNAL_COUNT; i++) {
            kill(arrpid[1], SIGUSR1);
        }
    }

    while (1) {
        pause();
    }

    return 0;
}
