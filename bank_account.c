#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#define NUM_DEPOSIT_THREADS 5
#define NUM_WITHDRAW_THREADS 5
#define NUM_TRANSFER_THREADS 3
#define NUM_OPERATIONS 10
#define TRANSACTION_AMOUNT 100

// Stany kont
int account1_balance = 0;
int account2_balance = 0;
int total_balance = 0; // Całkowita suma pieniędzy

// Muteks
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

// Sprawdzanie spójności sumy pieniędzy
void check_total_balance() {
    int current_total = account1_balance + account2_balance;
    if (current_total != total_balance) {
        printf("ERROR: ! Nieprawidłowe saldo całkowite ! Oczekiwane: %d, Aktualne: %d\n", total_balance, current_total);
        exit(EXIT_FAILURE); // Wyjście z programu w przypadku błędu
    }
}

// Depozyt pieniędzy
void* deposit(void* arg) {
    int* data = (int*)arg;
    int account_id = data[0];
    int thread_id = data[1];

    for (int i = 0; i < NUM_OPERATIONS; i++) {
        pthread_mutex_lock(&mutex);
        int* account_balance = (account_id == 1) ? &account1_balance : &account2_balance;

        *account_balance += TRANSACTION_AMOUNT;
        total_balance += TRANSACTION_AMOUNT;

        printf("Wątek %d: Wpłacono %d na konto %d, Saldo: %d, Saldo całkowite: %d\n",
               thread_id, TRANSACTION_AMOUNT, account_id, *account_balance, total_balance);

        check_total_balance();
        pthread_mutex_unlock(&mutex);
        usleep(rand() % 100000);
    }

    return NULL;
}

// Wypłacanie pieniędzy
void* withdraw(void* arg) {
    int* data = (int*)arg;
    int account_id = data[0];
    int thread_id = data[1];

    for (int i = 0; i < NUM_OPERATIONS; i++) {
        pthread_mutex_lock(&mutex);
        int* account_balance = (account_id == 1) ? &account1_balance : &account2_balance;

        if (*account_balance >= TRANSACTION_AMOUNT) {
            *account_balance -= TRANSACTION_AMOUNT;
            total_balance -= TRANSACTION_AMOUNT;

            printf("Wątek %d: Wypłacono %d z konta %d, Saldo: %d, Saldo całkowite: %d\n",
                   thread_id, TRANSACTION_AMOUNT, account_id, *account_balance, total_balance);
            check_total_balance();
        } else {
            printf("Wątek %d: Niewystarczająca ilość pieniędzy na koncie %d, Saldo: %d\n",
                   thread_id, account_id, *account_balance);
        }

        pthread_mutex_unlock(&mutex);
        usleep(rand() % 100000);
    }

    return NULL;
}

// Przelew pieniędzy
void* transfer(void* arg) {
    int thread_id = *(int*)arg;

    for (int i = 0; i < NUM_OPERATIONS; i++) {
        pthread_mutex_lock(&mutex);

        if (account1_balance >= TRANSACTION_AMOUNT) {
            account1_balance -= TRANSACTION_AMOUNT;
            account2_balance += TRANSACTION_AMOUNT;

            printf("Wątek %d: Przelano %d z konta 1 na konto 2, Salda: [1: %d, 2: %d], Saldo całkowite: %d\n",
                   thread_id, TRANSACTION_AMOUNT, account1_balance, account2_balance, total_balance);
        } else {
            printf("Wątek %d: Przelew nieudany ze względu na niewystarczające fundusze na koncie 1\n", thread_id);
        }

        check_total_balance();
        pthread_mutex_unlock(&mutex);
        usleep(rand() % 100000);
    }

    return NULL;
}

int main() {
    pthread_t deposit_threads[NUM_DEPOSIT_THREADS];
    pthread_t withdraw_threads[NUM_WITHDRAW_THREADS];
    pthread_t transfer_threads[NUM_TRANSFER_THREADS];

    int thread_data[NUM_DEPOSIT_THREADS + NUM_WITHDRAW_THREADS][2];
    int transfer_thread_ids[NUM_TRANSFER_THREADS];

    // Inicjalizacja całkowitej sumy pieniędzy
    total_balance = account1_balance + account2_balance;

    // Wątki depozytów
    for (int i = 0; i < NUM_DEPOSIT_THREADS; i++) {
        thread_data[i][0] = (i % 2) + 1; // Zmiany pomiędzy dwoma kontami
        thread_data[i][1] = i + 1;
        if (pthread_create(&deposit_threads[i], NULL, deposit, &thread_data[i]) != 0) {
            perror("Błąd w tworzeniu wątku depozytu");
            exit(EXIT_FAILURE);
        }
    }

    // Wątki poborów
    for (int i = 0; i < NUM_WITHDRAW_THREADS; i++) {
        thread_data[NUM_DEPOSIT_THREADS + i][0] = (i % 2) + 1; // Zmiany pomiędzy dwoma kontami
        thread_data[NUM_DEPOSIT_THREADS + i][1] = NUM_DEPOSIT_THREADS + i + 1;
        if (pthread_create(&withdraw_threads[i], NULL, withdraw, &thread_data[NUM_DEPOSIT_THREADS + i]) != 0) {
            perror("Błąd w tworzeniu wątku poboru");
            exit(EXIT_FAILURE);
        }
    }

    // Wątki przelewów
    for (int i = 0; i < NUM_TRANSFER_THREADS; i++) {
        transfer_thread_ids[i] = i + 1;
        if (pthread_create(&transfer_threads[i], NULL, transfer, &transfer_thread_ids[i]) != 0) {
            perror("Błąd w tworzeniu wątku przelewu");
            exit(EXIT_FAILURE);
        }
    }

    // Łączenie wątków
    for (int i = 0; i < NUM_DEPOSIT_THREADS; i++) {
        pthread_join(deposit_threads[i], NULL);
    }
    for (int i = 0; i < NUM_WITHDRAW_THREADS; i++) {
        pthread_join(withdraw_threads[i], NULL);
    }
    for (int i = 0; i < NUM_TRANSFER_THREADS; i++) {
        pthread_join(transfer_threads[i], NULL);
    }

    printf("Salda końcowe: konto 1: %d, konto 2: %d, Saldo całkowite: %d\n", account1_balance, account2_balance, total_balance);

    pthread_mutex_destroy(&mutex);

    return 0;
}
