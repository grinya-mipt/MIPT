// AP: нужно использовать только одну очередь
// DG: исправлено

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

//difines below cannot be changed without server's ones!
#define IPC_FTOK_FILE "/bin/cat"
#define MAX_MSG_SIZE 256 //must satisfy maximum of Syspem V IPC message
#define HELLO_MSG_ID 1
#define BYE_MSG_ID 2
#define SEND 1
#define RECIEVE 2

struct client_msg{
	long mtype;
	int data[MAX_MSG_SIZE];
};

struct client_msg_info{
	long mtype;
	int data[5];
};

struct client_msg_one{
	long mtype;
	int data;
};

int* recieve_data(int msgid, int msgtype,int size,int row_count,int col_count);
void write_from_str_into_file(char* str, int fd);
void write_log(int semid, int fd_log, char* log_msg);
void print_err(char* dscrp);
size_t write_into_file(int fd,void* addr, const size_t size);
char* itoa(int number);
long int chg_msg_type(long int arg, const int msg_type);

int semid;
int fd_log;

int main(int argc, char** argv, char** envp){
//set up rigth message types
	long int send_type = chg_msg_type(getpid(), SEND);
	long int recieve_type = chg_msg_type(getpid(), RECIEVE);
	
//create or get message quewe
	key_t msg_key = ftok(IPC_FTOK_FILE, 1);
	if(msg_key < 0) print_err("generation of IPC key");
	int msgid = msgget(msg_key, 0666 | IPC_CREAT);
	if(msgid < 0) print_err("create or get message quewe for sendind");
        
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
                write_log(semid, fd_log, "Created semaphors for logging");
	}else print_err("create semaphors");
    
//send hello
	struct client_msg msg_hello;
	msg_hello.mtype = HELLO_MSG_ID;
	msg_hello.data[0] = getpid();
	if(msgsnd(msgid, (void*) &msg_hello, sizeof(int), 0) < 0) print_err("send hello message");
	write_log(semid, fd_log, "Sent hello message to server");

//recieve work_size and decide "to work or not to work"
	struct client_msg_one msg_work_size, msg_bye;
	msg_work_size.mtype = recieve_type;
	if(msgrcv(msgid, (void*) &msg_work_size, sizeof(int), recieve_type, 0) < 0) print_err("get work_size message");
	if(msg_work_size.data == 0){
		write_log(semid,fd_log,"Nothing to work with");
	//send END_WORK messege
		msg_bye.mtype = BYE_MSG_ID;
		msg_bye.data = getpid();
		if(msgsnd(msgid, (void*) &msg_bye, sizeof(int), 0) < 0) print_err("send END_WORK to server");
		write_log(semid,fd_log,"Sent END_WORK message to server");		
	}else{
		int work_size = msg_work_size.data;
		write_log(semid,fd_log,"Recieved size of work");
	//recieve information for calculating
		struct client_msg_info msg_info;
		msg_info.mtype = recieve_type;
		if(msgrcv(msgid, (void*) &msg_info, 5*sizeof(int), recieve_type, 0) < 0) print_err("get additional information");
		write_log(semid,fd_log,"Recieved additional information for calculation of data");		
		int size_of_mtx_str= msg_info.data[0];
		int row_total = msg_info.data[1];
		int row_count = msg_info.data[2];
		int col_count = msg_info.data[3];
		int i_start_index = msg_info.data[4];
	
	//recieve data for calculating    
		int* data = recieve_data(msgid, recieve_type, size_of_mtx_str, col_count, row_count);
		write_log(semid, fd_log, "Recieved data for calculating");	
			
	//calculating result from recieved data
		int* result;
		result = (int*)calloc(work_size,sizeof(int));
		int i, j = 0, k = size_of_mtx_str, n = 0, disp = i_start_index - (row_total - row_count)*size_of_mtx_str;
		while(n < work_size){
			for(k=0;k<size_of_mtx_str;k++) result[n]+=(*(data + disp*size_of_mtx_str + k))*(*(data + (j+row_count)*size_of_mtx_str + k));
			n++; disp++;
			if(disp>=row_total){ 
				j++;
				disp = 0;
			}
		}
		write_log(semid,fd_log,"Calculated the result");

	//send result
		struct client_msg msg_result;
		msg_result.mtype = send_type;
		int sent = 0;
		int parts_number = work_size / MAX_MSG_SIZE + 1;
		for(i = 0; i < parts_number; i++){
			int size = (i == (parts_number - 1)) ? work_size - sent : MAX_MSG_SIZE;
			memcpy(msg_result.data,result + i*MAX_MSG_SIZE, size*sizeof(int));
			if(msgsnd(msgid, (void*) &msg_result, size*sizeof(int), 0) < 0) print_err("send data to server");
	        	sent += size;
		}
		write_log(semid,fd_log,"Sent the result");
		free(result);
		free(data);
	//sent END_WORK messege
		msg_bye.mtype = BYE_MSG_ID;
		msg_bye.data = getpid();
		if(msgsnd(msgid, (void*) &msg_bye, sizeof(int), 0) < 0) print_err("send END_WORK to server");
		write_log(semid,fd_log,"Sent END_WORK message to server");
	}
	
//delete semaphores
	struct client_msg msg_sem;
	msg_sem.mtype = getpid();
	write_log(semid,fd_log,"Ready for recieve BYE message from server and end work");
	if(msgrcv(msgid, (void*) &msg_sem, sizeof(int), getpid(), 0) < 0) print_err("recieve BYE message from server");	
	if(msg_sem.data[0] == 0){
		write_log(semid,fd_log,"Semaphors for log is going to be deleted");
		write_log(semid,fd_log,"Shutting down");
		msg_bye.mtype = getpid();
		if(msgsnd(msgid, (void*) &msg_bye, 0, 0) < 0) print_err("send confirmation to server");
		if(semctl(semid, 0, IPC_RMID, NULL) < 0) print_err("delete semafors");		
	}
	return 0;
}

int* recieve_data(int msgid, int msgtype, int size,int row_count,int col_count){
	struct client_msg msg;
	msg.mtype = msgtype;
	int whole_work = (row_count + col_count) * size;
	int* data = (int*)malloc(whole_work * sizeof(int));
	int i, recieved = 0;
	int parts_number = whole_work / MAX_MSG_SIZE + 1;
	for(i = 0; i < parts_number; i++){
		int record_size = (i == (parts_number - 1)) ? whole_work - recieved : MAX_MSG_SIZE;
		if(msgrcv(msgid, (void*) &msg, record_size*sizeof(int), msgtype, 0) < 0) print_err("get data from server");
		memcpy(data + i * MAX_MSG_SIZE, msg.data, record_size*sizeof(int));
		recieved += record_size;
	}
	return data;
}

void write_log(int semid, int fd_log, char* log_msg){
	int i;
	char* begin_1 = "[Client ";
	char* client_id = itoa((int)getpid());
	char* begin_2 = "][";    
	struct timeval time;
	gettimeofday(&time, NULL);
	struct tm* date_time = localtime(&time.tv_sec);
	char* format = "%d.%m.%y %H:%M:%S.";
	int size = strlen(format) + 6;
	char* date = (char*)malloc((size + 1) * sizeof(char));
	strftime(date, size, format, date_time);
	char* usec = (char*)calloc(7,sizeof(char));
	usec[0] = 0;
	int micsec = time.tv_usec % (1000 * 1000);
	char* tmp = itoa(micsec);
	if(strlen(tmp) != 6){
		for(i=0;i<(6-strlen(tmp));i++) usec[i] = '0';
		usec[6-strlen(tmp)] = 0;		
	}
	usec = strcat(usec,tmp);
	free(tmp);
	char* end_1 = "] ";
	char* end_2 = "\n";
	char* log = (char*)malloc((strlen(log_msg) + strlen(begin_1) + strlen(begin_2) + strlen(date) + strlen(usec) + strlen(end_1) + strlen(client_id) + strlen(end_2) + 1) * sizeof(char));
	log[0] = 0;
    
	strcat(log, begin_1);
	strcat(log, client_id);
	strcat(log, begin_2);
	strcat(log, date);
	strcat(log, usec);
	strcat(log, end_1);
	strcat(log, log_msg);
	strcat(log, end_2);
	
	free(client_id);	
	free(date);
	free(usec);
    
	struct sembuf sem_oper;
	sem_oper.sem_num = 0;
	sem_oper.sem_flg = 0;
    
	sem_oper.sem_op = -1;
	if(semop(semid, &sem_oper, 1) < 0) print_err("decrease semafor");
	write_from_str_into_file(log, fd_log);
	sem_oper.sem_op = 1;
	if(semop(semid, &sem_oper, 1) < 0) print_err("increase semafor");
	free(log);
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
	char* out = (char*)calloc(size,sizeof(char));
	if(minus){
		out[0] = '-';
		start = 1;
	}    
	int power = 1;
	tmp = minus ? -number : number;
	int i;
//write digits
	for(i = 0; i < number_of_digits; i++){
		int digit = (tmp - (tmp / (power * 10)) * power * 10) / power;
		out[number_of_digits + start -1 - i] = '0' + digit;
		power = power * 10;
	}
	out[size - 1] = 0;
	return out;
}

void print_err(char* dscrp){
	printf("[CLIENT %d]ERROR: %s. ERRNO: %s\n",getpid(), dscrp, strerror(errno));
	exit(-1);
}

size_t write_into_file(int fd,void* addr, const size_t size){
	size_t len;
	if((len = write(fd,addr,size)) < 0) print_err("write into file");
	return len;
}

long int chg_msg_type(long int arg, const int msg_type){
	long int* cal = (long int*)calloc(1,sizeof(long int));
	*cal = arg;
	*((char*)((char*)cal+3*sizeof(char))) = (const char)msg_type;
	long int result = *cal;
	free(cal);
	return result;	
}
