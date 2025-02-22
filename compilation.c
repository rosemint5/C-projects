#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>

char **process_commands(int *n) {
    char *current_command = NULL;
    size_t command_size = 0;
    int c;
    size_t char_index = 0;
    int command_counter = 0;
    char **commands = NULL;

    while ((c = getchar()) != EOF && c != '\n') {
        if (c == ',') {
            if (char_index > 0) {
                current_command[char_index] = '\0';
                commands = (char **)realloc(commands, sizeof(char *) * (command_counter + 1));
                if (!commands) {
                    perror("Błąd alokacji pamięci.\n");
                    exit(1);
                }
                commands[command_counter++] = current_command;
                char_index = 0;
                current_command = NULL;
                command_size = 0;
            }
        } else {
            if (char_index >= command_size) {
                command_size = (command_size == 0) ? 10 : command_size * 2;
                current_command = (char *)realloc(current_command, command_size);
                if (!current_command) {
                    perror("Błąd alokacji pamięci.\n");
                    exit(1);
                }
            }
            current_command[char_index++] = c;
        }
    }

    if (char_index > 0) {
        current_command[char_index] = '\0';
        commands = (char **)realloc(commands, sizeof(char *) * (command_counter + 1));
        if (!commands) {
            perror("Błąd alokacji pamięci.\n");
            exit(1);
        }
        commands[command_counter] = current_command;
        command_counter++;
    }

    *n = command_counter;
    return commands;
}

int main() {
    int command_count = 0;
    printf("\nPodaj pliki z kodem źródłowym i flagami kompilacji (oddzielone przecinkami): ");
    char **commands = process_commands(&command_count);

    if (command_count < 1) {
        fprintf(stderr, "Brak komend do wykonania!\n");
        exit(3);
    }

    printf("\nLiczba komend: %d\n", command_count);
    for (int i = 0; i < command_count; i++) {
        printf("Komenda %d: %s\n", i + 1, commands[i]);
    }

    char **failed_compilations = NULL;
    int failed_count = 0;

    for (int i = 0; i < command_count; i++) {
        pid_t pid = fork();
        if (pid < 0) {
            perror("Błąd tworzenia procesów.\n");
            exit(4);
        }
        if (pid == 0) {
            // Przygotowanie argumentów do execvp
            char *args[50];
            char *command_copy = strdup(commands[i]);
            char *token = strtok(command_copy, " ");
            int arg_index = 0;

            // Add gcc as the first argument
            args[arg_index++] = "gcc";

            while (token != NULL) {
                args[arg_index++] = token;
                token = strtok(NULL, " ");
            }
            args[arg_index] = NULL;

            // Debugging arguments
            printf("Wykonywanie: ");
            for (int j = 0; args[j] != NULL; j++) {
                printf(" %s", args[j]);
            }
            printf("\n");

            // Check if the file exists
            if (access(args[1], F_OK) == -1) { // args[1] should be the source file
                printf("Plik '%s' nie istnieje lub nie jest dostępny.\n", args[1]);
                free(command_copy);
                exit(7);
            }

            if (access(args[1], R_OK) == -1) {
                fprintf(stderr, "Brak praw do odczytu pliku '%s'.\n", args[1]);
                free(command_copy);
                exit(8);
            }

            // Default call to gcc
            execvp("gcc", args);
            perror("Błąd execvp.\n");
            free(command_copy);
            exit(5);
        } else {
        int status;
        wait(&status);
        if (WIFEXITED(status)) {
            int exit_status = WEXITSTATUS(status);
            if (exit_status != 0) {
                // Dodajemy do failed_compilations tylko w przypadku błędu
                failed_compilations = (char **)realloc(failed_compilations, sizeof(char *) * (failed_count + 1));
                if (!failed_compilations) {
                    perror("Błąd alokacji pamięci.\n");
                    exit(6);
                }
                failed_compilations[failed_count++] = strdup(commands[i]);
            }
        } else {
            // Proces nie zakończył się normalnie
            fprintf(stderr, "Proces kompilacji nie zakończył się poprawnie.\n");
        }
    }
    }

    printf("\nPliki, których kompilacja zakończyła się błędem:\n");
    for (int i = 0; i < failed_count; i++) {
        printf("%s\n", failed_compilations[i]);
        free(failed_compilations[i]);
    }

    // Czyszczenie pamięci
    free(failed_compilations);
    for (int i = 0; i < command_count; i++) {
        free(commands[i]);
    }
    free(commands);

    return 0;
}
