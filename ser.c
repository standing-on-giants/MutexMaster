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

#define PORT 8080
#define MAX_CONNECTIONS 5

void lock_file(int fd, short type) { // to get lock
    struct flock lock;
    memset(&lock, 0, sizeof(lock));
    lock.l_type = type;
    lock.l_whence = SEEK_SET;
    lock.l_start = 0;
    lock.l_len = 0; // Lock the whole file
    fcntl(fd, F_SETLKW, &lock);
}
// NOTE: record locking possible but useful only when DB entries are index i.e. a sseparate .ndx file




void admin(int client_socket, struct Account *acc) {
    // Admin operations
    char buffer[MAX_BUFFSIZE] = {0};
    char x[100];
    sprintf(x, "Welcome admin \"%s\"", acc->username);
    strcpy(buffer, formatHeading(x));
    strcat(buffer, "\n\n1. See all books and available copies.\n2. See incumbent allocations.\n3. Add book.\n4. Update copies of a particular book.\n5. Delete book (all copies).\n6. Allocate a book.\n7. Deallocate a book.\n8. See allocations to a particular user.\n9. View all users/members.\n10. Exit\n");
    send(client_socket, buffer, MAX_BUFFSIZE, 0);

    while (1) {
        memset(buffer, '\0', sizeof(buffer));
        send(client_socket, "\nEnter choice:", MAX_BUFFSIZE, 0);

        int choice;
        recv(client_socket, &choice, sizeof(choice), 0);
        printf("Recvd choice %d\n", choice);

        switch (choice) {
            case 1:
                seeAllBooks(client_socket);
                break;

            case 2:
                seeAllocations(client_socket);
                break;

            case 3:
                addBook(client_socket);
                break;

            case 4:
                updateBookCopies(client_socket);
                break;

            case 5:
                deleteBook(client_socket); 

                break;

            case 6:
                allocateBook(client_socket);

                break;

            case 7:
                deallocateBook(client_socket);
                break;

            case 8:
                seeAllocationsForUser(client_socket);

                break;

            case 9:
                viewAllUsers(client_socket);
                break;

            case 10:
                send(client_socket, "Exiting admin panel.\n", MAX_BUFFSIZE, 0);
                return;

            default:
                send(client_socket, "Invalid choice.\n", MAX_BUFFSIZE, 0);
                break;
        }
    }
}



void member(int client_socket, struct Account *acc) {
    // Member operations
    char buffer[MAX_BUFFSIZE] = {0};
    char x[100];
    sprintf(x, "Welcome member \"%s\"", acc->username);
    strcpy(buffer, formatHeading(x));
    strcat(buffer, "\n\n1. View books in library.\n2. View current issues (allocations).\n3. Exit\n");
    send(client_socket, buffer, MAX_BUFFSIZE, 0);

    while (1) {
        memset(buffer, '\0', sizeof(buffer));
        send(client_socket, "\nEnter choice: ", MAX_BUFFSIZE, 0);

        int choice;
        recv(client_socket, &choice, sizeof(choice), 0);

        switch (choice) {
            case 1:
                viewBooksInLibrary(client_socket);
                break;

            case 2:
                viewCurrentIssues(client_socket, acc);
                break;

            case 3:
                exitMemberPanel(client_socket);
                return;

            default:
                send(client_socket, "Invalid choice.\n", strlen("Invalid choice.\n"), 0);
                break;
        }
    }
}



int checkCredentials(const char *username, const char *password, enum AccountType type, struct Account * account) {// authentication
    FILE *file;
    int fd;
    if (type == MEMBER)
        file = fopen(memAccs, "rb");
    else if (type == ADMIN)
        file = fopen(admAccs, "rb");
    else {
        printf("Invalid account type.\n");
        return INVALID_ENTITY;
    }

    if (file == NULL) {
        printf("Error opening file.\n");
        return INVALID_ENTITY;
    }

    fd = fileno(file);
    lock_file(fd, F_RDLCK); // Acquire a read lock

    fseek(file, 0, SEEK_SET);
    //struct Account account;

    while (fread(account, sizeof(struct Account), 1, file)) {
        if (strcmp((*account).username, username) == 0 &&
            strcmp((*account).password, password) == 0 &&
            (*account).type == type) {
            lock_file(fd, F_UNLCK); // Release the lock
            fclose(file);
            return ACCOUNT_FOUND;
        }
    }

    lock_file(fd, F_UNLCK); // Release the lock
    fclose(file);
    return INVALID_CREDENTIALS;
}

void registerAccount(int client_socket ) {

    char username[MAX_USERNAME_LENGTH];
    char password[MAX_PASSWORD_LENGTH];
    int accountType;

    send(client_socket, "Enter username: ", strlen("Enter username: "), 0);
    recv(client_socket, username, MAX_USERNAME_LENGTH, 0);
    username[strcspn(username, "\n")] = '\0';

    send(client_socket, "Enter password: ", strlen("Enter password: "), 0);
    recv(client_socket, password, MAX_PASSWORD_LENGTH, 0);
    password[strcspn(password, "\n")] = '\0';

    send(client_socket, "Enter account type (0 for member, 1 for admin): ", strlen("Enter account type (0 for member, 1 for admin): "), 0);
    char accountTypeStr[2];
    recv(client_socket, accountTypeStr, 2, 0);
    accountType = atoi(accountTypeStr);

    FILE *file;
    if (accountType == 0) {
        file = fopen(memAccs, "rb+");
    } else if (accountType == 1) {
        file = fopen(admAccs, "rb+");
    } else {
        send(client_socket, "Invalid account type.\n", strlen("Invalid account type.\n"), 0);
        return;
    }

    if (file == NULL) {
        send(client_socket, "Error opening file.\n", strlen("Error opening file.\n"), 0);
        return;
    }

    int fd = fileno(file);
    lock_file(fd, F_WRLCK); // Acquire a write lock

    struct Account newAccount;
    strcpy(newAccount.username, username);
    strcpy(newAccount.password, password);
    newAccount.type = (accountType == 1) ? ADMIN : MEMBER;
    time_t t=time(NULL);
    strcpy(newAccount.joiningTime, ctime(&t));

    fseek(file, 0, SEEK_END);
    fwrite(&newAccount, sizeof(struct Account), 1, file);

    lock_file(fd, F_UNLCK); // Release the lock
    fclose(file);

    send(client_socket, "Account registered successfully. Open a new session to login.\n", strlen("Account registered successfully. Open a new session to login.\n"), 0);
}

int login(int client_socket, struct Account * account) {
    //int client_socket = *((int *)arg);

    char usrnm[MAX_USERNAME_LENGTH];
    char passwd[MAX_PASSWORD_LENGTH];
    enum AccountType type;

    send(client_socket, "Enter username: ", MAX_USERNAME_LENGTH, 0);
    int valread = recv(client_socket, usrnm, MAX_USERNAME_LENGTH, 0);
    if (valread <= 0) {
        printf("Client disconnected.\n");
        return END_CONN;
    }
    printf("Received: %s\n", usrnm);

    send(client_socket, "Enter password: ", MAX_PASSWORD_LENGTH, 0);
    valread = recv(client_socket, passwd, MAX_PASSWORD_LENGTH, 0);
    if (valread <= 0) {
        printf("Client disconnected.\n");
        return END_CONN;
    }
    printf("Received: %s\n", passwd);

    send(client_socket, "Login as (0: Member, 1: Admin): ", 50, 0);
    char buff[2] = {0};
    valread = recv(client_socket, buff, 2, 0);
    if (valread <= 0) {
        printf("Client disconnected.\n");
        return END_CONN;
    }
    printf("Received: %s\n", buff);

    if (strcmp(buff, "0") == 0) type = MEMBER;
    else if (strcmp(buff, "1") == 0) type = ADMIN;
    else {
        printf("Invalid account type.\n");
        return END_CONN;
    }

    //struct Account account;

    int r = checkCredentials(usrnm, passwd, type, account);
    if (r == ACCOUNT_FOUND) {
        printf("Account found.\n");
        (type==1)?send(client_socket, "Account found. Logging in as Admin...\n", MAX_BUFFSIZE, 0):
                send(client_socket, "Account found. Logging in as Member...\n", MAX_BUFFSIZE, 0);
        return (type==0)? TO_MEMBER: TO_ADMIN;
    } else {
        printf("Invalid credentials.\n");
        send(client_socket, "Invalid credentials.\n", strlen("Invalid credentials.\n"), 0);
    }return END_CONN;
}

int entry(int client_socket) {
    //int client_socket = *((int *)arg);
    char *msg = "1. Register account.\n2. Login.\n\nEnter choice: ";
    char choice[2];

    send(client_socket, msg, strlen(msg), 0);
    if (recv(client_socket, choice, 2, 0) <= 0) {
        printf("Client disconnected.\n");
        return END_CONN;
    }

    if (strcmp(choice, "1") == 0) {
        return TO_REGISTER;
    } else if (strcmp(choice, "2") == 0) {
        return TO_LOGIN;
    } else {
        printf("Invalid choice.\n");
        return END_CONN;
    }
}

void handle_connection(int client_socket, struct sockaddr_in *address) {
    /*char usrnm[MAX_USERNAME_LENGTH];
    char passwd[MAX_PASSWORD_LENGTH];
    enum AccountType type;*/
    struct Account account;

    int r = entry(client_socket);

    if (r == END_CONN) {
        printf("Bad input.\n");
        goto end_conn;;
    } else if (r == TO_REGISTER) {
        registerAccount(client_socket);
    } else {
        r= login(client_socket, &account);
        if(r==TO_ADMIN) admin(client_socket, &account);
        else if(r==TO_MEMBER) member(client_socket, &account);
    }

    end_conn: close(client_socket);
    printf("Client %s:%d exits.\n", inet_ntoa((*address).sin_addr), ntohs((*address).sin_port));
}

int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, MAX_CONNECTIONS) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }
    printf("Server running at 127.0.0.1:%d...\n", PORT);

    while (1) {
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0) {
            perror("accept");
            exit(EXIT_FAILURE);
        }

        printf("Connection accepted from %s:%d\n", inet_ntoa(address.sin_addr), ntohs(address.sin_port));

        int pid = fork();
        if (pid == 0) {
            close(server_fd);
            handle_connection(new_socket, &address);
            exit(0);
        } else if (pid > 0) {
            close(new_socket);
        } else {
            perror("fork");
            exit(EXIT_FAILURE);
        }
    }

    return 0;
}
