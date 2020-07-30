/*
 ============================================================================
 Name        : baash.c
 Author      : Sebastian Murillo
 Version     : 1.1
 Copyright   : GPL
 Description : bash-like linux command interpreter
 ============================================================================
 */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#include "sebastr.h"
#include "readline/readline.h"
#include "readline/history.h"

//auto-test
//#define test

// configs:
#define verbose
#define SLOWMODE 0
#define ASCII 1
#define STDERR_REDIRECT_CHAR 200
#define MAX_ARGUMENTS 20
#define SCREEN_START 0
#define SCREEN_FINISH 1

bool isChild();
bool findFile(char* file, char* result);
bool changeDirectory(char* path);
bool syntaxOK(char* command);
void prompt();
void procreate();
void showScreen(int screen);
void parse(char* cmdline);
void switchPipes();
void execute(char* instruction, int INPUT, int OUTPUT);
void checkFileRedirection(char* instruction);
void redirect(int FD_input, int FD_output);

char arguments[MAX_STRING];
char pName[MAX_STRING];
pid_t son = 0;
int children = 0;
int STDIN_backup;
int STDOUT_backup;
int FD_read = STDIN_FILENO;
int FD_write = STDOUT_FILENO;
int pipeA[2] = {STDIN_FILENO, STDOUT_FILENO};
int pipeB[2] = {STDIN_FILENO, STDOUT_FILENO};

int main(int argc, char* argv[]){
	showScreen(SCREEN_START);
	chdir(getenv("HOME")); // starting dir
	sprintf(pName, "%s", "MAIN");
	STDIN_backup = dup(fileno(stdin));
	STDOUT_backup = dup(fileno(stdout));
	// loop
	char command[MAX_STRING];
	while(true){
		prompt();
#ifdef test
		//      TESTs: el '_' para substituir el enter ('\n')
		//sprintf(command, "%s", "cmd1 arg11 | cmd2 & cmd3 arg31 arg32 | cmd4 arg41 arg42 arg43_");
		sprintf(command, "%s", "ps -A | grep chromium | /bin/grep 00:00:00_");
		//sprintf(command, "%s", "ls | cat_");
		//sprintf(command, "%s", "cmd1 | cmd2 | cmd3 | cmd4_");
		//sprintf(command, "%s", "ls | wc_");
		//sprintf(command, "%s", "ls > file_output.txt_");
		//sprintf(command, "%s", "    command    2>&1    something    >file_output.txt    <     file_input.txt      arg     ");
		//sprintf(command, "%s", "cat < file_input.txt > file_output.txt_");
		//sprintf(command, "%s", "date&ls&date&ls&date&ls&date&ls&date&ls&_");
#else
		// no command
		fgets(command, MAX_STRING, stdin);
#endif
		if(!strcmp(command, "\n")) continue;
		// change directory
		if(!strncmp(command, "cd ", 3)){
			command[strlen(command) - 1] = '\0';
			if(!changeDirectory(&command[3])) printf(">baash - cd: inexistent directory [%s]\n", &command[3]);
			continue;
		}
		if(!strcmp(command, "cd\n")) continue;
		// exit
		if(!strcmp(command, "exit\n")){
			showScreen(SCREEN_FINISH);
			exit(EXIT_SUCCESS);
		}
		command[strlen(command) - 1] = '\0'; // remove '\n'
		if(!syntaxOK(command)){
			printf(">baash - incorrect syntax\n");
			continue;
		}
		parse(command);
		// restore STD_IO
		dup2(STDIN_backup, fileno(stdin));
		dup2(STDOUT_backup, fileno(stdout));
#ifdef verbose
		printf("%s is done, waiting for his %d children...\n", pName, children);
#endif
		while(children > 0){
			wait(0);
			children--;
		}
#ifdef verbose
		printf("%s's children have finished\n", pName);
#endif
#ifdef test
		showScreen(SCREEN_FINISH);
		exit(EXIT_SUCCESS);
#endif
	}
}

bool isChild(){
	if(son == 0) return true;
	return false;
}

bool findFile(char* file, char* result){
	// absolute path
	if(file[0] == '/'){
		if(access(file, F_OK) == 0){
			strcpy(result, file);
			return true;
		}
		result = NULL;
		return false;
	}
	//relative path
	char current[MAX_STRING];
	getcwd(current, MAX_STRING);
	sprintf(result, "%s/%s", current, file);
	if(access(result, F_OK) == 0) return true;
	//search $PATH
	char* PATH = getenv("PATH");
	strcat(PATH, ":");
	while(strext(PATH, result, "", ":")){
		PATH += strlen(result) + 1;
		sprintf(result, "%s/%s", result, file);
		if(access(result, F_OK) == 0) return true;
	}
	result = NULL;
	return false;
}

bool changeDirectory(char* path){
#ifdef verbose
	printf("changing directory to: [%s]\n", path);
#endif
	if(!chdir(path)) return true;
	return false;
}

bool syntaxOK(char* command){
	strfix(command, ' ');
	if(command[0] == '|') return false;
	if(command[strlen(command) - 1] == '|') return false;
	return true;
}

void prompt(){
	char current[MAX_STRING];
	getcwd(current, MAX_STRING);
	printf("[baash - %s]: ", current);
}

void procreate(){
	if((son = fork()) < 0){
		perror("miscarriage"); // dark...
		exit(EXIT_FAILURE);
	}
	if(!isChild()) children++;
	else{ //isChild
#ifdef verbose
		printf("%s has a new child -> son %d\n", pName, getpid());
#endif
		children = 0;
	}
}

void showScreen(int screen){
	if(ASCII){
		puts(R"EOF(*      _                         _          *)EOF");
		puts(R"EOF(*     | |                       | |         *)EOF");
		puts(R"EOF(*     | |__    __ _   __ _  ___ | |__       *)EOF");
		puts(R"EOF(*     | '_ \  / _` | / _` |/ __|| '_ \      *)EOF");
		puts(R"EOF(*     | |_) || (_| || (_| |\__ \| | | |     *)EOF");
		puts(R"EOF(*     |_.__/  \__,_| \__,_||___/|_| |_|     *)EOF");
		puts(R"EOF(*                                           *)EOF");
		if(screen == SCREEN_START) puts(R"EOF(*                  STARTED                  *)EOF");
		if(screen == SCREEN_FINISH) puts(R"EOF(*                TERMINATED                 *)EOF");
		return;
	}
	if(screen == SCREEN_START) printf(">baash started\n");
	if(screen == SCREEN_FINISH) printf(">baash terminated.\n");
}

void parse(char* cmdline){
	if(SLOWMODE) sleep(5);
	strfix(cmdline, ' ');
#ifdef verbose
	printf("%s is %-12s [%s]\n", pName, "parsing:", cmdline);
#endif
	char result[MAX_STRING];
	// repalce "2>&1" for STDERR_REDIRECT_CHAR
	sprintf(result, "%c", STDERR_REDIRECT_CHAR);
	strrep(cmdline, "2>&1", result);
	// run concurrently
	if(strext(cmdline, result, "", "&")){
		procreate();
		if(isChild()){ // son parses result
			parse(result);
			exit(EXIT_SUCCESS);
		}
		children--;
		strext(cmdline, result, "&", "");
		parse(result); // parent parses after & and doesn't wait
		return;
	}
	// run sequentially
	if(cmdline[0] == '|'){ // caso [| cmd2 |] y [| cmd4]
		switchPipes();
		FD_read = pipeA[0];
		close(pipeA[1]);
#ifdef verbose
		printf("%s opened INPUT pipe. FD: [%d]\n", pName, FD_read);
#endif
		if(strext(&cmdline[1], result, "", "|")){ // caso [| cmd2 |]
			if(pipe(pipeB) < 0){
				perror("call the plumber @ pipe opening11");
				exit(EXIT_FAILURE);
			}
			FD_write = pipeB[1];
#ifdef verbose
			printf("%s opened OUTPUT pipe (I). FD: [%d]\n", pName, FD_write);
#endif
		}
		else{ // caso [| cmd4]
			strext(&cmdline[1], result, "", "");
			strcat(result, " "); // need for correct &cmdline[strlen(result)];
			FD_write = fileno(stdout); //restore normal output
#ifdef verbose
			printf("%s is restoring standard output. FD: [%d]\n", pName, FD_write);
#endif
		}
	}
	else if(strext(cmdline, result, "", "|")){ // caso [cmd1 |]
		FD_read = fileno(stdin);
		if(pipe(pipeB) < 0){
			perror("call the plumber @ pipe() I");
			exit(EXIT_FAILURE);
		}
		FD_write = pipeB[1];
#ifdef verbose
		printf("%s opened OUTPUT pipe (II). FD: [%d]\n", pName, FD_write);
#endif

	}
	else{ // caso [cmd]
		FD_read = fileno(stdin);
		FD_write = fileno(stdout);
		strext(cmdline, result, "", "");
#ifdef verbose
		printf("%s is using normal pipes. FD_read: [%d]\tFD_write: [%d]\n", pName, FD_read, FD_write);
#endif
	}
#ifdef verbose
	printf("%s used execute(\"%s\",%d,%d)\n", pName, result, FD_read, FD_write);
#endif
	execute(result, FD_read, FD_write);
	char* next = &cmdline[strlen(result)];
	strfix(next, ' ');
	if(strcmp(next, "")) parse(next); // next != "" -> parse remaining line [| cmd2 | cmd3 | cmd4]
}

void switchPipes(){
#ifdef verbose
	printf("%s is switching pipes\n", pName);
#endif
	int tmp[2] = {pipeB[0], pipeB[1]};
	pipeB[0] = pipeA[0];
	pipeB[1] = pipeA[1];
	pipeA[0] = tmp[0];
	pipeA[1] = tmp[1];
}

void execute(char* instruction, int INPUT, int OUTPUT){
	procreate();
	if(!isChild()) return;
	char result[MAX_STRING];
	strext(instruction, result, "", " ");
	sprintf(pName, "son %d (%s)", getpid(), result);
	// file redirection
	checkFileRedirection(instruction);
	// process arguments
	char* arguments[MAX_ARGUMENTS];
	int arg = 1;
	bool hasArguments = strext(instruction, result, "", " ");
	if(!hasArguments) strcpy(result, instruction); // case [cmd]
	arguments[0] = calloc(strlen(result), sizeof(char)); // argument[0] is the program's name
	strcpy(arguments[0], result);
	if(hasArguments){ // case [cmd arg1 arg2 arg3 ...]
		instruction += strlen(result) + 1;
		// get middle arguments: [1;n-1]
		while(strext(instruction, result, "", " ")){
			instruction += strlen(result) + 1;
			arguments[arg] = calloc(strlen(result), sizeof(char));
			strcpy(arguments[arg], result);
			arg++;
		}
		// last argument ends with '\n'
		strext(instruction, result, "", "");
		arguments[arg] = calloc(strlen(result), sizeof(char));
		strcpy(arguments[arg], result);
		arg++;
	}
	else{
#ifdef verbose
		printf("%s hasArguments: %d\targ = %d\n", pName, hasArguments, arg);
#endif
	}
#ifdef verbose
	printf("%s hasArguments: %d\targ = %d\n", pName, hasArguments, arg);

	char output[MAX_STRING];
	sprintf(output, "%s used execv(%s,", pName, arguments[0]);
	for(int i = 0;i < arg;i++){
		strcat(output, arguments[i]);
		if(i < arg - 1) strcat(output, ",");
	}
	strcat(output, ");\n");
	printf("%s", output);
#endif
	arguments[arg] = NULL;
	if(SLOWMODE) sleep(1);
	if(!findFile(arguments[0], result)){
		printf(">baash - command not found: [%s]\n", arguments[0]);
		exit(EXIT_SUCCESS);
	}
#ifdef verbose
	printf("%s found file @ [%s]\n", pName, result);
#endif

	redirect(FD_read, FD_write);
	if(execv(result, arguments)){ // execv returns only if there is an error
		char error[MAX_STRING];
		sprintf(error, "process %d returned error %d when executing %s", getpid(), errno, result); // should never run
		perror(error);
		exit(EXIT_FAILURE);
	}
}

void checkFileRedirection(char* instruction){
	char result[MAX_STRING];
	char* pos;
	while((pos = strchr(instruction, STDERR_REDIRECT_CHAR)) != NULL){
		if(dup2(fileno(stdout), fileno(stderr)) < 0){
			perror("call the plumber @ stderr redirect");
			exit(EXIT_FAILURE);
		}
		printf("%s redirecting STDERR to STDIN\n", pName);
		pos[0] = ' ';
	}
	strfix(instruction, ' ');
	strcat(instruction, " ");
	strrep(instruction, "< ", "<");
	strrep(instruction, "> ", ">");
	if(strext(instruction, result, "<", " ")){
#ifdef verbose
		printf("%s redirecting INPUT to %s\n", pName, result);
#endif
		if((FD_read = open(result, O_RDONLY, 0444)) == -1){
			char error[MAX_STRING];
			sprintf(error, "%s cannot redirect INPUT to %s", pName, result);
			perror(error);
		}
		char tmp[MAX_STRING];
		sprintf(tmp, "<%s", result);
#ifdef verbose
		printf("%s is removing [%s] from [%s]\n", pName, tmp, instruction);
#endif
		strsub(instruction, tmp);
#ifdef verbose
		printf("%s has now instruction: [%s]\n", pName, instruction);
#endif
	}
	if(strext(instruction, result, ">", " ")){
#ifdef verbose
		printf("%s redirecting OUTPUT to %s\n", pName, result);
#endif
		//FD_write = open(result, O_CREAT | O_WRONLY, 0666); // overwrite
		FD_write = open(result, O_CREAT | O_WRONLY | O_APPEND, 0666); // append
		if(FD_write == -1){
			char error[MAX_STRING];
			sprintf(error, "%s cannot redirect OUTPUT to %s", pName, result);
			perror(error);
		}
		char tmp[MAX_STRING];
		sprintf(tmp, ">%s", result);
#ifdef verbose
		printf("%s is removing [%s] from [%s]\n", pName, tmp, instruction);
#endif
		strsub(instruction, tmp);
	}
	strfix(instruction, ' ');
#ifdef verbose
	printf("%s completed redirection. instruction: [%s]\n", pName, instruction);
#endif
}

void redirect(int FD_input, int FD_output){
#ifdef verbose
	printf("%s pipe state:\n\tFD_read = %d\tFD_write = %d\n\tpipeA = {%d,%d}\tpipeB = {%d,%d}\n", pName, FD_input, FD_output, pipeA[0], pipeA[1], pipeB[0], pipeB[1]);
#endif
	if(dup2(FD_input, fileno(stdin)) < 0 || dup2(FD_output, fileno(stdout)) < 0){
		perror("call the plumber @ redirect");
		sleep(1);
		exit(EXIT_FAILURE);
	}
}
