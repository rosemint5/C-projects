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

sem_t chopsticks[NUMBER_OF_PHILOSOPHERS];  // Semafor dla pałeczek
pthread_t philosophers[NUMBER_OF_PHILOSOPHERS]; // Wątek filozofów
int state[NUMBER_OF_PHILOSOPHERS];
int eatCount[NUMBER_OF_PHILOSOPHERS];  // Zliczanie ile razy filozofowie zjedli
int chopstickStatus[NUMBER_OF_PHILOSOPHERS]; // Status pałeczek, 0 domyślny, 1 zajęta
char* names[NUMBER_OF_PHILOSOPHERS] = {"Zeus", "Athena", "Artemis", "Hades", "Poseidon"}; // Imiona filozofów
int deadlockCounter = 0; // Zliczanie zakleszczeń

int main() {
    int i;
    srand(time(NULL));

    for (i = 0; i < NUMBER_OF_PHILOSOPHERS; ++i) {
        state[i] = WAITING; // Domyślny stan to czekanie
        eatCount[i] = 0;
        chopstickStatus[i] = 0;
    }

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
    int sleepTime = rand();
    if (philosopherNumber == 0) { // Zeus ma krótszy czas myślenia
        sleepTime = (rand() % 5 + 4) * 1000; // Czas myślenia między 4 a 8 sekund * 1000
    } else {
        sleepTime = rand() % 4 + 2; // Pozostali filozofowie myślą między 2 a 5 sekund
    }
    state[philosopherNumber] = THINKING;
    printf("Philosopher %s is thinking for %d seconds.\n", names[philosopherNumber], sleepTime);
    sleep(sleepTime);
    state[philosopherNumber] = WAITING;
}

void pickUp(int philosopherNumber) {
    int left = philosopherNumber;
    int right = (philosopherNumber + 1) % NUMBER_OF_PHILOSOPHERS;

    state[philosopherNumber] = WAITING;
    printf("Philosopher %s is waiting for chopsticks %d and %d\n", names[philosopherNumber], left, right);

    if (philosopherNumber % 2 == 0) {
        // Parzyści filozofowie: najpierw prawa, później lewa
        sem_wait(&chopsticks[right]);
        chopstickStatus[right] = 1; // Zaznaczenie pałeczki jako zajęta
        printf("Philosopher %s picked up chopstick %d\n", names[philosopherNumber], right);
        sleep(1);

        if (sem_wait(&chopsticks[left]) != 0) {
            chopstickStatus[right] = 0; // Odłożenie pałeczki
            printf("Philosopher %s could not get chopstick %d, releasing chopstick %d\n", names[philosopherNumber], left, right);
            sem_post(&chopsticks[right]);
            sleep(1);
            return;
        }
        chopstickStatus[left] = 1; // Zaznaczenie pałeczki jako zajęta
    } else {
        // Nieparzyści filozofowie: najpierw lewa, później prawa
        sem_wait(&chopsticks[left]);
        chopstickStatus[left] = 1; // Zaznaczenie pałeczki jako zajęta
        printf("Philosopher %s picked up chopstick %d\n", names[philosopherNumber], left);
        sleep(1);

        if (sem_wait(&chopsticks[right]) != 0) {
            chopstickStatus[left] = 0; // Odłożenie pałeczki
            printf("Philosopher %s could not get chopstick %d, releasing chopstick %d\n", names[philosopherNumber], right, left);
            sem_post(&chopsticks[left]);
            sleep(1);
            return;
        }
        chopstickStatus[right] = 1; // Zaznaczenie pałeczki jako zajęta
    }

    state[philosopherNumber] = EATING;
    printf("Philosopher %s picked up chopsticks %d and %d\n", names[philosopherNumber], left, right);
    eat(philosopherNumber);
}

void eat(int philosopherNumber) {
    int eatTime;
    if (philosopherNumber == 0) { // Załóżmy, że pierwszy filozof (Zeus) je krócej
        eatTime = 1; // Czas jedzenia to sekunda
    } else {
        eatTime = rand() % 4 + 1; // Czas jedzenia między 1 a 4 sekund
    }
    
    printf("Philosopher %s is eating for %d seconds.\n", names[philosopherNumber], eatTime);
    sleep(eatTime); // Konwersja sekund na mikrosekundy
    eatCount[philosopherNumber]++; // Zwiększenie liczby posiłków
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
        printf("Philosopher %s is currently %s and has eaten %d times.\n", names[i], stateStr, eatCount[i]);
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