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
#include <signal.h>
#include <semaphore.h>

#define NUM_AEREI 4

struct aereo {
	pid_t id;	// PID processo aereo
	int nome;	// Ordine di quando è stato creato
};

// Funzioni generiche utili
const char* getTime();
int getRandomSec(int, int);

int main() {
	struct aereo aerei[NUM_AEREI];
	sem_t pista;
	int tempoPreparazione = 0, tempoDecollo = 0, coda[NUM_AEREI], fileDescriptor, status, autorizzazione;
	pid_t hangar, torre;	// Dichiarazioni variabili di tipo pid_t per i PID dei nostri processi
	if(mkfifo("canaleComunicazione", 0777) == -1) {	// Pipe named per il dialogo tra i vari processi
		if(errno != EEXIST) {	// Se la pipe non è stata creata lo comunica all'utente e va in errore
			fprintf(stderr, "Errore nella creazione del canale di comunicazione!\n");
			return -1;
		}
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
				if((fileDescriptor = open("canaleComunicazione", O_RDWR)) == -1) { // L'aereo richiede la disponibilità della pista alla torre
					fprintf(stderr, "Errore apertura del file \"canaleComunicazione\"!\n");
					return -1;
				}
				if(write(fileDescriptor,&aerei[i].nome, sizeof(pid_t)) == -1) {
					fprintf(stderr, "Errore scrittura nel file \"canaleComunicazione\"!\n");
					return -1;
				}
				//if(read(fileDescriptor, &autorizzazione, sizeof(pid_t)) == -1) {
				//	fprintf(stderr, "Errore lettura del file \"canaleComunicazione\"!\n");
				//	return -1;
				//}
				/*if (autorizzazione) {

				}*/
				close(fileDescriptor);
				return 0;
			}

		}
		for(int i = 0; i < NUM_AEREI; i++) {	// Hangar attende che tutti i processi figli aereo terminino
			waitpid(aerei[i].id,&status,0);
		}
		printf("%s|  \e[1;91m%d\033[0m   | Hangar ha finito la creazione dei aerei e sta in attesa\n", getTime(), getpid());
		// OOO Qua va la notifica alla torre che gli aerei sono terminati dopodiché hangar termina
		return 0;
	}
	else {	// Processo padre Torre
		torre = getpid();
		//sem_init(&pista, 1, 2);
		printf("%s|  \033[1;31m%d\033[0m   | Torre di controllo avviata\n", getTime(), getpid());
		if((fileDescriptor = open("canaleComunicazione", O_RDWR)) == -1) {
			fprintf(stderr, "Errore apertura del file \"canaleComunicazione\"!\n");
			return -1;
		}
		for(int i = 0; i < NUM_AEREI; i++) {
			if(read(fileDescriptor, &coda[i], sizeof(pid_t)) == -1) {
				fprintf(stderr, "Errore lettura del file \"canaleComunicazione\"!\n");
				return -1;
			}
			printf("%s|  \033[1;31m%d\033[0m   | Torre ha ricevuto la richiesta di decollo dall'aereo %d\n", getTime(), getpid(), coda[i]+1);
			//sem_post(&pista);
		}
		close(fileDescriptor);
		wait(NULL);
		//sem_destroy(&pista);
		unlink("canaleComunicazione");
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
	srand(time(NULL));	// Inizializzo il seme per la generazione di futuri numeri pseudorandomici
	return ((rand() % (secondiMax - secondiMin + 1)) + secondiMin);
}