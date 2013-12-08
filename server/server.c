//Замечания
//1. выделение лишней памяти в нитях, (из-за использования структур) для которых work_size == 0
//2. делать везде (?) free()
//3. при логировании в клиенте сделать аккуратно вывод микросекунд

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <pthread.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <limits.h>

#define IPC_FTOK_FILE "/bin/cat"
#define DATA "./data.txt"
#define RES "./result_matrix.txt."
#define MAX_MSG_SIZE 256
#define HELLO_MSG_ID 1
#define BYE_MSG_ID 2
#define INT_DEC_LEN 25

struct thread_data{
	int mtx_sizes[3];
	int** mtx1;
	int** mtx2;
	int** res;
	int i_start_index;
	int j_start_index;
	int row_count;
	int col_count;
	int work_size;    
	int msgid;
};

struct client_msg{
	long mtype;
	int data[MAX_MSG_SIZE];
};

void* client(void* arg);

size_t read_int_from_file(void* addr, int fd);
size_t write_into_file(int fd,void* addr, const size_t size);
void read_matrixes(int* sizes, int** matx1, int** matx2,int fd);

void print_err(char* dscrp);
void print_err_thread(char* dscrp,pthread_t id);
char* itoa(int number);

int main(int argc, char** argv, char** envp){
	(void)umask(0);
//checking number of arguments and converting it (char*) to (int)
	if(argc != 2){
		printf("Invalid argument. Number of clients = %s\n",argv[1]);
		exit(-1);
	}
 	int n = atoi(argv[1]);
	if(n <= 0 || n == INT_MAX) print_err("invalid number of clients");

//create or get message quewe
	key_t msg_key = ftok(IPC_FTOK_FILE, 1);
	if(msg_key < 0) print_err("generation of IPC key");
	int msgid = msgget(msg_key, 0666 | IPC_CREAT);
	if(msgid < 0) print_err("create or get message quewe");

	struct msginfo msg_data;
	if(msgctl(msgid, MSG_INFO, (struct msqid_ds*)&msg_data) < 0) print_err("get message quewe limits data");

//Messege quewe info for debagging
	printf("Max message lenght = %d\nMax quewe lenght = %d\nMax number of quewes = %d\n", msg_data.msgmax, msg_data.msgmnb, msg_data.msgmni);
  
//open data file (format - from "generate" program)
	int fd;
	if((fd = open(DATA, O_RDONLY, 0666)) < 0) print_err("open data file");

//read matrixes from file
	int **mtx1,**mtx2;
	int *mtx_sizes;
	mtx_sizes = (int*)malloc(3*sizeof(int));
	read_matrixes(mtx_sizes, mtx1, mtx2, fd);
	if(close(fd) < 0) print_err("close data file");

//create supporting variables
	int i,j,tmp;

//debuging

printf("%d %d %d\n",mtx_sizes[0],mtx_sizes[1],mtx_sizes[2]);
for(i=0;i<mtx_sizes[0];i++) {
for(j=0;j<mtx_sizes[1];j++) printf("%d ",mtx1[i][j]);
printf("\n");
}
printf("\n");
for(i=0;i<mtx_sizes[2];i++) {
for(j=0;j<mtx_sizes[1];j++) printf("%d ",mtx2[i][j]);
printf("\n");
}

//create result matrix
	int** res = (int**)malloc(mtx_sizes[0]*sizeof(int*));
	for(i=0;i<mtx_sizes[0];i++) res[i] = (int*)calloc(mtx_sizes[2],sizeof(int));

//defining work for different threads
	struct thread_data* thread;
	thread = (struct thread_data*)malloc((n+1)*sizeof(struct thread_data));
	int total_size_of_work = mtx_sizes[0]*mtx_sizes[2];
	if(total_size_of_work > n){
	//setting up numbers of elements for calculation for each thread (case n < total_size_of_work)
		int initial_work = (mtx_sizes[0]*mtx_sizes[2])/n;
		int rest_work = (mtx_sizes[0]*mtx_sizes[2])%n;		
		for(i=0;i<n;i++) thread[i].work_size = initial_work;
		for(i=0;rest_work>0;rest_work--) ++thread[i++].work_size;
	//setting up coordinates of start point for each thread (case n < total_size_of_work)
		thread[0].i_start_index = thread[0].j_start_index = 0;
		for(i=1;i<n;i++) {
			thread[i].i_start_index = thread[i-1].i_start_index + thread[i-1].work_size/mtx_sizes[0];
			if( (thread[i].j_start_index = thread[i-1].j_start_index + thread[i-1].work_size%mtx_sizes[0]) >= mtx_sizes[0]){
				thread[i].i_start_index++;
				thread[i].j_start_index = thread[i-1].work_size%mtx_sizes[0] - mtx_sizes[0] + thread[i-1].j_start_index;
			}
		}
	//calculating usage of colomns and rows for each thread
		thread[0].row_count = thread[0].col_count = 1;
		thread[n].i_start_index = 0;
		thread[n].j_start_index = mtx_sizes[2]; //there is a trick here: we consider imaginary element with (i,j) = (0,mtx_size[2]) for short code
		for(i=1;i<=n;i++){
			if((tmp = thread[i].j_start_index - thread[i-1].j_start_index) == 0){
				thread[i-1].col_count = 1;
				thread[i-1].row_count = thread[i].i_start_index - thread[i-1].i_start_index;
			} else {			
				thread[i-1].col_count = (thread[i].i_start_index == 0) ? tmp : tmp+1;
				if(tmp > 1) thread[i-1].row_count = mtx_sizes[0];
				else thread[i-1].row_count = ((tmp = thread[i].i_start_index - thread[i-1].i_start_index) >= 0) ? mtx_sizes[0] : mtx_sizes[0] + tmp;
			}
		}
	} else {
	//setting up numbers of elements for calculation and respective coordinates for each thread (case n >= total_size_of_work)
		thread[0].i_start_index = thread[0].j_start_index = 0;		
		for(i=1;i<total_size_of_work;i++){
			thread[i].work_size = 1;
			thread[i].col_count = 1;
			thread[i].row_count = 1;
			thread[i].res = res;
			thread[i].i_start_index = (thread[i-1].i_start_index == (mtx_sizes[0] - 1) ) ? 0 : thread[i-1].i_start_index + 1;
			thread[i].j_start_index = (thread[i].i_start_index == 0) ? thread[i-1].j_start_index + 1 : thread[i-1].j_start_index;
		}
		for(i=total_size_of_work;i<n;i++) thread[i].work_size = 0;	
	}

//start timer
	struct timeval start_time;
	gettimeofday(&start_time, NULL);

//create and start threads for clients
	pthread_t *thd_id;
	thd_id = (pthread_t*)malloc(n*sizeof(pthread_t));
	for(i = 0; i < n; i++){
		thread[i].msgid = msgid;
		thread[i].mtx1 = mtx1;
		thread[i].mtx2 = mtx2;
		thread[i].mtx_sizes[0] = mtx_sizes[0];
		thread[i].mtx_sizes[1] = mtx_sizes[1];
		thread[i].mtx_sizes[2] = mtx_sizes[2];		
		if(pthread_create(&thd_id[i], NULL, client, &thread[i]) > 0) print_err("create thread");
	}

//waiting for threads
	for(i = 0;i < n;i++) pthread_join(thd_id[i], (void**)NULL);

//delete message quewe
	if(msgctl(msgid, IPC_RMID, NULL) < 0) print_err("delete message quewe");

//print time of calculation
	struct timeval end_time;
	gettimeofday(&end_time,NULL);
	int delta = end_time.tv_usec - start_time.tv_usec;
	int sec = delta / (1000 * 1000);
	int msec = (delta - sec * 1000 * 1000) / 1000;
	int usec = delta - sec * 1000 * 1000 - msec * 1000;
	printf("Working time: %ds %dms %dmcs\n", sec, msec, usec);

//synchronization of clients' shutting down
	struct client_msg msg;
	for(i=0;i<n;i++){
		if(msgrcv(msgid, (struct msgbuf*) &msg, sizeof(int),BYE_MSG_ID,0)<0) print_err("recieve BYE from client message");
		msg.mtype = msg.data[0];
		msg.data[0] = (i == n-1) ? 0 : 1;
		if(msgsnd(msgid, (struct msgbuf*) &msg, sizeof(int), 0) < 0) print_err("send BYE from server messege");	
	}

//write integer matrix into RES
	if ((fd = open(RES, O_WRONLY | O_CREAT | O_TRUNC, 0666)) < 0) print_err("open result file");
	char* buffer;
	char space = ' ', end = '\n';
	for(i = 0;i < mtx_sizes[0];i++) {
		for(j = 0;j < mtx_sizes[2];j++) {
			buffer = itoa(res[i][j]);
			write_into_file(fd,buffer,strlen(buffer));
			write_into_file(fd,&space,1);	
		}
		write_into_file(fd,&end,1);
	}
	free(buffer);	
	if(close(fd)) print_err("close result file");
	return 0;
}

void read_matrixes(int* sizes, int** matx1, int** matx2,int fd){
	read_int_from_file(&sizes[0],fd);
	read_int_from_file(&sizes[1],fd);
	read_int_from_file(&sizes[2],fd);	
	if((matx1 = (int**)malloc(sizes[0]*sizeof(int*)))==(int**)NULL) print_err("malloc err");
	if((matx2 = (int**)malloc(sizes[2]*sizeof(int*)))==(int**)NULL) print_err("malloc err");	
	int i,j;
	for(i=0;i<sizes[0];i++) if((matx1[i] = (int*)calloc(sizes[1],sizeof(int)))==(int*)NULL) print_err("calloc err");
	for(i=0;i<sizes[0];i++) for(j=0;j<sizes[1];j++) read_int_from_file(&matx1[i][j],fd);
//second matrix is transposed
	for(i=0;i<sizes[2];i++) if((matx2[i] = (int*)calloc(sizes[1],sizeof(int)))==(int*)NULL) print_err("calloc err");
	for(i=0;i<sizes[1];i++) for(j=0;j<sizes[2];j++) read_int_from_file(&matx2[j][i],fd);
}

void* client(void* arg){
	pthread_t mythid = pthread_self();
	struct thread_data* data = (struct thread_data*)arg;    
	struct client_msg msg_initial;
	if(msgrcv(data->msgid, (struct msgbuf*) &msg_initial, sizeof(int), HELLO_MSG_ID, 0) < 0) print_err_thread("get hello message",mythid);

//send "to work or not to work"
	msg_initial.mtype = msg_initial.data[0];
	msg_initial.data[0] = data->work_size;
	if(msgsnd(data->msgid, (struct msgbuf*) &msg_initial, sizeof(int), 0) < 0) print_err_thread("send 'to work or not to work'",mythid);

	if(data->work_size == 0) return NULL; 

//send other initial information
	msg_initial.data[0] = data->mtx_sizes[1];
	msg_initial.data[1] = data->row_count;
	msg_initial.data[2] = data->col_count;
	if(msgsnd(data->msgid, (struct msgbuf*) &msg_initial, 4 * sizeof(int), 0) < 0) print_err_thread("send initial information for calculating",mythid);

	struct client_msg msg_data;
	msg_data.mtype = msg_initial.mtype;
	int i, r, k, flag = 0, sent = 0, whole_work = (data->col_count + data->row_count) * data->mtx_sizes[1];
	int parts_number = whole_work / MAX_MSG_SIZE + 1;
	int start_1 = data->i_start_index * data->mtx_sizes[1]*sizeof(int), start_2 = data->j_start_index * data->mtx_sizes[1]*sizeof(int);
	for(i = 0; i < parts_number; i++){
		int size = (i == (parts_number - 1)) ? whole_work - sent : MAX_MSG_SIZE;
		if(flag == 0){	
			if((r = data->row_count * data->mtx_sizes[1] - sent) < size){
				memcpy(msg_data.data, data->mtx1 + start_1 +i*MAX_MSG_SIZE*sizeof(int), r*sizeof(int));
				memcpy(msg_data.data, data->mtx2 + start_2, (size-r)*sizeof(int));
				k = i;
				flag = 1;	
			} else  memcpy(msg_data.data, data->mtx1 + start_1 +i*MAX_MSG_SIZE*sizeof(int), size*sizeof(int));
		} else memcpy(msg_data.data, data->mtx2 + start_2 +(i-k)*MAX_MSG_SIZE*sizeof(int), size*sizeof(int));
		if(msgsnd(data->msgid, (struct msgbuf*) &msg_data, MAX_MSG_SIZE, 0) < 0) print_err_thread("send data to client",mythid);
	        sent += size;
	}

//recieve the result
	int* result = (int*)malloc(data->work_size * sizeof(int));
	int recieved = 0;
	parts_number = data->work_size / MAX_MSG_SIZE + 1;
	for(i = 0; i < parts_number; i++){
		int record_size = (i == (parts_number - 1)) ? data->work_size - recieved : MAX_MSG_SIZE;
		if(msgrcv(data->msgid, (struct msgbuf*) &msg_data, MAX_MSG_SIZE, msg_initial.mtype, 0) < 0) print_err_thread("recieveing data from client",mythid);
		memcpy(result + i * MAX_MSG_SIZE, msg_data.data, record_size*sizeof(int));
		recieved += record_size;
	}
//recording to RES matrix
	i = data->i_start_index;
	int j = data->j_start_index, rest_work = data->work_size;
	while(rest_work > 0) {
		data->res[i][j] = result[data->work_size-rest_work];
		rest_work--;
		if(i == data->mtx_sizes[0]-1) {
			i = 0;
			j++;
		} else i++;		
	}
	free(result);
	return NULL;
}

void print_err(char* dscrp){
	printf("Error: %s\nErrno: %s\n", dscrp, strerror(errno));
	exit(-1);
}

void print_err_thread(char* dscrp,pthread_t id){
	printf("Error in thread %d: %s\nErrno: %s\n",(int)id, dscrp, strerror(errno));
	exit(-1);
}

size_t read_int_from_file(void* addr,int fd){
	size_t size;
	if((size = read(fd,addr,sizeof(int))) < 0) print_err("read from file");
	return size;
}

size_t write_into_file(int fd,void* addr, const size_t size){
	size_t len;
	if((len = write(fd,addr,size)) < 0) print_err("write into file");
	return len;
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
