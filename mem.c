#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <time.h>
#include <fcntl.h>
#include "auth.h"
#include <time.h>
#include <asm-generic/socket.h>
#define MAX_BUFFSIZE 1024


// Function prototypes for the cases
// void viewBooksInLibrary(int client_socket);
// void viewCurrentIssues(int client_socket, struct Account *acc);
// void exitMemberPanel(int client_socket);

// void member(int client_socket, struct Account *acc) {
//     // Member operations
//     char buffer[MAX_BUFFSIZE] = {0};
//     char x[100];
//     sprintf(x, "Welcome member \"%s\"", acc->username);
//     strcpy(buffer, formatHeading(x));
//     strcat(buffer, "\n\n1. View books in library.\n2. View current issues (allocations).\n3. Exit\n");
//     send(client_socket, buffer, MAX_BUFFSIZE, 0);

//     while (1) {
//         memset(buffer, '\0', sizeof(buffer));
//         send(client_socket, "\nEnter choice: ", MAX_BUFFSIZE, 0);

//         int choice;
//         recv(client_socket, &choice, sizeof(choice), 0);

//         switch (choice) {
//             case 1:
//                 viewBooksInLibrary(client_socket);
//                 break;

//             case 2:
//                 viewCurrentIssues(client_socket, acc);
//                 break;

//             case 3:
//                 exitMemberPanel(client_socket);
//                 return;

//             default:
//                 send(client_socket, "Invalid choice.\n", strlen("Invalid choice.\n"), 0);
//                 break;
//         }
//     }
// }

void viewBooksInLibrary(int client_socket) {
    char buffer[MAX_BUFFSIZE] = {0};
    FILE *bookFile;
    int fd;
    struct Book book;

    bookFile = fopen(booksCol, "rb+");
    if (bookFile == NULL) {
        send(client_socket, "Error opening book file.\n", strlen("Error opening book file.\n"), 0);
        return;
    }

    fd = fileno(bookFile);
    lock_file(fd, F_RDLCK); // Acquire read lock
    fseek(bookFile, 0, SEEK_SET);

    strcpy(buffer, "Books in library:\n");
    while (fread(&book, sizeof(struct Book), 1, bookFile)) {
        if (book.delete == 0) {
            char temp[100];
            sprintf(temp, "Class ID: %s, Name: %s, Copies: %d\n", book.class_id, book.name, book.copies);
            strcat(buffer, temp);
        }
    }
    lock_file(fd, F_UNLCK); // Release lock
    fclose(bookFile);

    send(client_socket, buffer, MAX_BUFFSIZE, 0);
}

void viewCurrentIssues(int client_socket, struct Account *acc) {
    char buffer[MAX_BUFFSIZE] = {0};
    FILE *allocFile;
    int fd;
    struct Allocation allocation;

    allocFile = fopen(allocList, "rb+");
    if (allocFile == NULL) {
        send(client_socket, "Error opening allocation file.\n", strlen("Error opening allocation file.\n"), 0);
        return;
    }

    fd = fileno(allocFile);
    lock_file(fd, F_RDLCK); // Acquire read lock
    fseek(allocFile, 0, SEEK_SET);

    strcpy(buffer, "Your current allocations:\n");
    while (fread(&allocation, sizeof(struct Allocation), 1, allocFile)) {
        if (allocation.delete == 0 && strcmp(allocation.name, acc->username) == 0) {
            char temp[200];
            sprintf(temp, "Class ID: %s, Date of Issue: %s, Date of Return: %s\n",
                    allocation.class_id, allocation.dateOfIssue, allocation.dateOfReturn);
            strcat(buffer, temp);
        }
    }
    lock_file(fd, F_UNLCK); // Release lock
    fclose(allocFile);

    send(client_socket, buffer, MAX_BUFFSIZE, 0);
}

void exitMemberPanel(int client_socket) {
    send(client_socket, "Exiting member panel.\n", strlen("Exiting member panel.\n"), 0);
}
