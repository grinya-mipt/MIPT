#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/sem.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <limits.h>

#define LOG_FILE "./log.txt"
#define IPC_FTOK_FILE "/bin/cat"
#define MAX_MSG_SIZE 256
#define NUM_START_INDEX 48

#define HELLO_MSG_ID 1
#define BYE_MSG_ID 2

struct client_msg{
    long mtype;
    int data[MAX_MSG_SIZE];
};

int* recieve_data(int msgid, int msgtype,int size,int row_count,int col_count);
void write_from_str_into_file(char* str, int fd);
void write_log(int semid, int fd_log, char* log_msg);
void print_err(char* dscrp);
size_t write_into_file(int fd,void* addr, const size_t size);
char* itoa(int number);

int semid;
int fd_log;

int main(int argc, char** argv, char** envp){
	pid_t pid = getpid();
//create or get message quewe
	key_t msg_key = ftok(IPC_FTOK_FILE, 1);
	if(msg_key < 0) print_err("generation of IPC key");
	int msgid = msgget(msg_key, 0666 | IPC_CREAT);
	if(msgid < 0) print_err("create or get message quewe");
        
//open or create log file
	fd_log = open(LOG_FILE, O_WRONLY | O_CREAT | O_APPEND, 0666);
	if(fd_log < 0) print_err("create or open log file");
    
//create or get semafors for logging
	key_t sem_key = ftok(IPC_FTOK_FILE, 2);
	if(sem_key < 0) print_err("generation IPC semaphore key");
	semid = semget(sem_key, 1, 0666 | IPC_CREAT | IPC_EXCL);
	if(semid < 0 && errno == EEXIST){
		if((semid = semget(sem_key, 1, 0)) < 0) print_err("getting semafors");
	}else if(semid >= 0){
		struct sembuf sem_oper;
		sem_oper.sem_num = 0;
		sem_oper.sem_flg = 0;
		sem_oper.sem_op = 1;
	        semop(semid, &sem_oper, 1);
                write_log(semid, fd_log, "Create semaphors for logging");
	}else print_err("create semaphors");
    
//send hello
	write_log(semid, fd_log, "Sending hello message to server");
	struct client_msg msg;
	msg.mtype = HELLO_MSG_ID;
	msg.data[0] = (int)pid;
	if(msgsnd(msgid, (struct msgbuf*) &msg, sizeof(int), 0) < 0) print_err("send hello message");

//recieve work_size and decide "to work or not to work"
	int work_size;
	if(msgrcv(msgid, (struct msgbuf*) &msg, sizeof(int), (int)pid, 0) < 0) print_err("get work_size message");
	if((work_size = msg.data[0]) == 0){
		write_log(semid,fd_log,"Nothing to work with");
	//send BYE messege
		msg.mtype = BYE_MSG_ID;
		msg.data[0] = (int)pid;
		if(msgsnd(msgid, (struct msgbuf*) &msg, sizeof(int), 0) < 0) print_err("send BYE to server");
		write_log(semid,fd_log,"Send BYE message to server");		
	}else{
		write_log(semid,fd_log,"Recieve size of work");
	//recieve information for calculating  
		if(msgrcv(msgid, (struct msgbuf*) &msg, 5*sizeof(int), (int)pid, 0) < 0) print_err("get information for calculating message");
		write_log(semid,fd_log,"Recieve additional information and format of data");		
		int size_of_mtx_str= msg.data[0];
		int row_count = msg.data[1];
		int col_count = msg.data[2];
	
	//recieve data for calculating    
		int* data = recieve_data(msgid, (int)pid, size_of_mtx_str, col_count, row_count);
		write_log(semid, fd_log, "Recieved data for calculating");	
			
	//calculating result from recieved data
		int* result;
		result = (int*)calloc(work_size,sizeof(int));
		int i,j,k;
		for(i=0;i<col_count;i++)
			for(j=0;j<row_count;j++)
				for(k=0;j<size_of_mtx_str;j++) result[i]+= *(data + j*size_of_mtx_str*sizeof(int) + k*sizeof(int))*(*(data + row_count*size_of_mtx_str*sizeof(int) + i*size_of_mtx_str*sizeof(int) + k*sizeof(int)));
		write_log(semid,fd_log,"Calculated the result");

	//send result
		int sent = 0, whole_work = work_size * size_of_mtx_str;
		int parts_number = whole_work / MAX_MSG_SIZE + 1;
		for(i = 0; i < parts_number; i++){
			int size = (i == (parts_number - 1)) ? whole_work - sent : MAX_MSG_SIZE;
			memcpy(msg.data, result + i*MAX_MSG_SIZE*sizeof(int), size*sizeof(int));
			if(msgsnd(msgid, (struct msgbuf*) &msg, MAX_MSG_SIZE, 0) < 0) print_err("send data to server");
	        	sent += size;
		}
		write_log(semid,fd_log,"Sent the result");
	//sent BYE messege
		msg.mtype = BYE_MSG_ID;
		msg.data[0] = (int)pid;
		if(msgsnd(msgid, (struct msgbuf*) &msg, sizeof(int), 0) < 0) print_err("send BYE to server");
		write_log(semid,fd_log,"Send BYE message to server");
	}
	
//delete semaphores
	if(msgrcv(msgid, (struct msgbuf*) &msg, sizeof(int), (int)pid, 0) < 0) print_err("recieve BYE message from server");
	write_log(semid,fd_log,"Recieve BYE message from server");
	if(msg.data[0] == 0){
		if(semctl(semid, 0, IPC_RMID, NULL) < 0) print_err("delete semafors");
		write_log(semid,fd_log,"Semaphors deleted");
	}
	return 0;
}

int* recieve_data(int msgid, int msgtype,int size,int row_count,int col_count){
	struct client_msg msg;
	msg.mtype = msgtype;
	int whole_work = (row_count + col_count) * size;
	int* data = (int*)malloc(whole_work * sizeof(int));
	int i, recieved = 0;
	int parts_number = whole_work / MAX_MSG_SIZE + 1;
	for(i = 0; i < parts_number; i++){
		int record_size = (i == (parts_number - 1)) ? whole_work - recieved : MAX_MSG_SIZE;
		if(msgrcv(msgid, (struct msgbuf*) &msg, MAX_MSG_SIZE, msgtype, 0) < 0) print_err("getting data from server");
		memcpy(data + i * MAX_MSG_SIZE, msg.data, record_size*sizeof(int));
		recieved += record_size;
	}
	return data;
}

void write_log(int semid, int fd_log, char* log_msg){
	int i;
	char* begin_1 = "[Client ";
	char* client_id = itoa(getpid());
	char* begin_2 = "][";    
//time_t seconds = time(NULL);
	struct timeval time;
	gettimeofday(&time, NULL);
	struct tm* date_time = localtime(&time.tv_sec);
	char* format = "%d.%m.%y %H:%M:%S.";
	int size = strlen(format) + 6;
	char* date = (char*)malloc((size + 1) * sizeof(char));
	strftime(date, size, format, date_time);
	char* usec = (char*)calloc(6,sizeof(char));
	char* tmp = itoa(time.tv_usec % (1000 * 1000));
	if(strlen(tmp) != 6){
		for(i=0;i<(6-strlen(tmp));i++) usec[i] = '0';
		usec[6-strlen(tmp)] = 0;		
	}
	usec = strcat(usec,tmp);
	free(tmp);
	char* end_1 = "] ";
	char* end_2 = "\n";
	char* log = (char*)malloc((strlen(log_msg) + strlen(begin_1) + strlen(client_id) + strlen(begin_2) + strlen(date) + strlen(usec) + strlen(end_1) + strlen(end_2)) * sizeof(char));
	log[0] = '\0';
    
	strcat(log, begin_1);
	strcat(log, client_id);
	strcat(log, begin_2);
	strcat(log, date);
	strcat(log, usec);
	strcat(log, end_1);
	strcat(log, log_msg);
	strcat(log, end_2);
    
	struct sembuf sem_oper;
	sem_oper.sem_num = 0;
	sem_oper.sem_flg = 0;
    
	sem_oper.sem_op = -1;
	if(semop(semid, &sem_oper, 1) < 0) print_err("decrease semafor");
	write_from_str_into_file(log, fd_log);
	sem_oper.sem_op = 1;
	if(semop(semid, &sem_oper, 1) < 0) print_err("increase semafor");
	free(log);
	free(date);
}

void write_from_str_into_file(char* str, int fd){
	int size = strlen(str);
	int tmp, count = 0;
	do{
		tmp = write_into_file(fd, str + count, size - count);
		count += tmp;
	} while(count < size);
}


char* itoa(int number){
	int minus;
	minus = (number < 0) ? 1 : 0;	
	int tmp = minus ? -number : number;    
//conut digits
	int number_of_digits = 1;
	while((tmp = tmp/10) > 0) number_of_digits++;
//allocate size for digits, minus and \0
	int start = 0;
	int size = number_of_digits + (minus ? 1 : 0) + 1;
	char* result = (char*)calloc(size, sizeof(char));
	if(minus){
		result[0] = '-';
		start = 1;
	}    
	int power = 1;
	tmp = minus ? -number : number;
	int i;
//write digits
	for(i = 0; i < number_of_digits; i++){
		int digit = (tmp - (tmp / (power * 10)) * power * 10) / power;
		result[number_of_digits + start -1 - i] = '0' + digit;
		power = power * 10;
	}
	result[size - 1] = '\0';
	return result;
}

void print_err(char* dscrp){
	printf("Error: %s\nErrno: %s\n", dscrp, strerror(errno));
	exit(-1);
}

size_t write_into_file(int fd,void* addr, const size_t size){
	size_t len;
	if((len = write(fd,addr,size)) < 0) print_err("write into file");
	return len;
}
