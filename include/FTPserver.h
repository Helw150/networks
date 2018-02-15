/*
placeholder server include file
*/

#define MAX_USERS 500

struct SetupVals {
    int server_fd, addrlen;
    struct sockaddr_in address;
};

struct RuntimeVals {
    int highest_socket, front, user_id[MAX_USERS], active_sockets[MAX_USERS], authenticated[MAX_USERS];
    fd_set tracked_sockets;
    char *response;
};

struct User {
    char *name, *password;
 };

struct UserDB {
    struct User users[10];
};
