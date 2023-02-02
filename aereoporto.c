#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <semaphore.h>
#include <signal.h>

#define NUM_AEREI 10

struct aereo {
	pid_t id;	// PID processo aereo
	int nome;	// Ordine di quando è stato creato
};

//void handler(int segnale) {}

// Funzioni generiche utili
const char* getTime();
int getRandomSec(int, int);

int main() {
	struct aereo aerei[NUM_AEREI];
	//struct sigaction sa = {0};
	sem_t *pista;
	int tempoPreparazione = 0, tempoDecollo = 0, coda[NUM_AEREI], status;
	int fd[2];
	int fd1[2];
	int fd2[2];
	pid_t hangar, torre, autorizzato;	// Dichiarazioni variabili di tipo pid_t per i PID dei nostri processi
	pista = sem_open("pista", O_CREAT, 0644, 2);	// La pista non è altro che un semaforo named
	//sa.sa_handler = &handler;
	if(pipe(fd) == -1) {	// Creo le seguenti pipe per far comunicare i processi
		fprintf(stderr, "Errore nella creazione della pipe!\n");
		return -1;
	}
	if(pipe(fd1) == -1) {
		fprintf(stderr, "Errore nella creazione della pipe!\n");
		return -1;
	}
	if(pipe(fd2) == -1) {
		fprintf(stderr, "Errore nella creazione della pipe!\n");
		return -1;
	}
	printf("\n  TEMPO   |   PID   | EVENTO\n");
	if((hangar = fork()) < 0) {	// Se la nuova fork torna errore
		fprintf(stderr, "Errore nella creazione del processo Hangar!\n");
		return -1;
	}
	else if(hangar == 0) {	// Processo figlio Hangar
		hangar = getpid();	// Salvo il PID nella variabile hangar di tipo pid_t
		printf("%s|  \e[1;91m%d\033[0m   | Hangar creato\n", getTime(), getpid());
		for(int i = 0; i < NUM_AEREI; i++) {
			sleep(2);	// Per la creazione di un aereo ci vogliono 2 secondi
			if((aerei[i].id = fork()) < 0) {	// Creo effettivamente l'aereo e ritorno eventuali errori
				fprintf(stderr, "Errore nella creazione del processo Aereo %d!\n", i+1);
				return -1;
			}
			else if(aerei[i].id == 0) {	// Processi nipoti aerei
				aerei[i].nome = i;
				printf("%s|  \033[1;33m%d\033[0m   | Aereo %d creato dall'hangar\n", getTime(), getpid(), i+1);
				tempoPreparazione = getRandomSec(3, 8);
				printf("%s|  \033[1;33m%d\033[0m   | Aereo %d si sta preparando... tempo stimato %d secondi\n", getTime(), getpid(), i+1, tempoPreparazione);
				sleep(tempoPreparazione);
				printf("%s|  \033[1;33m%d\033[0m   | Aereo %d invia richiesta di decollo alla torre\n", getTime(), getpid(), i+1);
				// L'aereo ha finito di prepararsi e ora invia la richiesta di decollo alla torre
				if(write(fd[1],&aerei[i].nome, sizeof(int)) == -1) {
					fprintf(stderr, "Errore comunicazione tra i processi 1 aereo\n");
					return -1;
				}
				close(fd[1]);
				if(read(fd1[0], &autorizzato, sizeof(pid_t)) == -1) {
					fprintf(stderr, "Errore comunicazione tra i processi 2 aereo\n");
					return -1;
				}
				close(fd1[0]);
				if(autorizzato == aerei[i].id) {
					sem_wait(pista);
					tempoDecollo = getRandomSec(5, 15);
					printf("%s|  \033[1;33m%d\033[0m   | Aereo %d sta decollando... tempo stimato %d secondi\n", getTime(), getpid(), aerei[i].nome+1, tempoDecollo);
					sleep(tempoDecollo);
					printf("%s|  \033[1;33m%d\033[0m   | Aereo %d notifica la torre che ha liberato la pista\n", getTime(), getpid(), aerei[i].nome+1);
					sem_post(pista);
				}
				return 0;
			}
		}
		sleep(1);
		printf("%s|  \e[1;91m%d\033[0m   | Hangar ha finito la creazione dei aerei e sta in attesa\n", getTime(), getpid());
		for(int i = 0; i < NUM_AEREI; i++) {	// Hangar attende che tutti i processi figli aereo terminino
			waitpid(aerei[i].id,&status,0);
		}
		//kill(torre, SIGUSR1);
		return 0;
	}
	else {	// Processo padre Torre
		torre = getpid();
		printf("%s|  \033[1;31m%d\033[0m   | Torre di controllo avviata\n", getTime(), getpid());
		for(int i = 0; i < NUM_AEREI; i++) {
			if(read(fd[0], &coda[i], sizeof(int)) == -1) {
				fprintf(stderr, "Errore comunicazione tra i processi 1 torre\n");
				return -1;
			}
			printf("%s|  \033[1;31m%d\033[0m   | Torre ha ricevuto la richiesta di decollo dall'aereo %d\n", getTime(), getpid(), coda[i]+1);
		}
		close(fd[0]);
		for(int i = 0; i < NUM_AEREI; i++) {
			autorizzato = aerei[coda[i]].id;
			if(write(fd1[1], &autorizzato, sizeof(pid_t)) == -1) {
				fprintf(stderr, "Errore comunicazione tra i processi 2 torre\n");
				return -1;
			}
			printf("%s|  \033[1;31m%d\033[0m   | Torre da' l'autorizzazione per il decollo all'aereo %d\n", getTime(), getpid(), coda[i]+1);
			//sem_wait(pista);
			//if(read(fd2[0], &autorizzato, sizeof(int)) == -1) {
			//	fprintf(stderr, "Errore comunicazione tra i processi 3 torre\n");
			//	return -1;
			//}
			//sem_post(pista);
			//printf("%s|  \033[1;31m%d\033[0m   | Torre da' libera una pista di decollo\n", getTime(), getpid());
		}
		close(fd1[1]);
		//close(fd2[0]);
		waitpid(hangar,&status,0);
		sem_close(pista);
		sem_unlink("pista");
		return 0;
	}
    return 0;
}

const char* getTime() {
	char tempo[11];
	time_t timet;
	time(&timet);
	struct tm *pTm = localtime(&timet);	// Creo la struttura
	sprintf(tempo, "[%02d:%02d:%02d]", pTm->tm_hour, pTm->tm_min, pTm->tm_sec);	// Salvo il tempo attuale nella stringa tempo
	return strdup(tempo);	// Porto la stringa tempo dallo stack all'heap e la ritorno
}

int getRandomSec(int secondiMin, int secondiMax) {	// Funzione che genera un numero pseudorandomico
	srand(getpid());	// Inizializzo il seme per la generazione di futuri numeri pseudorandomici
	return ((rand() % (secondiMax - secondiMin + 1)) + secondiMin);
}