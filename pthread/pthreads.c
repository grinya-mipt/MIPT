#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>

#define FILE "./mat.txt"
#define RES "./result.txt"

struct arg {
	int x;
	int y;
	int len;
};

int FD;
int **mtx1, **mtx2,**res;
int a,b,c;
int i,j;
char buffer[33];
char sh = ' ', pr = '\n';
const size_t INT_SIZE = 4;

void err_exit(const char str[]);
size_t nread(void* addr);
size_t nwrite(int fd,void* addr, const size_t size);
void* mythread(void* thread);
int itoa(int data, char* dest);

int main(int argc, char** argv)
{
time_t start = time(NULL);
//checking number of arguments
	if (argc != 2) err_exit("Must be only 1 argument - number of threads!\n");

	int m = atoi(argv[1]);
	
//reading file
	(void*)umask(0);
	if ((FD = open(FILE, O_RDONLY)) < 0) err_exit("Error in open()\n");
	
	nread(&a);
	nread(&b);
	nread(&c);
	
	mtx1 = (int**)malloc(a*sizeof(int*));
	mtx2 = (int**)malloc(b*sizeof(int*));
	
	for(i=0;i<a;i++) {
		mtx1[i] = (int*)malloc(b*sizeof(int));
		for(j=0;j<b;j++) nread(&mtx1[i][j]);
	}
	for(i=0;i<b;i++) {
		mtx2[i] = (int*)malloc(c*sizeof(int));
		for(j=0;j<c;j++) nread(&mtx2[i][j]);
	}

	close(FD);

/*
printf("%d %d %d\n",a,b,c);
printf("\n");
for(i=0;i<a;i++) {
	for(j=0;j<b;j++) printf("%d ",mtx1[i][j]);
	printf("\n");
}
printf("\n");
for(i=0;i<b;i++) {
	for(j=0;j<c;j++) printf("%d ",mtx2[i][j]);
	printf("\n");
}
*/

//arranging memory for result
	res = (int**)malloc(a*sizeof(int*));
	for(i=0;i<a;i++) res[i] = (int*)malloc(c*sizeof(int));

//setting up numbers of elements for calculation for each thread
	int num = (a*c)/m;
	int r = (a*c)%m;

	struct arg* thread;
	thread = (struct arg*)malloc(m*sizeof(struct arg));
	for(i=0;i<m;i++) thread[i].len = num;
	for(i=0;r>0;r--) ++thread[i++].len;


/*
for(i=0;i<m;i++) printf("%d ",thread[i].len);
printf("\n");	
*/

//setting up coordinates of start point for each thread
	thread[0].x = thread[0].y = 0;
	for(i=1;i<m;i++) {
	if(thread[i].len==0) i = m;
	else {
		thread[i].x = thread[i-1].x + thread[i-1].len/a;
		if( (thread[i].y = thread[i-1].y + thread[i-1].len%a) >= a) {
			thread[i].x++;
			thread[i].y = thread[i-1].len%a - a + thread[i-1].y;
			}
		}
	}

/*
printf("\n");
for(i=0;i<m;i++) printf("%d %d\n",thread[i].y,thread[i].x);	
*/

//work in threads
	pthread_t *thid, mythid;
	thid = (pthread_t*)malloc(m*sizeof(pthread_t));
	int err_res;
	for(i=1;i<m;i++) {
		if(thread[i].len != 0) {
			err_res = pthread_create(&thid[i], (pthread_attr_t *)NULL, mythread, &thread[i]);
			if(err_res != 0) {
				printf("Error in pthread_create() [thread number = %d], returned value = %d\n",i,err_res);
				exit(-1);
			}
		} else i = m;
	}

//work in main thread
	mythread(&thread[0]);

//waiting for other threads
	for(i=0;i<m;i++) pthread_join(thid[i], (void**)NULL);

	printf("Calculating time = %d\n",(int)(time(NULL) - start));

/*
printf("\n");
for(i=0;i<a;i++) {
	for(j=0;j<c;j++) printf("%d ",res[i][j]);
	printf("\n");
}	
*/
	if ((FD = open(RES, O_WRONLY | O_CREAT | O_TRUNC, 0666)) < 0) err_exit("Error in open()\n");
	int size;
	for(i=0;i<a;i++) {
		for(j=0;j<c;j++) {
			size = itoa(res[i][j], buffer);
//printf("%s ",buffer);
			nwrite(FD,buffer,size);
			nwrite(FD,&sh,1);	
		}
		nwrite(FD,&pr,1);
	}	
	close(FD);
	printf("Whole working time = %d\n",(int)(time(NULL) - start));
	return 0;
}

void* mythread(void* thread)
{
	pthread_t mythid;
	mythid = pthread_self();
	int k1, i1 = ((struct arg*)thread)->y, j1 = ((struct arg*)thread)->x, n1 = ((struct arg*)thread)->len;
	while(n1 > 0) {
		res[i1][j1] = 0;
		for(k1=0;k1<b;k1++) res[i1][j1]+= mtx1[i1][k1]*mtx2[k1][j1];
		n1--;
		if(i1 == a-1) {
			i1 = 0;
			j1++;
		} else i1++;		
	}
	return NULL;
}

void err_exit(const char str[])
{
	int lenght = strlen(str);
	if (write(2,str,lenght) < 0){
		printf("Smth goes wrong in sys call write() in function err_exit()\n");
		exit(-1);		
		}
	exit(-1);
}

size_t nread(void* addr)
{
	size_t sz;
	if((sz = read(FD,addr,INT_SIZE)) < 0) err_exit("Error in read()\n");
	return sz;
}

size_t nwrite(int fd,void* addr, const size_t size)
{
	size_t sz;
	if((sz = write(fd,addr,size)) < 0) err_exit("Error in write()\n");
	return sz;
}

int itoa(int data, char* dest)
{
	char buffer0 [33];
	char buffer1 [33];
	int len = 1;
	int flag = 1;
	int tmp = data, t = tmp%10;
	buffer0[0] = '0'+(char)t;
	buffer0[1] = '\0';
	tmp = (tmp - t)/10;
	while(tmp > 0) {
		t = tmp%10;
		if(flag) {			
			buffer1[0] = '0'+(char)t;
			buffer1[1] = '\0';
			strcat(buffer1,buffer0);
			flag--;
			len++;
		} else {
			buffer0[0] = '0'+(char)t;
			buffer0[1] = '\0';
			strcat(buffer0,buffer1);
			flag++;
			len++;
		}
		tmp = (tmp - t)/10;
	}
	if(flag) strcpy(dest,buffer0); 
	else strcpy(dest,buffer1);
	return len;
}
