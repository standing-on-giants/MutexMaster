#ifndef AUTH_H
#define AUTH_H

#define memAccs "db_files/memAccounts.bin"
#define admAccs "db_files/adminAccounts.bin"
#define booksCol "db_files/booksCollection.bin"
#define allocList "db_files/allocationsList.bin"
#define MAX_BUFFSIZE 1024
#define MAX_USERNAME_LENGTH 50
#define MAX_PASSWORD_LENGTH 50
#define INVALID_ENTITY 0
#define INVALID_CREDENTIALS 1
#define ACCOUNT_FOUND 2

#define END_CONN 0
#define TO_LOGIN 2
#define TO_REGISTER 1

#define TO_ADMIN 11
#define TO_MEMBER 10

// Enumeration for account types
enum AccountType {
    MEMBER,
    ADMIN
};

// Structure to hold user account information
struct Account {
    char username[MAX_USERNAME_LENGTH];
    char password[MAX_PASSWORD_LENGTH];
    enum AccountType type;
    char joiningTime[30];
};

struct Book{
	char class_id[4]; // 3 digit
	char name[50];
	int copies;
	uint8_t delete;
};

struct Allocation{
	char class_id[4]; // book
	char name[50]; // name of member
	char dateOfIssue[30]; // date of issue
	char dateOfReturn[30];// date of return
	uint8_t delete;
};

char* formatHeading(char *text);
void format_time(time_t rawtime, char *buffer, size_t buffer_size);

// Function prototypes for the cases for member
void viewBooksInLibrary(int client_socket);
void viewCurrentIssues(int client_socket, struct Account *acc);
void exitMemberPanel(int client_socket);

// Function prototypes for the cases for admin
void seeAllBooks(int client_socket);
void seeAllocations(int client_socket);
void addBook(int client_socket);
void updateBookCopies(int client_socket);
void deleteBook(int client_socket); 
void allocateBook(int client_socket);
void deallocateBook(int client_socket);
void seeAllocationsForUser(int client_socket);
void viewAllUsers(int client_socket);
void exitAdminPanel(int client_socket);

#endif