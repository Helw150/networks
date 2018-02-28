/*
placeholder server include file
*/

#define MAX_USERS 500

struct User {
    char *name, *password;
 };

struct UserDB {
    struct User users[10];
};

struct RuntimeVals {
    int highest_socket, front, user_id[MAX_USERS], active_sockets[MAX_USERS], authenticated[MAX_USERS];
    char *cwds[MAX_USERS];
    fd_set tracked_sockets;
    char *response;
    struct UserDB user_db;
};
