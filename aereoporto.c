#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>

const char* getTime();
int getRandomSec(int, int);

int main() {
	int tempoPreparazione = 0;
	pid_t hangar, torre, aereo[10];	// Dichiarazioni variabili di tipo pid_t
	printf("\n  TEMPO   |   PID   | EVENTO\n");
	if((hangar = fork()) < 0) {	// Se la nuova fork torna errore
		fprintf(stderr, "Errore nella creazione del processo hangar!\n");
		return 1;
	}
	else if(hangar == 0) {	// Processo figlio
		hangar = getpid();	// Salvo il PID nella variabile hangar di tipo pid_t
		printf("%s|  \e[1;91m%d\033[0m   | Hangar creato\n", getTime(), getpid());
		for(int i = 0; i < 10; i++) {
			sleep(2);	// Per la creazione di un aereo ci vogliono 2 secondi
			if((aereo[i] = fork()) < 0) {	// Creo effettivamente l'aereo e ritorno eventuali errori
				fprintf(stderr, "Errore nella creazione del processo Aereo %d!\n", i+1);
				return 2;
			}
			else if(aereo[i] == 0) {	// Processi aerei 
				printf("%s|  \033[1;33m%d\033[0m   | Aereo %d creato dall'hangar\n", getTime(), getpid(), i+1);
				tempoPreparazione = getRandomSec(3, 8);
				printf("%s|  \033[1;33m%d\033[0m   | Aereo %d si sta preparando... tempo stimato %d\n", getTime(), getpid(), i+1, tempoPreparazione);
				sleep(tempoPreparazione);
				printf("%s|  \033[1;33m%d\033[0m   | Aereo %d invia notifica alla torre\n", getTime(), getpid(), i+1);
				return 0;
			}
		}
		for(int i = 0; i < 10; i++) {	// Hangar attende che tutti i processi figli aereo terminino
			wait(NULL);
		}
		printf("%s|  \e[1;91m%d\033[0m   | Hangar ha finito la creazione dei aerei\n", getTime(), getpid());
		printf("%s|  \e[1;91m%d\033[0m   | Hangar inizia l'attesa decollo aerei\n", getTime(), getpid());
		// OOO Qua va la notifica alla torre che gli aerei sono terminati dopodiché hangar termina (con una pipe)
	}
	else {	// Processo padre
		torre = getpid();
		printf("%s|  \033[1;31m%d\033[0m   | Torre di controllo avviata\n", getTime(), torre);
		// OOO Qua creerò il file FIFO
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