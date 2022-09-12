#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <string.h>

const char* getTime();

int main() {
	pid_t pid, hangar, torre, aereo[10];
	printf("\n  TEMPO   | PID | EVENTO\n");
	if((pid = fork()) < 0) {
		fprintf(stderr, "Errore nella creazione di altri processi!\n");
		return 1;
	}
	else if(pid == 0) {	// Processo figlio
		hangar = getpid();
		printf("%s| %d| hangar creato\n", getTime(), hangar);
	}
	else {	// Processo padre
		torre = getpid();
		printf("%s| %d| torre creata\n", getTime(), torre);
	}
    return 0;
}

const char* getTime() {
	char tempo[11];
	time_t timet;
	time(&timet);
	struct tm *pTm = localtime(&timet);
	sprintf(tempo, "[%02d:%02d:%02d]", pTm->tm_hour, pTm->tm_min, pTm->tm_sec);
	return strdup(tempo);
}