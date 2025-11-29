#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>
#include <sys/wait.h>
#include <time.h>

int main(int argc, char *argv[])
{
    int ShmID;
    int *ShmPTR;
    pid_t pid;
    int status;

    // Create shared memory for 2 integers: BankAccount and Turn
    ShmID = shmget(IPC_PRIVATE, 2 * sizeof(int), IPC_CREAT | 0666);
    if (ShmID < 0) {
        printf("*** shmget error ***\n");
        exit(1);
    }

    ShmPTR = (int *)shmat(ShmID, NULL, 0);
    if (ShmPTR == (int *)-1) {
        printf("*** shmat error ***\n");
        exit(1);
    }

    // Shared variables:
    // ShmPTR[0] = BankAccount
    // ShmPTR[1] = Turn  (0 = parent, 1 = child)
    ShmPTR[0] = 0; // BankAccount
    ShmPTR[1] = 0; // Turn

    pid = fork();
    if (pid < 0) {
        printf("*** fork error ***\n");
        exit(1);
    }

    // ---------------- Parent process: Dear Old Dad ----------------
    if (pid > 0) {
        int i;
        srand(time(NULL) ^ getpid());

        for (i = 0; i < 25; i++) {
            // Sleep some random amount of time between 0 - 5 seconds
            sleep(rand() % 6);

            // Strict alternation: wait for Turn == 0
            while (ShmPTR[1] != 0) {
                // busy wait / no-op (small sleep to reduce CPU burn)
                usleep(1000);
            }

            int account = ShmPTR[0];

            if (account <= 100) {
                // Randomly generate a balance amount to give the Student between 0 and 100
                int balance = rand() % 101;   // 0–100 inclusive

                if (balance % 2 == 0) {
                    account += balance;
                    printf("Dear old Dad: Deposits $%d / Balance = $%d\n",
                           balance, account);
                } else {
                    printf("Dear old Dad: Doesn't have any money to give\n");
                }
            } else {
                printf("Dear old Dad: Thinks Student has enough Cash ($%d)\n",
                       account);
            }

            // Copy value back to BankAccount
            ShmPTR[0] = account;

            // Give turn to child
            ShmPTR[1] = 1;
        }

        // Wait for child to finish
        wait(&status);

        // Detach and remove shared memory
        shmdt((void *)ShmPTR);
        shmctl(ShmID, IPC_RMID, NULL);

        exit(0);
    }

    // ---------------- Child process: Poor Student -----------------
    else {
        int i;
        srand(time(NULL) ^ getpid());

        for (i = 0; i < 25; i++) {
            // Sleep some random amount of time between 0 - 5 seconds
            sleep(rand() % 6);

            // Strict alternation: wait for Turn == 1
            while (ShmPTR[1] != 1) {
                // busy wait / no-op (small sleep to reduce CPU burn)
                usleep(1000);
            }

            int account = ShmPTR[0];

            // Randomly generate a balance amount the Student needs between 0-49
            int balance = rand() % 50;
            printf("Poor Student needs $%d\n", balance);

            if (balance <= account) {
                account -= balance;
                printf("Poor Student: Withdraws $%d / Balance = $%d\n",
                       balance, account);
            } else {
                printf("Poor Student: Not Enough Cash ($%d)\n", account);
            }

            ShmPTR[0] = account;

            // Give turn back to parent
            ShmPTR[1] = 0;
        }

        exit(0);
    }
}
