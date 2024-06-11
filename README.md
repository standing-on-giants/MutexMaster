# OLMS - Online Library Management System

OLMS is a mini project made as a part of the Operating Systems Lab Course.

## Context

The project assumes that the library is huge, with ample books, members, and administrators. The members can self-register to become authorized members of the library. They choose the books and approach the administrators to authorize the issue. In a large library, administrators might sometimes be unable to interact as fluently as they would face-to-face. Hence, mutual exclusion is required to prevent collisions in "transactions" (in a rough sense only). The administrators can add books, authorize issues, and revoke them, while members can view their issued books, due dates, and books available in the library at their convenience. For the current case, even admin can self-register, but an additional authorization can be put in place, as the case may be. Numerous queries for the admin can be explored. A few have been implemented here as described by the menu.

## Salient Features

- It utilizes a concurrent server to handle multiple clients that connect to it.
- File locking is implemented using fcntl to ensure mutual exclusion and protect critical sections, thus ensuring concurrency control.
- User Authentication: Members are required to pass through a login system to access their accounts, ensuring data privacy and security.

## How to run?

1. Clone the repository.
2. Run the following command to make all: 
```
make all
```
3. After step 2, one can see 2 executable files: "client" and "server". Run the following command to start the server: 
```
./server
```
4. On different terminals (since for testing purposes the client and server will be on the same machine), run the following command to start the client: 
```
./client
```
5. To remove the executables, run the following command: 
```
make clean
```
