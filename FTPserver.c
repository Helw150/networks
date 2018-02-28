// Server side starter from Nabil's Announcement
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/select.h>
#include <sys/sendfile.h>
#include <unistd.h>

#include <helpers.h>
#include <FTPserver.h>

#define PORT 8080

// Creates some fake users for our "database"
struct UserDB createUsers(){
    struct UserDB user_db;
    user_db.users[0].name = "user1";
    user_db.users[0].password = "pass1";
    user_db.users[1].name = "user2";
    user_db.users[1].password = "pass2";
    user_db.users[2].name = "user3";
    user_db.users[2].password = "pass3";
    user_db.users[3].name = "user4";
    user_db.users[3].password = "pass4";
    user_db.users[4].name = "user5";
    user_db.users[4].password = "pass5";
    user_db.users[5].name = "user6";
    user_db.users[5].password = "pass6";
    user_db.users[6].name = "user7";
    user_db.users[6].password = "pass7";
    user_db.users[7].name = "user8";
    user_db.users[7].password = "pass8";
    user_db.users[8].name = "user9";
    user_db.users[8].password = "pass9";
    user_db.users[9].name = "user0";
    user_db.users[9].password = "pass0";
    return user_db;
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
	send(new_socket, reject_message, strlen(reject_message), 0 );
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
    // If this socket has been authenticated`
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
	   Nabil at some point said that LS had to be sent over the data port. However, this doesn't match what is in the RFC or the Pseudo-code in the PDF. I am going to ignore that comment and send LS over the regular socket.
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
	}  else if(checkRegex(commands.GET, buffer)) {
	    struct sockaddr_in address;
	    int addrlen = sizeof(address);
	    getpeername(runtime.active_sockets[array_int], (struct sockaddr *)&address, (socklen_t*)&addrlen);
	    char *data_ip  = (char *) malloc(sizeof(char) * 1024);
	    inet_ntop(AF_INET, &address.sin_addr, data_ip, 1024);
	    int data_port = ntohs(address.sin_port)+1;
	    int data_sock = connectToSocket(data_ip, data_port);
	    char path[1024];
	    strcpy(path, runtime.cwds[array_int]);
	    strcat(path, "/");
	    strcat(path, stripStartingChars(commands.get_len, buffer));
	    runtime.response = transferFile(data_sock, path);
	}else {
	    runtime.response = "Invalid FTP command\n";
	}
    }
    // Return runtime variables with updated tables and response
    return runtime;
}

int main(int argc, char const *argv[]){
    struct CommandRegex commands = compileAllCommandChecks(); 
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
    memset(&runtime.active_sockets[0], -1, sizeof(runtime.active_sockets));
    
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
		    // Unexpected Close
		    if(valread == 0){
			// Close Socket
			close(runtime.active_sockets[i]);
			// Remove from FD tracking in the future
			runtime.active_sockets[i] = -1;
			// Remove from CWDs
			runtime.cwds[i] = "/home/";
			// De-authenticate
			runtime.authenticated[i] = 0;
			runtime.user_id[i] = -1;
			break;
		    }
		    char *command = strtok(buffer, "\n");
		    while(command != NULL) {
			printf("%d: %s\n", runtime.active_sockets[i] , command);
			if(checkRegex(commands.QUIT, command) || valread == 0){
			    // Close Socket
			    close(runtime.active_sockets[i]);
			    // Remove from FD tracking in the future
			    runtime.active_sockets[i] = -1;
			    // Remove from CWDs
			    runtime.cwds[i] = "/home/";
			    // De-authenticate
			    runtime.authenticated[i] = 0;
			    runtime.user_id[i] = -1;
			    break;
			} else {
			    runtime = handleCommand(command, i, runtime);
			    send(runtime.active_sockets[i], runtime.response, strlen(runtime.response), 0);
			    command = strtok(NULL, "\n");
			}
		    }
		}
	    }
	memset(&buffer[0], 0, sizeof(buffer));
    }
    return 0;
}
