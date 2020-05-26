#include <stdlib.h>
#include <fcntl.h>
#include <pthread.h>
#include <errno.h>
#include <semaphore.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define M 10

pthread_t thread_list[M];
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
int thread_counter=0;
sem_t * tornello;
int fd;



char * frase1(pthread_t tid, int secondi){

	char  strA[] = "fase 1, thread id=";

	char * strB = malloc(16*sizeof(char));

	if( strB == NULL ) {
		perror("malloc");
		return NULL;
	}

	snprintf(
			strB, // zona di memoria in cui snprintf scriverà la stringa di caratteri
			16, // dimensione della zona di memoria
			"%lu", // stringa di formattazione
			tid); // numero intero da convertire in stringa

	char  strC[] = ", sleep period=";

	char * strD = malloc(16*sizeof(char));

	if( strD == NULL ) {
		perror("malloc");
		return NULL;
	}

	snprintf(
			strD, // zona di memoria in cui snprintf scriverà la stringa di caratteri
			16, // dimensione della zona di memoria
			"%d", // stringa di formattazione
			secondi); // numero intero da convertire in stringa

	char  strE[] = " secondi\n";

	//preparo la stringa di destinazione
	int message_length = strlen(strA) + strlen(strB) + strlen(strC) + strlen(strD) + strlen(strE) + 1; // \0 finale

	char * message = malloc(sizeof(char) * message_length);

	if ( message == NULL ) {
		perror("malloc");
		return NULL;
	}

	message[0] = '\0';

	strcat(message, strA);
	strcat(message, strB);
	strcat(message, strC);
	strcat(message, strD);
	strcat(message, strE);

	free(strB);
	free(strD);

	return message;
}



int fase1(void){

	int random;

	int res;

	random = rand() % 3 + 0;

	//// ATTENZIONE!!! fuori standard!!! funziona su Linux
	pthread_t tid = pthread_self();
	////

	sleep(random);

	char * message = frase1(tid, random);

	if ( message == NULL ) {
		perror("frase1");
		return -1;
	}

	res = write(fd, message, strlen(message));

	if (res == -1) {
		perror("write()");
		return -1;
	}

	free(message);

	return 0;

}



char * frase2_a(pthread_t tid){

	char  strA[] = "fase 2, thread id=";

	char * strB = malloc(16*sizeof(char));

	if( strB == NULL ) {
		perror("malloc");
		return NULL;
	}

	snprintf(
			strB, // zona di memoria in cui snprintf scriverà la stringa di caratteri
			16, // dimensione della zona di memoria
			"%lu", // stringa di formattazione
			tid); // numero intero da convertire in stringa

	char  strC[] = ", dopo la barriera ";

	//preparo la stringa di destinazione
	int message_length = strlen(strA) + strlen(strB) + strlen(strC) + 1; // \0 finale

	char * message = malloc(sizeof(char) * message_length);

	if ( message == NULL ) {
		perror("malloc");
		return NULL;
	}

	message[0] = '\0';

	strcat(message, strA);
	strcat(message, strB);
	strcat(message, strC);

	free(strB);

	return message;
}



char * frase2_b(pthread_t tid){

	char  strA[] = "thread id=";

	char * strB = malloc(16*sizeof(char));

	if( strB == NULL ) {
		perror("malloc");
		return NULL;
	}

	snprintf(
			strB, // zona di memoria in cui snprintf scriverà la stringa di caratteri
			16, // dimensione della zona di memoria
			"%lu", // stringa di formattazione
			tid); // numero intero da convertire in stringa

	char  strC[] = " bye!\n";

	//preparo la stringa di destinazione
	int message_length = strlen(strA) + strlen(strB) + strlen(strC) + 1; // \0 finale

	char * message = malloc(sizeof(char) * message_length);

	if ( message == NULL ) {
		perror("malloc");
		return NULL;
	}

	message[0] = '\0';

	strcat(message, strA);
	strcat(message, strB);
	strcat(message, strC);

	free(strB);

	return message;
}



//scrive all'interno del file i messaggi della fase 2
int fase2(void){

	int res;

	//// ATTENZIONE!!! fuori standard!!! funziona su Linux
	pthread_t tid = pthread_self();
	////

	char * message = frase2_a(tid);

	if ( message == NULL ) {
		perror("frase2_a");
		return -1;
	}

	res = write(fd, message, strlen(message));

	if (res == -1) {
		perror("write()");
		return -1;
	}

	free(message);

	struct timespec t;

	t.tv_sec = 0;
	t.tv_nsec = 10000000;

	if( nanosleep(&t, NULL) != 0 ) {
		printf("errore timespec\n");
		perror("nanosleep");
		return -1;
	}

	char * end_message = frase2_b(tid);

	if ( end_message == NULL ) {
		perror("frase2_b");
		return -1;
	}

	res = write(fd, end_message, strlen(end_message));

	if (res == -1) {
		perror("write()");
		return -1;
	}

	free(end_message);

	return 0;

}



void * thread_function(void * arg) {

	//blocco il mutex perchè fase1 fa delle operazioni di scrittura nel file (delicate)
	if ( pthread_mutex_lock(&mutex) != 0 ) {
		perror("pthread_mutex_lock");
		return NULL;
	}

	thread_counter++;

	if( fase1() != 0) {
		perror("fase1");
		return NULL;
	}

	if ( pthread_mutex_unlock(&mutex) != 0 ) {
		perror("pthread_mutex_unlock");
		return NULL;
	}

	printf("attualmente %d threads hanno finito la fase 1\n", thread_counter);

	if ( thread_counter == M) {
		//sono arrivati tutti i thread
		printf("tutti i thread hanno finito la fase 1\n");
		if (sem_post(tornello) == -1) {
			perror("sem_post");
			return NULL;
		}
	}

	if (sem_wait(tornello) == -1) {
		perror("sem_post");
		return NULL;
	}

	//passato il primo dico al secondo che può procedere...
	if (sem_post(tornello) == -1) {
		perror("sem_post");
		return NULL;
	}

	printf("passato per tornello\n");

	//se arrivo qui il mutex è gia stato rilasciato, lo torno a bloccare perchè
	//fase 2 devo "aggiornare" il file

	if ( pthread_mutex_lock(&mutex) != 0 ) {
		perror("pthread_mutex_lock");
	}

	if( fase2() != 0) {
		perror("fase2");
		return NULL;
	}

	if ( pthread_mutex_unlock(&mutex) != 0 ) {
		perror("pthread_mutex_unlock");
	}

	return NULL;
}

int main(int argc, char * argv[]) {

	char * file_name;
	int s;
	int thread_res[M];

	if (argc == 1) {
		printf("specificare come parametro il nome del file\n");
		exit(EXIT_FAILURE);
	}

	file_name = argv[1];
	printf("opero sul file %s\n", file_name);

	fd = open(file_name,
				  O_CREAT | O_RDWR,
				  S_IRUSR | S_IWUSR // l'utente proprietario del file avrà i permessi di lettura e scrittura sul nuovo file
				 );

	if (fd == -1) {
		perror("open()");
		exit(EXIT_FAILURE);
	}

	tornello = malloc(sizeof(sem_t));

	if ( tornello == NULL ) {
		perror("malloc");
		exit(EXIT_FAILURE);
	}

	s = sem_init(tornello,
					0, // 1 => il semaforo è condiviso tra processi,
					   // 0 => il semaforo è condiviso tra threads del processo
					0 // valore iniziale del semaforo (se mettiamo 0 che succede?)
				  );

	if ( s == -1 ) {
		perror("sem_init");
		exit(EXIT_FAILURE);
	}

	for(int i=0; i<M; i++) {
		thread_res[i] = pthread_create(&(thread_list[i]), NULL, thread_function, NULL);
		if (thread_res[i] != 0) {
			perror("pthread_create");
			exit(EXIT_FAILURE);
		}
	}

	for(int i=0; i<M; i++) {
		thread_res[i] = pthread_join(thread_list[i], NULL);
		if (thread_res[i] != 0) {
			perror("pthread_create");
			exit(EXIT_FAILURE);
		}
		printf("join numero %d avvenuta con successo con il thread %lu\n", i, thread_list[i]);
	}

	if ( (sem_destroy(tornello)) != 0 ) {
		perror("sem_destroy");
		exit(EXIT_FAILURE);
	}

	printf("Ho fininto, bye\n");

	return 0;

}
