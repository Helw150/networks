#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <helpers.h>

#define PATH_MAX 1024


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
    commands.CD = compileRegex("CD [(A-z)(0-9)/\.][(A-z)(0-9)/\.]*");
    commands.cd_len = 3;
    commands.GET = compileRegex("GET [(A-z)(0-9)/\.][(A-z)(0-9)/\.]*");
    commands.get_len = 4;
    commands.PUT = compileRegex("PUT [(A-z)(0-9)/\.][(A-z)(0-9)/\.]*");
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
    strcpy(response, "successfully executed!\n");
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
