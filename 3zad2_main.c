#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>

#define FIFO_PATH "game_fifo"

int main() {
    // Tworzenie FIFO
    if (mkfifo(FIFO_PATH, 0777) == -1) {
        if (errno != EEXIST) {
            perror("mkfifo");
            exit(EXIT_FAILURE);
        }
    }

    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (pid == 0) {
        // Dziecko zaczyna grę
        execlp("./3zad2_gra", "3zad2_gra", NULL);
        perror("execlp");
        exit(EXIT_FAILURE);
    }

    // Rodzic wysyła komendy
    int fifo_fd = open(FIFO_PATH, O_WRONLY);
    if (fifo_fd == -1) {
        perror("open FIFO");
        exit(EXIT_FAILURE);
    }

    // Rodzic gra w grę
    char command[10];
    while (1) {
        printf("Enter command (w, a, s, d, exit): ");
        scanf("%s", command);

        if (strcmp(command, "exit") == 0) {
            write(fifo_fd, command, strlen(command) + 1);
            break;
        }

        if (write(fifo_fd, command, strlen(command) + 1) == -1) {
            perror("write FIFO");
            break;
        }
    }

    close(fifo_fd);
    wait(NULL); // Czekanie na dziecko
    unlink(FIFO_PATH); // Usuwanie FIFO
    return 0;
}
