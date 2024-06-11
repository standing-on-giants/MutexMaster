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

void format_time(time_t rawtime, char *buffer, size_t buffer_size) {
    struct tm *timeinfo = localtime(&rawtime);
    strftime(buffer, buffer_size, "%Y-%m-%d %H:%M:%S", timeinfo);
}

char* formatHeading(char *text) {
    int textLength = strlen(text);
    int totalWidth = 52;
    int paddingLeft = (totalWidth - textLength - 2) / 2; // Padding for left side (2 extra for asterisks)
    int paddingRight = totalWidth - paddingLeft - textLength - 2; // Padding for right side (ensures even padding)

    // Allocate memory for the formatted string
    // 3 lines of width totalWidth, plus null terminator
    char* formattedText = (char*)malloc((totalWidth * 3 + 1) * sizeof(char)); 
    if (formattedText == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        exit(1);
    }

    // Construct the formatted string
    // Top border
    memset(formattedText, '*', totalWidth);
    formattedText[totalWidth] = '\n';
    formattedText[totalWidth + 1] = '\0';

    // Line with text
    strcat(formattedText, "*");
    for (int i = 0; i < paddingLeft; i++) strcat(formattedText, " ");
    strcat(formattedText, text);
    for (int i = 0; i < paddingRight; i++) strcat(formattedText, " ");
    strcat(formattedText, "*\n");

    // Bottom border
    memset(formattedText + strlen(formattedText), '*', totalWidth);
    formattedText[strlen(formattedText) + totalWidth] = '\n';
    formattedText[strlen(formattedText) + totalWidth + 1] = '\0';

    return formattedText;
}


void admin(int client_socket, struct Account *acc) { // admin operations
    char buffer[MAX_BUFFSIZE] = {0};
    char x[100];
    sprintf(x, "Welcome admin \"%s\"", acc->username);
    strcpy(buffer, formatHeading(x));
    strcat(buffer, "\n\n1. See all books and available copies.\n2. See incumbent allocations.\n3. Add book.\n4. Update copies of a particular book.\n5. Delete book (all copies).\n6. Allocate a book.\n7. Deallocate a book.\n8. See allocations to a particular user.\n9. View all users/members.\n10. Exit\n");
    send(client_socket, buffer, MAX_BUFFSIZE, 0);

    while (1) {
        memset(buffer, '\0', sizeof(buffer));
        send(client_socket, "\nEnter choice:", MAX_BUFFSIZE, 0);
        //printf("chabuuuk\n");
        int choice;
        recv(client_socket, &choice, sizeof(choice), 0);
        printf("Recvd choice %d\n", choice);

        FILE *bookFile, *allocFile;
        int fd;
        struct Book book;
        struct Allocation allocation;

        switch (choice) {
            case 1: // See all books and available copies
                bookFile = fopen(booksCol, "rb+");
                if (bookFile == NULL) {
                    send(client_socket, "Error opening book file.\n", strlen("Error opening book file.\n"), 0);
                    continue;
                }

                fd = fileno(bookFile);
                lock_file(fd, F_RDLCK); // Acquire read lock
                fseek(bookFile, 0, SEEK_SET);

                strcpy(buffer, "Books and available copies:\n");
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
                break;

            case 2: // See incumbent allocations
                allocFile = fopen(allocList, "rb+");
                if (allocFile == NULL) {
                    send(client_socket, "Error opening allocation file.\n", MAX_BUFFSIZE, 0);
                    continue;
                }

                fd = fileno(allocFile);
                lock_file(fd, F_RDLCK); // Acquire read lock
                fseek(allocFile, 0, SEEK_SET);

                memset(buffer, '\0', sizeof(buffer));
                strcpy(buffer, "Current allocations:\n");
                while (fread(&allocation, sizeof(struct Allocation), 1, allocFile)) {
                    if (allocation.delete == 0) {
                        char temp[200];
                        sprintf(temp, "Class ID: %s, Member Name: %s, Date of Issue: %s, Date of Return: %s\n",
                                allocation.class_id, allocation.name, allocation.dateOfIssue, allocation.dateOfReturn);
                        strcat(buffer, temp);
                    }
                }
                lock_file(fd, F_UNLCK); // Release lock
                fclose(allocFile);

                send(client_socket, buffer, MAX_BUFFSIZE, 0);
                break;

            case 3: // Add book
                send(client_socket, "Enter class ID: ", MAX_BUFFSIZE, 0);
                recv(client_socket, book.class_id, sizeof(book.class_id), 0);

                send(client_socket, "Enter book name: ", MAX_BUFFSIZE, 0);
                recv(client_socket, book.name, sizeof(book.name), 0);

                send(client_socket, "Enter number of copies: ", MAX_BUFFSIZE, 0);
                recv(client_socket, &book.copies, sizeof(book.copies), 0);

                book.delete = 0;

                bookFile = fopen(booksCol, "rb+");
                if (bookFile == NULL) {
                    send(client_socket, "Error opening book file.\n", MAX_BUFFSIZE, 0);
                    break;
                }

                fd = fileno(bookFile);
                lock_file(fd, F_WRLCK); // Acquire write lock
                struct Book bk;
                int book_added = 0;
                while (fread(&bk, sizeof(struct Book), 1, bookFile)) {
                    if (bk.delete == 1) {
                        fseek(bookFile, -sizeof(struct Book), SEEK_CUR);
                        fwrite(&book, sizeof(struct Book), 1, bookFile);
                        book_added = 1;
                        break;
                    }
                }
                if (!book_added) {
                    fseek(bookFile, 0, SEEK_END);
                    fwrite(&book, sizeof(struct Book), 1, bookFile);
                }

                lock_file(fd, F_UNLCK); // Release lock
                fclose(bookFile);

                send(client_socket, "Book added successfully.\n", MAX_BUFFSIZE, 0);
                break;

            case 4: // Update copies of a particular book
            {
                char class_id[4] = {0};
                int new_copies;

                send(client_socket, "Enter class ID of the book to update: ", MAX_BUFFSIZE, 0);
                recv(client_socket, class_id, sizeof(class_id), 0);

                send(client_socket, "Enter the new number of copies: ", MAX_BUFFSIZE, 0);
                recv(client_socket, &new_copies, sizeof(new_copies), 0);

                bookFile = fopen(booksCol, "rb+");
                if (bookFile == NULL) {
                    send(client_socket, "Error opening book file.\n", MAX_BUFFSIZE, 0);
                    break;
                }

                fd = fileno(bookFile);
                lock_file(fd, F_WRLCK); // Acquire write lock

                fseek(bookFile, 0, SEEK_SET);
                int found = 0;
                while (fread(&book, sizeof(struct Book), 1, bookFile)) {
                    if (book.delete == 0 && strcmp(book.class_id, class_id) == 0) {
                        found = 1;
                        book.copies += new_copies;
                        fseek(bookFile, -sizeof(struct Book), SEEK_CUR);
                        fwrite(&book, sizeof(struct Book), 1, bookFile);
                        break;
                    }
                }

                lock_file(fd, F_UNLCK); // Release lock
                fclose(bookFile);

                if (found) {
                    send(client_socket, "Book copies updated successfully.\n", MAX_BUFFSIZE, 0);
                } else {
                    send(client_socket, "Book not found.\n", MAX_BUFFSIZE, 0);
                }
                break;
            }

            case 5: // Delete book (all copies)
            {
                char class_id[4] = {0};

                send(client_socket, "Enter class ID of the book to delete: ", MAX_BUFFSIZE, 0);
                recv(client_socket, class_id, sizeof(class_id), 0);

                bookFile = fopen(booksCol, "rb+");
                if (bookFile == NULL) {
                    send(client_socket, "Error opening book file.\n", MAX_BUFFSIZE, 0);
                    break;
                }

                fd = fileno(bookFile);
                lock_file(fd, F_WRLCK); // Acquire write lock
                fseek(bookFile, 0, SEEK_SET);

                int found = 0;
                while (fread(&book, sizeof(struct Book), 1, bookFile)) {
                    if (strcmp(book.class_id, class_id) == 0 && book.delete == 0) {
                        found = 1;
                        book.delete = 1;
                        fseek(bookFile, -sizeof(struct Book), SEEK_CUR);
                        fwrite(&book, sizeof(struct Book), 1, bookFile);
                        break;
                    }
                }

                lock_file(fd, F_UNLCK); // Release lock
                fclose(bookFile);

                if (found) {
                    send(client_socket, "Book deleted successfully.\n", MAX_BUFFSIZE, 0);
                } else {
                    send(client_socket, "Book not found.\n", MAX_BUFFSIZE, 0);
                }
                break;
            }

            case 6: // Allocate a book
            {
                char member_username[MAX_USERNAME_LENGTH];
                char book_class_id[4];
                int duration_days;

                // Receive member username
                send(client_socket, "Enter member username: ", MAX_BUFFSIZE, 0);
                recv(client_socket, member_username, sizeof(member_username), 0);

                // Receive book class ID
                send(client_socket, "Enter book class ID: ", MAX_BUFFSIZE, 0);
                recv(client_socket, book_class_id, sizeof(book_class_id), 0);

                // Receive allocation duration in days
                send(client_socket, "Enter allocation duration (days): ", MAX_BUFFSIZE, 0);
                recv(client_socket, &duration_days, sizeof(duration_days), 0);

                // Open the members file to check if the user exists
                FILE *membersFile = fopen(memAccs, "rb");
                if (membersFile == NULL) {
                    send(client_socket, "Error opening members file.\n", MAX_BUFFSIZE, 0);
                    return;
                }

                struct Account member;
                int user_found = 0;
                while (fread(&member, sizeof(struct Account), 1, membersFile)) {
                    if (strcmp(member.username, member_username) == 0) {
                        user_found = 1;
                        break;
                    }
                }
                fclose(membersFile);

                if (!user_found) {
                    send(client_socket, "User not found.\n", MAX_BUFFSIZE, 0);
                    break;
                }

                // Open the books file to check if the book is available
                bookFile = fopen(booksCol, "rb+");
                if (bookFile == NULL) {
                    send(client_socket, "Error opening books file.\n", MAX_BUFFSIZE, 0);
                    break;
                }

                fd = fileno(bookFile);
                lock_file(fd, F_WRLCK); // Acquire write lock

                int book_found = 0;
                while (fread(&book, sizeof(struct Book), 1, bookFile)) {
                    if (strcmp(book.class_id, book_class_id) == 0 && book.delete == 0) {
                        book_found = 1;
                        if (book.copies > 0) {
                            book.copies--; // Decrease the number of available copies
                            fseek(bookFile, -sizeof(struct Book), SEEK_CUR);
                            fwrite(&book, sizeof(struct Book), 1, bookFile);
                        } else {
                            send(client_socket, "No copies available.\n", MAX_BUFFSIZE, 0);
                            book_found = -1;
                        }
                        break;
                    }
                }

                lock_file(fd, F_UNLCK); // Release lock
                fclose(bookFile);

                if (!book_found) {
                    if (book_found == -1) {
                        break; // No copies available
                    }
                    send(client_socket, "Book not found.\n", MAX_BUFFSIZE, 0);
                    break;
                }

                // Proceed to add allocation record
                allocFile = fopen(allocList, "rb+");
                if (allocFile == NULL) {
                    send(client_socket, "Error opening allocation file.\n", MAX_BUFFSIZE, 0);
                    break;
                }

                fd = fileno(allocFile);
                lock_file(fd, F_WRLCK); // Acquire write lock

                // Fill allocation details
                strcpy(allocation.name, member_username);
                strcpy(allocation.class_id, book_class_id);
                time_t tnow = time(NULL);
                strcpy(allocation.dateOfIssue, ctime(&tnow)); printf("TI: %s\n", allocation.dateOfIssue);

                struct tm *tm_info;
                time_t raw_time = tnow + (duration_days * 24 * 60 * 60);
                tm_info = localtime(&raw_time);
                raw_time = mktime(tm_info);
                strcpy(allocation.dateOfReturn, ctime(&raw_time)); printf("TI: %s\n", allocation.dateOfReturn);

                allocation.delete = 0;

                // Write to allocation file
                fseek(allocFile, 0, SEEK_SET);
                int alloc_write = 0;
                struct Allocation temp_alloc;
                while (fread(&temp_alloc, sizeof(struct Allocation), 1, allocFile)) {
                    if (temp_alloc.delete == 1) {
                        fseek(allocFile, -sizeof(struct Allocation), SEEK_CUR);
                        fwrite(&allocation, sizeof(struct Allocation), 1, allocFile);
                        alloc_write = 1;
                        break;
                    }
                }
                if (!alloc_write) {
                    fseek(allocFile, 0, SEEK_END);
                    fwrite(&allocation, sizeof(struct Allocation), 1, allocFile);
                }

                lock_file(fd, F_UNLCK); // Release lock
                fclose(allocFile);

                send(client_socket, "Book allocated successfully.\n", MAX_BUFFSIZE, 0);
                break;
            }

            case 7: // Deallocate a book
            {
                char member_username[MAX_USERNAME_LENGTH];
                char book_class_id[4];

                send(client_socket, "Enter member username: ", MAX_BUFFSIZE, 0);
                recv(client_socket, member_username, sizeof(member_username), 0);

                send(client_socket, "Enter book class ID: ", MAX_BUFFSIZE, 0);
                recv(client_socket, book_class_id, sizeof(book_class_id), 0);

                allocFile = fopen(allocList, "rb+");
                if (allocFile == NULL) {
                    send(client_socket, "Error opening allocation file.\n", MAX_BUFFSIZE, 0);
                    break;
                }

                fd = fileno(allocFile);
                lock_file(fd, F_WRLCK); // Acquire write lock

                int allocation_found = 0;
                fseek(allocFile, 0, SEEK_SET);
                while (fread(&allocation, sizeof(struct Allocation), 1, allocFile)) {
                    if (allocation.delete == 0 && strcmp(allocation.name, member_username) == 0 && strcmp(allocation.class_id, book_class_id) == 0 ) {
                        allocation_found = 1;
                        allocation.delete = 1; // Mark allocation as deleted
                        fseek(allocFile, -sizeof(struct Allocation), SEEK_CUR);
                        fwrite(&allocation, sizeof(struct Allocation), 1, allocFile);
                        break;
                    }
                }

                lock_file(fd, F_UNLCK); // Release lock
                fclose(allocFile);

                if (!allocation_found) {
                    send(client_socket, "Allocation not found.\n", MAX_BUFFSIZE, 0);
                    break;
                }

                // Increment book copies
                bookFile = fopen(booksCol, "rb+");
                if (bookFile == NULL) {
                    send(client_socket, "Error opening book file.\n", MAX_BUFFSIZE, 0);
                    break;
                }

                fd = fileno(bookFile);
                lock_file(fd, F_WRLCK); // Acquire write lock
                fseek(bookFile, 0, SEEK_SET);

                int book_updated = 0;
                while (fread(&book, sizeof(struct Book), 1, bookFile)) {
                    if (strcmp(book.class_id, book_class_id) == 0 && book.delete == 0) {
                        book.copies++;
                        fseek(bookFile, -sizeof(struct Book), SEEK_CUR);
                        fwrite(&book, sizeof(struct Book), 1, bookFile);
                        book_updated = 1;
                        break;
                    }
                }

                lock_file(fd, F_UNLCK); // Release lock
                fclose(bookFile);

                if (book_updated) {
                    send(client_socket, "Book deallocated successfully.\n", MAX_BUFFSIZE, 0);
                } else {
                    send(client_socket, "Book not found for updating.\n", MAX_BUFFSIZE, 0);
                }
                break;
            }

            case 8: // See allocations to a particular user
            {
                char member_username[MAX_USERNAME_LENGTH];

                send(client_socket, "Enter member username: ", MAX_BUFFSIZE, 0);
                recv(client_socket, member_username, sizeof(member_username), 0);

                allocFile = fopen(allocList, "rb+");
                if (allocFile == NULL) {
                    send(client_socket, "Error opening allocation file.\n", MAX_BUFFSIZE, 0);
                    break;
                }

                fd = fileno(allocFile);
                lock_file(fd, F_RDLCK); // Acquire read lock
                fseek(allocFile, 0, SEEK_SET);

                strcpy(buffer, "Allocations for user:\n");
                int found = 0;
                while (fread(&allocation, sizeof(struct Allocation), 1, allocFile)) {
                    if (strcmp(allocation.name, member_username) == 0 && allocation.delete == 0) {
                        found = 1;
                        char temp[200];
                        sprintf(temp, "Class ID: %s, Date of Issue: %s, Date of Return: %s\n",
                                allocation.class_id, allocation.dateOfIssue, allocation.dateOfReturn);
                        strcat(buffer, temp);
                    }
                }

                lock_file(fd, F_UNLCK); // Release lock
                fclose(allocFile);

                if (found) {
                    send(client_socket, buffer, MAX_BUFFSIZE, 0);
                } else {
                    send(client_socket, "No allocations found for this user.\n", MAX_BUFFSIZE, 0);
                }
                break;
            }

            case 9:{ // view all members
                FILE * memFile=fopen(memAccs,"rb");
                int fd=fileno(memFile);
                lock_file(fd, F_RDLCK); // Acquiring read lock
                fseek(memFile, 0, SEEK_SET);
                memset(buffer, '\0', sizeof(buffer));
                struct Account acc;
                while(fread(&acc, sizeof(struct Account), 1, memFile)){
                    char temp[110];
                    sprintf(temp, "Username: %s, Date of joining: %s\n",acc.username, acc.joiningTime);
                    strcat(buffer, temp);
                }
                lock_file(fd, F_UNLCK); // releasing read lock
                fclose(memFile);
                send(client_socket, buffer, MAX_BUFFSIZE,0);
            
            }break;

            case 10: // Exit
                return;

            default:
                send(client_socket, "Invalid choice. Try again.\n", MAX_BUFFSIZE, 0);
                break;
        }
    }
}

void member(int client_socket, struct Account *acc) { //member operations
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

        FILE *bookFile, *allocFile;
        int fd;
        struct Book book;
        struct Allocation allocation;

        switch (choice) {
            case 1: // View books in library
                bookFile = fopen(booksCol, "rb+");
                if (bookFile == NULL) {
                    send(client_socket, "Error opening book file.\n", strlen("Error opening book file.\n"), 0);
                    continue;
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
                break;

            case 2: // View current issues (allocations)
                allocFile = fopen(allocList, "rb+");
                if (allocFile == NULL) {
                    send(client_socket, "Error opening allocation file.\n", strlen("Error opening allocation file.\n"), 0);
                    continue;
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
                break;

            case 3: // Exit
                send(client_socket, "Exiting member panel.\n", strlen("Exiting member panel.\n"), 0);
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
