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

#define NUM_AEREI 10

struct aereo {
	pid_t id;	// PID processo aereo
	int nome;	// Ordine di quando è stato creato
};

// Funzioni generiche utili
const char* getTime();
int getRandomSec(int, int);

// Signals

/*void inserisciInCoda(pid_t);
void rimuoviDallaCoda();
void gestioneSegnali(int);
*/
int main() {
	struct aereo aerei[NUM_AEREI];
	int tempoPreparazione = 0, coda[NUM_AEREI], canale_torre_hangar[2], fileDescriptor, status;	// canale_torre_hangar[0] = read;[1] = write
	pid_t hangar, torre;	// Dichiarazioni variabili di tipo pid_t per i PID dei nostri processi
	printf("\n  TEMPO   |   PID   | EVENTO\n");
	if((hangar = fork()) < 0) {	// Se la nuova fork torna errore
		fprintf(stderr, "Errore nella creazione del processo Hangar!\n");
		return 1;
	}
	else if(hangar == 0) {	// Processo figlio Hangar
		hangar = getpid();	// Salvo il PID nella variabile hangar di tipo pid_t
		printf("%s|  \e[1;91m%d\033[0m   | Hangar creato\n", getTime(), getpid());
		for(int i = 0; i < NUM_AEREI; i++) {
			sleep(2);	// Per la creazione di un aereo ci vogliono 2 secondi
			if((aerei[i].id = fork()) < 0) {	// Creo effettivamente l'aereo e ritorno eventuali errori
				fprintf(stderr, "Errore nella creazione del processo Aereo %d!\n", i+1);
				return 2;
			}
			else if(aerei[i].id == 0) {	// Processi aerei
				aerei[i].nome = i;
				printf("%s|  \033[1;33m%d\033[0m   | Aereo %d creato dall'hangar\n", getTime(), getpid(), i+1);
				tempoPreparazione = getRandomSec(3, 8);
				printf("%s|  \033[1;33m%d\033[0m   | Aereo %d si sta preparando... tempo stimato %d secondi\n", getTime(), getpid(), i+1, tempoPreparazione);
				sleep(tempoPreparazione);
				printf("%s|  \033[1;33m%d\033[0m   | Aereo %d invia richiesta di decollo alla torre\n", getTime(), getpid(), i+1);
				if((fileDescriptor = open("canale_torre_aereo", O_RDWR)) == -1) { // L'aereo richiede la disponibilità della pista alla torre
					fprintf(stderr, "Errore apertura del file \"canale_torre_aereo\"!\n");
					return 6;
				}
				if(write(fileDescriptor,&aerei[i].nome, sizeof(pid_t)) == -1) {
					fprintf(stderr, "Errore scrittura nel file \"canale_torre_aereo\"!\n");
					return 7;
				}
				waitpid(torre,&status,0);
				close(fileDescriptor);
				return 0;
			}
			if(i == (NUM_AEREI - 1)) {
				printf("%s|  \e[1;91m%d\033[0m   | Hangar ha finito la creazione dei aerei e sta in attesa\n", getTime(), getpid());
			}
		}
		for(int i = 0; i < 10; i++) {	// Hangar attende che tutti i processi figli aereo terminino
			waitpid(aerei[i].id,&status,0);
		}
		// OOO Qua va la notifica alla torre che gli aerei sono terminati dopodiché hangar termina (con una pipe unnamed)
	}
	else {	// Processo padre Torre
		torre = getpid();
		printf("%s|  \033[1;31m%d\033[0m   | Torre di controllo avviata\n", getTime(), getpid());
		if(mkfifo("canale_torre_aereo", 0777) == -1) {	// Pipe named per dialogare con processi nipoti
			if(errno != EEXIST) {	// Se il file non è stato creato lo comunica all'utente e va in errore
				fprintf(stderr, "Errore nella creazione del canale di comunicazione Torre <--> Aerei!\n");
				return 3;
			}
		}
		if((fileDescriptor = open("canale_torre_aereo", O_RDWR)) == -1) {
			fprintf(stderr, "Errore apertura del file \"canale_torre_aereo\"!\n");
			return 4;
		}
		for(int i = 0; i < NUM_AEREI; i++) {
			if(read(fileDescriptor, &coda[i], sizeof(pid_t)) == -1) {
				fprintf(stderr, "Errore lettura del file \"canale_torre_aereo\"!\n");
				return 5;
			}
			printf("%s|  \033[1;31m%d\033[0m   | Torre ha ricevuto la richiesta di decollo dall'aereo %d\n", getTime(), getpid(), coda[i]+1);
		}
		close(fileDescriptor);
		waitpid(hangar,&status,0);
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