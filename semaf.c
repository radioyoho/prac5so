#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/queue.h>

struct queue{
	int id;
	TAILQ_ENTRY(queue) entries;		
};

struct SEMAPHORE {
	int contador;
	TAILQ_HEAD (,queue) cola;
} *s;


void waitsem(struct SEMAPHORE);
void signalsem(struct SEMAPHORE);

void main(){
	
	struct queue *item;
	
	int shmid = shmget(0x1234,sizeof(shmid),0666|IPC_CREAT);
	if(shmid==-1)
	{
		perror("Error en la memoria compartida\n");
		exit(1);
	}
	
	s = shmat(shmid,NULL,0);
	
	if(s==NULL)
	{
		perror("Error en el shmat\n");
		exit(2);
	}
	
	
	//----------------------------------END OF THE LINE BOY--------------------------------
	shmdt(s);
}

void waitsem(struct SEMAPHORE *sema){
	sema->contador--;
	if(sema->contador < 0){
		//poner id de proceso en cola de bloqueado
		//bloquear proceso
	}
}

void signalsem(struct SEMAPHORE sema){
	sema->contador++;
	if(sema->contador <= 0){
		//quitar un id de la cola
		//desbloquear ese id
	}
}