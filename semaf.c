#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/queue.h>

#define atomic_xchg(A,B)		__asm__ __volatile__(	\
											"	lock xchg %1, %0 ;\n"	\
											: "=ir"	(A)					\
											: "m"	(B), "ir"	(A)		\
											);
#define CICLOS 10
char *pais[3]={"Peru","Bolvia","Colombia"};
int *g;

struct queue{
	int id;
	TAILQ_ENTRY(queue) entries;		
};

struct SEMAPHORE {
	int contador;
	TAILQ_HEAD (,queue) cola;
} *s;



void waitsem(struct SEMAPHORE *);
void signalsem(struct SEMAPHORE *);
void initsem(struct SEMAPHORE *);

void proceso(int i)
	{
		int k;
		for(k=0;k<CICLOS;k++)
		{
			// Llamada waitsem implementada en la parte 3
			waitsem(s);
			printf("Entra %s ",pais[i]);
			fflush(stdout);
			sleep(rand()%3);
			printf("- %s Sale\n",pais[i]);
			// Llamada waitsignal implementada en la parte 3
			signalsem(s);
			// Espera aleatoria fuera de la sección crítica
			sleep(rand()%3);
		}
		exit(0);
		// Termina el proceso
	}

void main(){
	//--------------------------------------primera direccion de memoria compartida---------------------------
	int shmid = shmget(0x1234,sizeof(s),0666|IPC_CREAT);
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
	//--------------------------------------segunda direccion de memoria compartida---------------------------
	
	int shmid2 = shmget(0x1234,sizeof(g),0666|IPC_CREAT);
	if(shmid2==-1)
	{
		perror("Error en la memoria compartida\n");
		exit(1);
	}
	
	g = shmat(shmid2,NULL,0);
	
	if(g==NULL)
	{
		perror("Error en el shmat\n");
		exit(2);
	}
	
	//--------------------------------------main stuff------------------------------------------------------
	int i;
	int pid;
	int status;
	srand(getpid());
	
	for(i=0;i<3;i++)
	{
		// Crea un nuevo proceso hijo que ejecuta la función proceso()
		pid=fork();
		if(pid==0)
			proceso(i);
	}
	for(i=0;i<3;i++)
		pid = wait(&status);
	
	//----------------------------------END OF THE LINE BOY--------------------------------
	shmdt(s);
	shmdt(g);
}

void waitsem(struct SEMAPHORE *sema){
	int l = 1;
	struct queue *item;
	do { atomic_xchg(l,*g); } while(l!=0);
	sema->contador--;//	
	if(sema->contador < 0){
		//poner id de proceso en cola de bloqueado
		
		item = malloc(sizeof(*item));
		if (item == NULL) {
			perror("malloc failed");
			exit(EXIT_FAILURE);
		}
		item->id = getpid();
		TAILQ_INSERT_TAIL(&sema->cola, item, entries);
		*g=0;
		
		//bloquear proceso
		kill(getpid(), SIGSTOP);
		//se puede agregar comprobacion	
	}
	*g=0;
}

void signalsem(struct SEMAPHORE *sema){
	int l = 1;
	struct queue *item;
	int id;
	do { atomic_xchg(l,*g); } while(l!=0);
	sema->contador++;
	if(sema->contador <= 0){
		//quitar un id de la cola
		item = TAILQ_FIRST(&sema->cola);
		id = item->id;
		TAILQ_REMOVE(&sema->cola, item, entries);
		free(item);
		//desbloquear ese id
		kill(id, SIGCONT);
	}
	*g=0;
}

void initsem(struct SEMEPHORE *sema){
	sema->contador = 1;
	sema->cola = NULL;
}
