#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <limits.h>

void print_err(char* dscrp);

int main(int argc, char** argv, char** envp){
	if(argc != 2) print_err("Invalid number of arguments. Must be only one argument - number of clients");
	int n = atoi(argv[1]);
	if(n <= 0 || n == INT_MAX) print_err("invalid number format");

	int pid = fork();
	if(pid > 0){
		if(execl("./server", "./server", argv[1], NULL) < 0) print_err("start server");
	}else if(pid == 0){
        	int i;
		for(i = 0; i < n; i++){
			int client_pid = fork();
		        if(client_pid == 0) if(execl("./client", "./client", NULL) < 0) print_err("start client");
			else if(client_pid < 0) print_err("create client fork()");
        	}
	}else print_err("create main client fork()");
}

void print_err(char* dscrp){
	printf("Error: %s\nErrno = %s\n", dscrp, strerror(errno));
	exit(-1);
}
