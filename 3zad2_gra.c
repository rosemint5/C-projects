#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>

#define FIFO_PATH "game_fifo"
#define BOARD_SIZE 5
#define PLAYER_SYMBOL 'X'

void display_board(int player_x, int player_y) {
    for (int i = 0; i < BOARD_SIZE; i++) {
        for (int j = 0; j < BOARD_SIZE; j++) {
            if (i == player_x && j == player_y)
                printf("%c ", PLAYER_SYMBOL);
            else
                printf(". ");
        }
        printf("\n");
    }
}

int main() {
    int player_x = 0, player_y = 0;

    // Otwarce FIFO do odczytu
    int fifo_fd = open(FIFO_PATH, O_RDONLY);
    if (fifo_fd == -1) {
        perror("open FIFO");
        exit(EXIT_FAILURE);
    }

    // Sterowanie ludzikiem
    char command[10];
    while (1) {
        if (read(fifo_fd, command, sizeof(command)) > 0) {
            int valid_command = 1;

            if (strcmp(command, "w") == 0) {
                if (player_x > 0) {
                    player_x--;
                }
            } else if (strcmp(command, "s") == 0) {
                if (player_x < BOARD_SIZE - 1) {
                    player_x++;
                }
            } else if (strcmp(command, "a") == 0) {
                if (player_y > 0) {
                    player_y--;
                }
            } else if (strcmp(command, "d") == 0) {
                if (player_y < BOARD_SIZE - 1) {
                    player_y++;
                }
            } else if (strcmp(command, "exit") == 0) {
                printf("Exiting game...\n");
                break;
            } else {
                valid_command = 0; // Nieodpowiednia komenda
            }

            if (valid_command) {
                // Usunięcie planszy
                system("clear");
                display_board(player_x, player_y);
            } else {
                printf("Invalid command.\n");
            }
        } else {
            printf("Parent closed FIFO. Exiting game...\n");
            break;
        }
    }


    close(fifo_fd);
    return 0;
}