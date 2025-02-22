#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <stdlib.h>
#include <semaphore.h>

#define NUMBER_OF_PHILOSOPHERS 5

// Stany filozofów
#define THINKING 0
#define EATING 1
#define WAITING 2

void *philosopher(void *);
void think(int philosopherNumber);
void pickUp(int philosopherNumber);
void eat(int philosopherNumber);
void putDown(int philosopherNumber);
void printState(int elapsedTime);
void detectDeadlock();

sem_t chopsticks[NUMBER_OF_PHILOSOPHERS]; // Semafor dla pałeczek
sem_t priorityLock; // Semfor dla priorytetów
pthread_t philosophers[NUMBER_OF_PHILOSOPHERS]; // Wątek filozofów
int state[NUMBER_OF_PHILOSOPHERS];
int eatCount[NUMBER_OF_PHILOSOPHERS];  // Zliczanie ile razy filozofowie zjedli
int eatTimeTotal[NUMBER_OF_PHILOSOPHERS]; // Zliczanie ile czasu filozofowie jedli
int waitPriority[NUMBER_OF_PHILOSOPHERS]; // Priorytet 0 jest domyślny, im wyższa liczba tym większy priorytet
int chopstickStatus[NUMBER_OF_PHILOSOPHERS]; // Status pałeczek, 0 domyślny, 1 zajęta
time_t waitingStartTime[NUMBER_OF_PHILOSOPHERS]; // Startowy czas czekania
char* names[NUMBER_OF_PHILOSOPHERS] = {"Zeus", "Athena", "Artemis", "Hades", "Poseidon"}; // Imiona filozofów
int deadlockCounter = 0; // Zliczanie zakleszczeń

int main() {
    int i;
    srand(time(NULL));

    for (i = 0; i < NUMBER_OF_PHILOSOPHERS; ++i) {
        state[i] = WAITING;  // Domyślny stan to czekanie
        eatCount[i] = 0;
        eatTimeTotal[i] = 0;
        waitPriority[i] = 0;
        chopstickStatus[i] = 0;
        waitingStartTime[i] = time(NULL);
    }

    // Semafor pozwala na dostęp jednemu wątkowi na raz
    sem_init(&priorityLock, 0, 1);

    // Semafor pozwala na dostęp jednemu wątkowi na raz
    for (i = 0; i < NUMBER_OF_PHILOSOPHERS; ++i) {
        sem_init(&chopsticks[i], 0, 1);
    }

    // Tworzenie wątków filozofów
    for (i = 0; i < NUMBER_OF_PHILOSOPHERS; ++i) {
        pthread_create(&philosophers[i], NULL, philosopher, (void *)(long)i);
    }

    // Wypisywanie stanów filozofów, co robią
    int elapsedTime = 0;
    while (1) {
        sleep(1);
        elapsedTime++;
        printState(elapsedTime);
        detectDeadlock();
    }

    // Łączenie wątków
    for (i = 0; i < NUMBER_OF_PHILOSOPHERS; ++i) {
        pthread_join(philosophers[i], NULL);
    }
    return 0;
}

void *philosopher(void *philosopherNumber) {
    int philosopherID = (int)(long)philosopherNumber;

    // Pętla główna filozofów
    while (1) {
        if (state[philosopherID] != WAITING) {
            think(philosopherID);
        }
        pickUp(philosopherID);
    }
}

void think(int philosopherNumber) {
    int sleepTime = rand() % 4 + 2; // Czas myślenia między 2 a 5 sekund
    state[philosopherNumber] = THINKING;
    printf("Philosopher %s is thinking for %d seconds.\n", names[philosopherNumber], sleepTime);
    sleep(sleepTime);

    // Blokada, filozof myśli jeżeli nie jest głodny, bo zjadł za dużo
    sem_wait(&priorityLock);

    // Sprawdzanie, czy filozof może dostać priorytet
    int canIncreasePriority = 1;
    for (int i = 0; i < NUMBER_OF_PHILOSOPHERS; ++i) {
        // Jeżeli filozof zjadł więcej posiłków niż inni o 5 to jest najedzony i myśli dłużej (zapobieganie głodzeniu)
        if (i != philosopherNumber && eatCount[philosopherNumber] - eatCount[i] >= 5) {
            canIncreasePriority = 0;
            break;
        }
    }

    if (canIncreasePriority) {
        waitPriority[philosopherNumber]++; // Dodawanie priorytetu, gdy filozof skończy myśleć
    }

    // Filozof zgłodniał, zwalnianie blokady i pozwalanie mu czekać na pałeczki
    sem_post(&priorityLock);

    state[philosopherNumber] = WAITING;
    waitingStartTime[philosopherNumber] = time(NULL); // Reset czasu myślenia
}

void pickUp(int philosopherNumber) {
    int left = philosopherNumber;
    int right = (philosopherNumber + 1) % NUMBER_OF_PHILOSOPHERS;

    state[philosopherNumber] = WAITING;
    printf("Philosopher %s is waiting for chopsticks %d and %d\n", names[philosopherNumber], left, right);

    while (1) {
        sem_wait(&priorityLock);

        // Sprawdzanie, czy filozof czekał dłużej niż 5 sekund
        // Jeżeli tak, to zwiększamy priorytet
        time_t currentTime = time(NULL);
        if (currentTime - waitingStartTime[philosopherNumber] > 5) {
            waitPriority[philosopherNumber]++;
            waitingStartTime[philosopherNumber] = currentTime; // Reset czasu czekania
        }

        // Sprawdzanie, czy filozof ma najwyższy priorytet (czy może zjeść)
        int hasHighestPriority = 1;
        for (int i = 0; i < NUMBER_OF_PHILOSOPHERS; ++i) {
            if (i != philosopherNumber && state[i] == WAITING && waitPriority[i] > waitPriority[philosopherNumber]) {
                hasHighestPriority = 0;
                break;
            }
        }

        // Zwolnienie blokady na jedzenie
        sem_post(&priorityLock);

        if (hasHighestPriority) {
            if (philosopherNumber % 2 == 0) {
                // Parzyści filozofowie: najpierw lewa, później prawa pałeczka
                sem_wait(&chopsticks[left]);
                chopstickStatus[left] = 1; // Zaznaczenie pałeczki jako zajęta
                printf("Philosopher %s picked up chopstick %d\n", names[philosopherNumber], left);
                sleep(1);

                sem_wait(&chopsticks[right]);
                chopstickStatus[right] = 1; // Zaznaczenie pałeczki jako zajęta
                printf("Philosopher %s picked up chopstick %d\n", names[philosopherNumber], right);
            } else {
                // Nieparzyści filozofowie: najpierw prawa, później lewa pałeczka
                sem_wait(&chopsticks[right]);
                chopstickStatus[right] = 1; // Zaznaczenie pałeczki jako zajęta
                printf("Philosopher %s picked up chopstick %d\n", names[philosopherNumber], right);
                sleep(1);

                sem_wait(&chopsticks[left]);
                chopstickStatus[left] = 1; // Zaznaczenie pałeczki jako zajęta
                printf("Philosopher %s picked up chopstick %d\n", names[philosopherNumber], left);
            }

            state[philosopherNumber] = EATING;

            // Unikanie race condition
            sem_wait(&priorityLock);
            waitPriority[philosopherNumber] = 0; // Reset priorytetu po jedzeniu
            sem_post(&priorityLock);

            eat(philosopherNumber);
            return;
        } else {
            sleep(1);
        }
    }
}

void eat(int philosopherNumber) {
    int eatTime = rand() % 4 + 1; // Czas jedzenia między 1 a 4 sekundy
    printf("Philosopher %s is eating for %d seconds.\n", names[philosopherNumber], eatTime);
    sleep(eatTime);

    sem_wait(&priorityLock);
    eatCount[philosopherNumber]++; // Zwiększenie liczby posiłków
    eatTimeTotal[philosopherNumber] += eatTime; // Zwiększenie ogólnego czasu jedzenia
    sem_post(&priorityLock);

    putDown(philosopherNumber);
}

void putDown(int philosopherNumber) {
    int left = philosopherNumber;
    int right = (philosopherNumber + 1) % NUMBER_OF_PHILOSOPHERS;

    // Zwalnianie semaforów
    sem_post(&chopsticks[left]);
    sem_post(&chopsticks[right]);
    chopstickStatus[left] = 0; // Zaznaczenie pałeczki jako dostępnej
    chopstickStatus[right] = 0; // Zaznaczenie pałeczki jako dostępnej
    printf("Philosopher %s has put down chopsticks %d and %d\n", names[philosopherNumber], left, right);
}


void printState(int elapsedTime) {
    printf("\n----- Second %d -----\n", elapsedTime);
    for (int i = 0; i < NUMBER_OF_PHILOSOPHERS; ++i) {
        const char *stateStr;
        switch (state[i]) {
            case THINKING: stateStr = "Thinking"; break;
            case EATING: stateStr = "Eating"; break;
            case WAITING: stateStr = "Waiting"; break;
            default: stateStr = "Unknown"; break;
        }
        printf("Philosopher %s is currently %s and has eaten for %d seconds (%d times). Their wait priority is %d\n", names[i], stateStr, eatTimeTotal[i], eatCount[i], waitPriority[i]);
    }

    printf("\n----- Chopstick status -----\n");
    for (int i = 0; i < NUMBER_OF_PHILOSOPHERS; ++i) {
        printf("Chopstick %d is %s\n", i, chopstickStatus[i] ? "in use" : "available");
    }
}

// Wykrywanie deadlocka
void detectDeadlock() {
    int waitingPhilosophers = 0;
    int occupiedChopsticks = 0;

    for (int i = 0; i < NUMBER_OF_PHILOSOPHERS; ++i) {
        if (state[i] == WAITING) {
            waitingPhilosophers++;
        }
    }

    for (int i = 0; i < NUMBER_OF_PHILOSOPHERS; ++i) {
        if (chopstickStatus[i] == 1) {
            occupiedChopsticks++;
        }
    }

    // Jeżeli wszyscy filozofowie czekają i wszystkie pałeczki są zajęte
    if (waitingPhilosophers == NUMBER_OF_PHILOSOPHERS && occupiedChopsticks == NUMBER_OF_PHILOSOPHERS) {
        deadlockCounter++;
    } else {
        deadlockCounter = 0;
    }

    if (deadlockCounter >= 5) { // Jeżeli deadlock trwa przez 5 sekund
        printf("\nDEADLOCK DETECTED! All philosophers are waiting and all chopsticks are in use for 5 seconds.\n");
        exit(1);
    }
}
