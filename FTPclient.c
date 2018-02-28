// Client side C/C++ program to demonstrate Socket programming
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>

#include <FTPclient.h>
#include <helpers.h>

void localCommand(char* input, struct CommandRegex commands){
    char *cwd = (char *) malloc(sizeof(char) * 1024);
    getcwd(cwd, 1024);
    if(checkRegex(commands.LS, input)){
	printf("%s", lsCommand(cwd));
    } else if (checkRegex(commands.PWD, input)) {
	chdir(".");
	printf(pwdCommand(cwd));
    } else if (checkRegex(commands.CD, input)) {
	char* dir = stripStartingChars(commands.cd_len, input);
	int ret = chdir(dir);
	if(ret == 0){ // If the directory is valid
	    getcwd(cwd, 1024);
	    // Return the pwd command so they know where they changed to
	    printf(pwdCommand(cwd));
	} else {
	    printf("Invalid target directory!\n");
	}
    }
    return;
}

int main(int argc, char const *argv[])
{
    char *ip_addr = argv[1];
    struct CommandRegex commands = compileAllCommandChecks();
    int PORT = atoi(argv[2]);
    struct sockaddr_in address;
    int sock = 0, valread;
    struct sockaddr_in serv_addr;
    char buffer[1024] = {0};
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
	    printf("\n Socket creation error \n");
	    return -1;
	}

    memset(&serv_addr, '0', sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    // Convert IPv4 and IPv6 addresses from text to binary form
    if(inet_pton(AF_INET, ip_addr, &serv_addr.sin_addr)<=0)
	{
	    printf("\nInvalid address/ Address not supported \n");
	    return -1;
	}

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
	{
	    printf("\nConnection Failed \n");
	    return -1;
	}
    int data_port;
    getsockname(sock, (struct sockaddr *)&address, sizeof(address));
    data_port = ntohs(address.sin_port)+1;
    char input[128];
    while(1){
	memset(&buffer[0], 0, sizeof(buffer));
	memset(&input[0], 0, sizeof(input));
	printf("ftp> ");
	fgets(input, 128, stdin);
	if(input[0] == '!'){
	    // Remove the local spec
	    char* local_input = stripStartingChars(1, input);
	    localCommand(local_input, commands);
	}
	else {
	    send(sock, input, strlen(input), 0);
	    valread = read(sock , buffer, 1024);
	    printf("%s", buffer);
	    if(checkRegex(commands.QUIT, input)){
		return 0;
	    }
	}
    }
    return 0;
}
