#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <helpers.h>

#define PATH_MAX 1024


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

// Turning answer from https://stackoverflow.com/questions/1085083/regular-expressions-in-c-examples into a function
regex_t compileRegex(char *regex){
    int reti;
    regex_t regex_obj;
    reti = regcomp(&regex_obj, regex, 0);
    if (reti) {
	perror("Improper Regex");
	exit(EXIT_FAILURE);
    }
    return regex_obj;
}

// simple wrapper of regex check
int checkRegex(regex_t regex, char *check_string){
    return !regexec(&regex, check_string, 0, NULL, 0);
}

// Compile all regex checks for commands
struct CommandRegex compileAllCommandChecks(){
    struct CommandRegex commands;
    commands.USER = compileRegex("USER [A-z][A-z]*");
    commands.user_len = 5;
    commands.PASS = compileRegex("PASS [A-z][A-z]*");
    commands.pass_len = 5;
    commands.LS = compileRegex("LS");
    commands.ls_len = 2;
    commands.PWD = compileRegex("PWD");
    commands.pwd_len = 3;
    commands.CD = compileRegex("CD [(A-z)(0-9)/.][(A-z)(0-9)/.]*");
    commands.cd_len = 3;
    commands.GET = compileRegex("GET [(A-z)(0-9)/.][(A-z)(0-9)/.]*");
    commands.get_len = 4;
    commands.PUT = compileRegex("PUT [(A-z)(0-9)/.][(A-z)(0-9)/.]*");
    commands.put_len = 4;
    commands.QUIT = compileRegex("QUIT");
    commands.quit_len = 4;
    return commands;
}

// Pulls away the first num_char chars and cleans the string
char* stripStartingChars(int num_chars, char *string){
    for(int i = 0; i < num_chars; i++){
	if(string[i] != '\n'){
	    string++;
	}
    }
    while(isspace(string[strlen(string)-1])){
	string[strlen(string)-1] = 0;
    }
    return string;
}


char *lsCommand(char *cwd){
    FILE *fp;
    int status;
    char path[PATH_MAX];

    char command[1024];
    strcpy(command, "ls -l ");
    strcat(command, cwd);
	    
    fp = popen(command, "r");
    if (fp == NULL)
	return "wrong command usage!\n";

    char *response = (char *) malloc(sizeof(char) * 10000);
    while (fgets(path, PATH_MAX, fp) != NULL)
	strcat(response, path);
    status = pclose(fp);
    return response;
}

char *pwdCommand(char *cwd){
    char *tmp = (char *) malloc(sizeof(char) * 1024);
    strcpy(tmp, "successfully executed!\n");
    strcat(tmp, cwd);
    strcat(tmp, "\n");
    return tmp;
}
