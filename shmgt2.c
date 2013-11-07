#include <stdio.h>
#include <string.h>
#include <sys/shm.h>
#include <stdlib.h>

#define PATH "./shmgt1.c"

int print_into_stderr(const char str[]);

int main(int argc, char** argv)
{
//PART 1: reading shared memory        
        int lenght;
        int shmid; //IPC descriptor for shared memory
        void *data; //Pointer to shared memory
        char pathname[] = PATH; //File for key generation
        key_t key; //IPC key

        if ((key = ftok(pathname,0)) < 0) {
                print_into_stderr("Can't generate IPC key\n");
                exit(-1);
                }

        //finding a shared memory for generated key
        if ((shmid = shmget(key, sizeof(int),0)) < 0) {
                        print_into_stderr("Can't find the shared memory\n");
                        exit(-1);
                        }
        
        //attaching the shared memory (only with lenght) to the address space of the process
        if ( (data = (void *)shmat(shmid,NULL,0)) == (void *)-1){
                print_into_stderr("Can't attach shared memory\n");
                exit(-1);
                }

        //calculating lenght of the string
        lenght = *(int *)data;

        //detaching shared memory        
        if (shmdt(data) < 0) {
                print_into_stderr("Can't detach shared memory\n");
                exit(-1);
                }

        // AP: а зачем снова ее находить? разве с предыдущей data работать было нельзя?
	// Нельзя, т.к. мы присоединили к адресному пространству процесса только неполную часть сегмента из разделяемой памяти, в которой содержится размер нужной информации (shmat() возвращает адрес сегмента разделяемой памяти в адресном пространстве процесса (!) по его дескриптору System V IPC). Получив его (lenght), мы можем присоединить теперь весь сегмент.
        //finding a shared memory for generated key
        if ((shmid = shmget(key, sizeof(int)+lenght*sizeof(char),0)) < 0) {
                        print_into_stderr("Can't find the shared memory\n");
                        exit(-1);
                        }
        
        //attaching the shared memory (only with lenght) to the address space of the process
        if ( (data = (void *)shmat(shmid,NULL,0)) == (void *)-1){
                print_into_stderr("Can't attach shared memory\n");
                exit(-1);
                } 

        printf("%s", (char*)data+sizeof(int));

        //detaching shared memory        
        if (shmdt(data) < 0) {
                print_into_stderr("Can't detach shared memory\n");
                exit(-1);
                }

//PART 2: deletion of shared memory

        if ( shmctl(shmid, IPC_RMID, NULL) == -1) {
                print_into_stderr("Can't delete shared memory\n");
                exit(-1);
                }

        return 0;
}

int print_into_stderr(const char str[])
{
        int lenght = strlen(str);
        if (write(2,str,lenght) < 0){
                printf("Smth goes wrong in sys call write() in function print_into_stderr()\n");
                return -1;                
                }
        return lenght;
}
