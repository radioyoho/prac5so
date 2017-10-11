#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#define atomic_xchg(A,B)		__asm__ __volatile__(	\
											"	lock xchg %1, %0 ;\n"	\
											: "=ir"	(A)					\
											: "m"	(B), "ir"	(A)		\
											);
#define CICLOS 10
#define MAXTHREAD 10

char *pais[3]={"Peru","Bolvia","Colombia"};
int *g;

//-----------------------------	QUEUE y funciones ---------------------------------------

typedef struct _QUEUE {
	int elements[MAXTHREAD];
	int head;
	int tail;
} QUEUE;

void _initqueue(QUEUE *q)
{
	q->head=0;
	q->tail=0;
}

void _enqueue(QUEUE *q,int val){
    q->elements[q->head]=val;
	// Incrementa al apuntador
	q->head++;
	q->head=q->head%MAXTHREAD;
}
int _dequeue(QUEUE *q){
    int valret;
	valret=q->elements[q->tail];
	// Incrementa al apuntador
	q->tail++;
	q->tail=q->tail%MAXTHREAD;
	return(valret);
}

//------------------------ FIN DE QUEUE -------------------------------------

struct SEMAPHORE {					//estructura del semaforo, contador y cola
	int contador;					//es un apuntador a estructura para que se utilice
	QUEUE cola;		//con memoria compartida
} *s;



void waitsem(struct SEMAPHORE *);
void signalsem(struct SEMAPHORE *);
void initsem(struct SEMAPHORE *);

void proceso(int i)					//Funcion de ferrocarriles
	{
		int l;
		int k;
		for(k=0;k<CICLOS;k++)
		{
			//printf("Hola\n");
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
	int shmid = shmget(IPC_PRIVATE,sizeof(s),0666|IPC_CREAT);
	if(shmid==-1)
	{
		perror("Error en la memoria compartida\n");
		exit(1);
	}
	
	s = (struct SEMAPHORE*)shmat(shmid,NULL,0);
	
	if(s==NULL)
	{
		perror("Error en el shmat\n");
		exit(2);
	}
	//--------------------------------------segunda direccion de memoria compartida---------------------------
	
	int shmid2 = shmget(IPC_PRIVATE,sizeof(g),0666|IPC_CREAT);
	if(shmid2==-1)
	{
		perror("Error en la memoria compartida\n");
		exit(1);
	}
	
	g = shmat(shmid2,NULL,0);
	
	*g = 0;
	
	if(g==NULL)
	{
		perror("Error en el shmat\n");
		exit(2);
	}
	
	//--------------------------------------inicio------------------------------------------------------
	int i;
	int pid;
	int status;
	initsem(s);
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
	
	//---------------------------------- fin de uso de memoria compartida --------------------------------
	shmdt(s);
	shmdt(g);
}

void initsem(struct SEMAPHORE *sema){	//paso por referencia del semaforo
	int l = 1;
	do { atomic_xchg(l,*g);} while(l!=0);//solucion de concurrencia por hardware
	sema->contador = 1;					//se inicia contador en 1
	_initqueue(&sema->cola);			//se inicia cola
	*g=0;
}

void waitsem(struct SEMAPHORE *sema){
	int l = 1;	
	do { atomic_xchg(l,*g);} while(l!=0);//solucion de concurrencia por hardware
	sema->contador--;					//se decrementa el contador, si es menor a 0
	if(sema->contador < 0){				//manda el id del proceso a la cola y bloquea
		//poner id de proceso en cola de bloqueado
		_enqueue(&sema->cola, getpid());
		*g=0;
		//bloquear proceso
		kill(getpid(), SIGSTOP);
		//se puede agregar comprobacion	
	}
	*g=0;
}

void signalsem(struct SEMAPHORE *sema){
	int l = 1;
	int id;
	do { atomic_xchg(l,*g);} while(l!=0);//solucion de concurrencia por hardware
	
	sema->contador++;					//se incrementa contador, si es menor o igual a 0
	if(sema->contador <= 0){			//se quita un id de la cola y se continua proceso con tal id
		//quitar un id de la cola
		id = _dequeue(&sema->cola);
		//desbloquear ese id
		kill(id, SIGCONT);
	}
	*g=0;
}