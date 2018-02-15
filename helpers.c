#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <helpers.h>

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
