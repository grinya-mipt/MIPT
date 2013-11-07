#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <sys/shm.h>
#include <stdlib.h>

#define BUFF 10
#define PATH "./shmgt1.c"

const int ps_info_min_lenght = 36; //how many symbols /bin/ps will show for unexistable process if PID < PID_LIMIT_MAX

struct string {
	int len;
	char* str;
	};

int print_into_stderr(const char str[]);

int read_string_from_pipe(int fd_0,struct string* res,int buff_size);

int main(int argc, char** argv)
{
//PART 1: checking if the input is correct

	//checking number of arguments
	if (argc != 2) {
		print_into_stderr("Must be only one argument - your PID of interest!\n");
		exit(-1);
		}

	//convert (char*)argument to int
	int pid = atoi(argv[1]);

	//cheking PID > 0
	if (pid < 1) {
		print_into_stderr("Invalid PID. It must be > 0\n");
		exit(-1);
		}
	
	//reading information about PID_LIMIT_MAX with fork() and pipe()
	int fdp[2];
	if (pipe(fdp) < 0) {
		print_into_stderr("Smth went wrong with creating a pipe\n");
		exit(-1);
		}
	int result = fork();
	
	if (result < 0) {
		print_into_stderr("Can't fork child\n");
		exit(-1);
		}
	if (result == 0) {
		// AP: а что делает dup2?
		dup2(fdp[1],1);
		if (close(fdp[0]) < 0) {
			print_into_stderr("Error in close()\n");
			exit(-1);
			}
		if (close(fdp[1]) < 0) {
			print_into_stderr("Error in close()\n");
			exit(-1);
			}
		execl("/bin/cat","/bin/cat","/proc/sys/kernel/pid_max",NULL);
		print_into_stderr("Failure with EXECL CAT\n");
		exit(-1);
		}

	struct string pid_max;
	pid_max.str = (char*)malloc(1*sizeof(char));
	pid_max.str[0] = '\0';
	pid_max.len = 1;
	
	if (close(fdp[1]) < 0) {
		print_into_stderr("Error in close()\n");
		exit(-1);
		}
	int read;
	read = read_string_from_pipe(fdp[0],&pid_max,BUFF);
	if (close(fdp[0]) < 0) {
		print_into_stderr("Error in close()\n");
		exit(-1);
		}
	
	//errors in read_string_from_pipe
	if (read == 0) {
		print_into_stderr("There is nothing to read in function read_string_from_pipe(). Probably, there is a failure with EXECL CAT\n");
		exit(-1);
		}
	if (read == -1) {
		print_into_stderr("Can't read the source in function read_string_from_pipe(). See line 72\n");
		exit(-1);
		}
	if (read == -2) {
		print_into_stderr("Can't do REALLOC in function read_string_from_pipe(). See line 72\n");
		exit(-1);
		}
	
	int pid_limit_max = atoi(pid_max.str);
	
//printf("%s",pid_max.str);
//printf("PID_LIMIT_MAX = %d\nUSER INPUT = %s\n(*INT) USER INPUT = %d\n",pid_limit_max,argv[1],pid);

	free(pid_max.str);	

	//checking if pid > PID_LIMIT_MAX
	if (pid > pid_limit_max) {
		print_into_stderr("Your PID is invalid. Your PID must be less then PID_LIMIT_MAX\n");
		printf("PID_LIMIT_MAX = %d\n",pid_limit_max);
		exit(-1);
		}
	//reading information about the process
	
	
	if (pipe(fdp) < 0) {
		print_into_stderr("Smth went wrong with creating a pipe\n");
		exit(-1);
		}

	result = fork();
	
	if (result < 0) {
		print_into_stderr("Can't fork child\n");
		exit(-1);
		}
	if (result == 0) {
		dup2(fdp[1],1);
		if (close(fdp[0]) < 0) {
			print_into_stderr("Error in close()\n");
			exit(-1);
			}
		if (close(fdp[1]) < 0) {
			print_into_stderr("Error in close()\n");
			exit(-1);
			}
		execl("/bin/ps","/bin/ps", argv[1], NULL);
		print_into_stderr("Failure with EXECL PS\n");
		exit(-1);
		}

	struct string ps_info;
	ps_info.str = (char*)malloc(1*sizeof(char));
	ps_info.str[0] = '\0';
	ps_info.len = 1;
	
	if (close(fdp[1]) < 0) {
		print_into_stderr("Error in close()\n");
		exit(-1);
		}
	read = read_string_from_pipe(fdp[0],&ps_info,BUFF);
	if (close(fdp[0]) < 0) {
		print_into_stderr("Error in close()\n");
		exit(-1);
		}

	//errors in read_string_from_pipe()
	if (read == 0) {
		print_into_stderr("There is nothing to read in function read_string_from_pipe(). Probably, there is a failure with EXECL PS\n");
		exit(-1);
		}
	if (read == -1) {
		print_into_stderr("Can't read the source in function read_string_from_pipe()\n");
		exit(-1);
		}
	if (read == -2) {
		print_into_stderr("Can't do REALLOC in function read_string_from_pipe()\n");
		exit(-1);
		}
//printf("%s%d\n",ps_info.str,ps_info.len);

	if (ps_info.len == ps_info_min_lenght) {
		print_into_stderr("There is no process with this PID\n");
		exit(-1);
		}
		
//PART 2: record to shared memory
	
	int shmid; //IPC descriptor for shared memory
	void *data; //Pointer to shared memory
	char pathname[] = PATH; //File for key generation
	key_t key; //IPC key

	if ((key = ftok(pathname,0)) < 0) {
		print_into_stderr("Can't generate IPC key\n");
		exit(-1);
		}

	//creating a shared memory for generated key
	if ((shmid = shmget(key, sizeof(int)+ps_info.len*sizeof(char),0666|IPC_CREAT|IPC_EXCL)) < 0) {
		if (errno != EEXIST) {
			print_into_stderr("Can't create shared memory\n");
			exit(-1);
			} 
		else {
			print_into_stderr("Shared memory already exists. Run second program to delete this IPC\n");
			exit(-1);
			}		
		}
	
	//attaching the shared memory to the address space of the process
	if ( (data = (void *)shmat(shmid,NULL,0)) == (void *)-1){
		print_into_stderr("Can't attach shared memory\n");
		exit(-1);
		}
	
	//recording info about process into shared memory
	*(int *)data = ps_info.len;
//printf("%d\n", *(int *)data);
	char* str = (char*)data + sizeof(int);
	int i;
	// AP: посмотрите функцию strcpy и перепишите
	for(i=0;i<ps_info.len;i++) *(str+i)=ps_info.str[i];

//printf("%s", str);

	if (shmdt(data) < 0) {
		print_into_stderr("Can't detach shared memory\n");
		exit(-1);
		}

	return 0;
}

// AP: а почему бы не включить сюда exit если он и так у вас всегда после этой функции идет
int print_into_stderr(const char str[])
{
	int lenght = strlen(str);
	if (write(2,str,lenght) < 0){
		printf("Smth goes wrong in sys call write() in function print_into_stderr()\n");
		return -1;		
		}
	return lenght;
}

int read_string_from_pipe(int fd_0,struct string* res,int buff_size)
{
	size_t size;
	char* data_tmp;
	char *buffer;
	buffer = (char*)malloc(buff_size*sizeof(char));
	int i;
	int flag = 1;
	res->len = 1;
	do 
		{
		if((size = read(fd_0,buffer,buff_size))<0)
			{
			//Can't read the source file
			free(buffer);
			return -1;
			}
		else if(size != 0)
			{
			res->len+=size;
			data_tmp = (char*)realloc(res->str,res->len*sizeof(char));
			if (data_tmp!=NULL) res->str = data_tmp;
			else	{
				//Can't do REALLOC
				free(buffer);
				return -2;
				}
			if(size==buff_size) strcat(res->str,buffer);
			// AP: посмотрите такую функцию как strncat и перепишите
			else for(i=0;i<size;i++) {
				res->str[res->len-size-1+i]=buffer[i];
				res->str[res->len-1] ='\0';
				}
			}
		else flag=0;
		} 
	while(flag==1);	
	free(buffer);
	return res->len;
}
