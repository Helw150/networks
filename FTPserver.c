// Server side starter from Nabil's Announcement
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/select.h>
#include <unistd.h>

#include <FTPserver.h>
#include <helpers.h>

#define PORT 8080

// Creates some fake users for our "database"
struct UserDB createUsers(){
    struct UserDB user_db;
    user_db.users[0].name = "Yasir";
    user_db.users[0].password = "Zaki";
    user_db.users[1].name = "Will";
    user_db.users[1].password = "Held";
    user_db.users[2].name = "Guyu";
    user_db.users[2].password = "Fan";
    user_db.users[3].name = "Paula";
    user_db.users[3].password = "Dosza";
    user_db.users[4].name = "Jerome";
    user_db.users[4].password = "White";
    user_db.users[5].name = "Megan";
    user_db.users[5].password = "Moore";
    user_db.users[6].name = "Christina";
    user_db.users[6].password = "Popper";
    user_db.users[7].name = "Nizar";
    user_db.users[7].password = "Habash";
    user_db.users[8].name = "God";
    user_db.users[8].password = "fried";
    user_db.users[9].name = "Jay";
    user_db.users[9].password = "Chen";
    return user_db;
}


struct SetupVals setupAndBind(int port_number, int opt){
    struct SetupVals setup;
    setup.addrlen = sizeof(setup.address);
    // Creating socket file descriptor
    if ((setup.server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
	{
	    perror("socket failed");
	    exit(EXIT_FAILURE);
	}

    // Forcefully attaching socket to the port 8080
    if (setsockopt(setup.server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT,
		   &opt, sizeof(opt)))
	{
	    perror("setsockopt");
	    exit(EXIT_FAILURE);
	}
    setup.address.sin_family = AF_INET;
    setup.address.sin_addr.s_addr = INADDR_ANY;
    setup.address.sin_port = htons(port_number);

    // Forcefully attaching socket to the port 8080
    if (bind(setup.server_fd, (struct sockaddr *)&setup.address, sizeof(setup.address)) < 0)
	{
	    perror("bind failed");
	    exit(EXIT_FAILURE);
	}
    // Setup listener on a master socket and let 3 people wait there
    if (listen(setup.server_fd, 3) < 0)
	{
	    perror("listen");
	    exit(EXIT_FAILURE);
	}
    return setup;
}

// Setup a new connection coming from the master port
struct RuntimeVals createNewConnection(struct SetupVals setup, struct RuntimeVals runtime){
    int new_socket;
    // Accept command to return the socket number which this new connection has been assigned
    if ((new_socket = accept(setup.server_fd, (struct sockaddr *)&setup.address,
			     (socklen_t*)&setup.addrlen))<0)
	{
	    perror("accept");
	    exit(EXIT_FAILURE);
	}
    // We need highest socket number for select, so keep track of this
    if(new_socket > runtime.highest_socket){
	runtime.highest_socket = new_socket;
    }
    // Check if the socket array has any free space
    int fill_empty = 0;
    for(int i = 0; i < runtime.front; i++){
	// If it does place our new socket there and break
	if(runtime.active_sockets[i] == -1){
	    runtime.active_sockets[i] = new_socket;
	    fill_empty = 1;
	    break;
	}
    }
    // If there weren't free spots in the array and the array isn't capped out
    if(!fill_empty && runtime.front < MAX_USERS){
	// place it at the front of the array and update front
	runtime.active_sockets[runtime.front] = new_socket;
	runtime.front++;
    } else if (runtime.front > MAX_USERS){
	// If the table is totally full, tell the client and close the socket
	char *reject_message = "Server is currently at max capacity";
	send(new_socket , reject_message, strlen(reject_message), 0 );
	close(new_socket);
    }
    return runtime;
}


struct RuntimeVals prepareSelect(struct RuntimeVals runtime, struct SetupVals setup){
    // Add master socket to our listening set
    FD_SET(setup.server_fd, &runtime.tracked_sockets);
    // For all the parts of the array that have had sockets given
    for( int i = 0; i < runtime.front; i++ )
	    {
		// If this array item is an active socket
		if(runtime.active_sockets[i] != -1) {
		    // Add that socket to the set
		    FD_SET(runtime.active_sockets[i], &runtime.tracked_sockets);
		}
	    }
    return runtime;
}

struct RuntimeVals serverCommandCD(struct RuntimeVals runtime, int array_int, char* dir){
    // First change to this users cwd
    chdir(runtime.cwds[array_int]);
    // Then change to the requested directory
    int ret = chdir(dir);
    if(ret == 0){ // If the directory is valid
	char *tmp = (char *) malloc(sizeof(char) * 10000);
	getcwd(tmp, 10000);
	// Update CWD in our store
	runtime.cwds[array_int] = tmp;
	// Return the pwd command so they know where they changed to
	runtime.response = pwdCommand(runtime.cwds[array_int]);
    } else {
	runtime.response = "Invalid command syntax or target directory!\n";
    }
    return runtime;
}

struct RuntimeVals passCommand(struct RuntimeVals runtime, int array_int, char* password){
    // If this socket does not have a set user then tell them to set the user
    if ( runtime.user_id[array_int] == -1){
	runtime.response = "set USER first\n";
    } else {
	// The socket has an associated user so check pass	
	// If the password is correctly associated
	if(!strcmp(password, runtime.user_db.users[runtime.user_id[array_int]].password)){
	    // Authenticate the user if the password is correct
	    runtime.response = "Authentication complete\n";
	    runtime.authenticated[array_int] = 1;
	    runtime.cwds[array_int] = "/home/";
	} else {
	    // If not tell them wrong password
	    runtime.response = "wrong password\n";
	}
    }
    return runtime;
}

struct RuntimeVals userCommand(struct RuntimeVals runtime, int array_int, char* username){
    int seen = 0;
    // Check if the username is in our "Database"
    for(int i = 0; i < 10; i++){
	// If it is then set the sockets user id to that user and set our response
	if(!strcmp(username, runtime.user_db.users[i].name)){
	    runtime.user_id[array_int] = i;
	    runtime.response = "Username OK, password required\n";
	    seen = 1;
	    break;
	}
    }
    // If this user is not in the DB then respond as such
    if(!seen) runtime.response = "Username does not exist\n";
    return runtime;
}

struct RuntimeVals handleCommand(char buffer[1024], int array_int, struct RuntimeVals runtime){
    // Compile all the commands which allow me to check which FTP command is being sent
    struct CommandRegex commands = compileAllCommandChecks();
    // If this socket has been authenticated
    if(!runtime.authenticated[array_int]){
	// Is this a valid user command
	if(checkRegex(commands.USER, buffer)){
	    char *username = stripStartingChars(commands.user_len, buffer);
	    runtime = userCommand(runtime, array_int, username);
	} else if(checkRegex(commands.PASS, buffer)) { // Is Valid PASS command
	    char *password = stripStartingChars(commands.pass_len, buffer);
	    runtime = passCommand(runtime, array_int, password);
	} else {
	    // anything besides USER and PASS when un-authenticated returns this message
	    runtime.response = "Authenticate first\n";
	}
    } else {
	/* Handles commands once authenticated*/
	/* 
	   NEED TO CHANGE LS TO SEND VIA A DATA PORT 
	   CREATE DATA CONNECTION AND SEND
	 */
	if(checkRegex(commands.LS, buffer)){
	    // Lists the directory stored as this users CWD
	    runtime.response = lsCommand(runtime.cwds[array_int]);
	} else if(checkRegex(commands.PWD, buffer)){
	    /* Since we have multiple users on the server, we cannot rely on the system PWD - instead use our own storage of the directory each user is on */
	    runtime.response = pwdCommand(runtime.cwds[array_int]);
	} else if(checkRegex(commands.CD, buffer)) {
	    // Changes directory to given input based on cwd
	    char *dir = stripStartingChars(commands.cd_len, buffer);
	    runtime = serverCommandCD(runtime, array_int, dir);
	} else {
	    runtime.response = "Invalid FTP command\n";
	}
    }
    // Return runtime variables with updated tables and response
    return runtime;
}

int main(int argc, char const *argv[])
{
    // Length of the values from the read command
    int valread;
    int opt = 1;
    // Create an empty buffer to store commands
    char buffer[1024] = {0};

    // Values involved in setting up the initial port as a listener for incoming new requests
    struct SetupVals setup = setupAndBind(PORT, opt);
    int addrlen = sizeof(setup.address);

    
    // Handles values that will be changed during the runtime of the server
    struct RuntimeVals runtime;
    runtime.front = 0;
    runtime.highest_socket = setup.server_fd;
    memset(runtime.user_id, -1, MAX_USERS * sizeof(runtime.user_id[0]));
    //Create a toy "database" of users for the purposes of authentication
    runtime.user_db = createUsers();
    
    
    while(1){
	// Clear the buffer between every setup;
	memset(&buffer[0], 0, sizeof(buffer));
	// Create and set up my FD_SET so that select will work
	runtime = prepareSelect(runtime, setup);
	// Wait until at least one socket is updated with a command
	select( runtime.highest_socket + 1 , &runtime.tracked_sockets , NULL , NULL , NULL);
	// If the socket pinged is the master socket this is a new connection
	if(FD_ISSET(setup.server_fd, &runtime.tracked_sockets)){
	    runtime = createNewConnection(setup, runtime);
	}
	// Check all active sockets and see if something has been sent there, if it has do something with it.
	for( int i = 0; i < runtime.front; i++ )
	    {
		if (FD_ISSET( runtime.active_sockets[i] , &runtime.tracked_sockets)){
		    valread = read( runtime.active_sockets[i] , buffer, 1024);
		    printf("%d: %s\n", runtime.active_sockets[i] ,buffer );
		    char *command = strtok(buffer, "\r\n");
		    /* 
		       NEED TO HANDLE MULTIPLE COMMANDS IN ONE
		       IN ONE FILE READ SEPARATED BY NEWLINE
		    */
		    while(command != NULL) {
			runtime = handleCommand(command, i, runtime);
			send(runtime.active_sockets[i], runtime.response, strlen(runtime.response), 0);
			command = strtok(NULL, "\n");
		    }
		}
	    }
    }
    return 0;
}
