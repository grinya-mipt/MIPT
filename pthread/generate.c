#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>


#define FILE "./data.txt"
#define SCALE 2

char buffer;
const size_t INT_SIZE = 4;

void print_err(char* dscrp);
size_t write_int(int fd,void* addr);

int main(int argc, char** argv)
{
//checking number of arguments
	if (argc != 4) print_err("invalid number of arguments. Must be only 3 arguments (for number of rows and colomns in each matrix) a,b,c");

//convert to int
	int a,b,c;
	a = atoi(argv[1]);
	b = atoi(argv[2]);
	c = atoi(argv[3]);

//arranging memory
	int** mtx1, **mtx2;
	mtx1 = (int**)malloc(a*sizeof(int*));
	mtx2 = (int**)malloc(b*sizeof(int*));

//generating randomly first matrix
	srand(time(NULL));
	int i,j;
	for(i=0;i<a;i++) {
		mtx1[i] = (int*)malloc(b*sizeof(int));
		for(j=0;j<b;j++) mtx1[i][j] = rand()%SCALE;
	}	
//generating randomly second matrix
	for(i=0;i<b;i++) {
		mtx2[i] = (int*)malloc(c*sizeof(int));
		for(j=0;j<c;j++) mtx2[i][j] = rand()%SCALE;
	}

//write into file
	int fd;
	(void)umask(0);
	if ((fd = open(FILE,O_WRONLY | O_CREAT | O_TRUNC, 0666)) < 0) print_err("open the generating file");
	write_int(fd,&a);
	write_int(fd,&b);
	write_int(fd,&c);
	for(i=0;i<a;i++) for(j=0;j<b;j++) write_int(fd,&mtx1[i][j]);
	for(i=0;i<b;i++) for(j=0;j<c;j++) write_int(fd,&mtx2[i][j]);
	if(close(fd)) print_err("close the generating file");

//for debugging
for(i=0;i<a;i++) {
	for(j=0;j<b;j++) printf("%d ",mtx1[i][j]);
	printf("\n");
}
printf("\n");
for(i=0;i<b;i++) {
	for(j=0;j<c;j++) printf("%d ",mtx2[i][j]);
	printf("\n");
}	
	return 0;	
}

void print_err(char* dscrp){
	printf("Error: %s\nErrno: %s\n", dscrp, strerror(errno));
	exit(-1);
}

size_t write_int(int fd,void* addr){
	size_t size;
	if((size = write(fd,addr,sizeof(int))) < 0) print_err("write int into generating file");
	return size;
}
