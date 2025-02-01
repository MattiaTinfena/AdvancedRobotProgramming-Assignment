#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/file.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include "auxfunc.h"
#include <time.h>

int levelTime = 40;
int numTarget = 4;
int numObstacle = 9;
int incTime = 10;
int incTarget = 1;
int incObstacle = 1;

const char *moves[] = {"upleft", "up", "upright", "left", "center", "right", "downleft", "down", "downright"};
char jsonBuffer[MAX_FILE_SIZE];

int writeSecure(char* filename, char* data, int numeroRiga, char mode) {
    if (mode != 'o' && mode != 'a') {
        fprintf(stderr, "Modalità non valida. Usa 'o' per overwrite o 'a' per append.\n");
        return -1;
    }

    FILE* file = fopen(filename, "r+");  // Apertura per lettura e scrittura
    if (file == NULL) {
        perror("Errore nell'apertura del file");
        return -1;
    }

    int fd = fileno(file);
    if (fd == -1) {
        perror("Errore nel recupero del file descriptor");
        fclose(file);
        return -1;
    }

    // Blocca il file per accesso esclusivo
    while (flock(fd, LOCK_EX) == -1) {
        if (errno == EWOULDBLOCK) {
            usleep(100000);  // Pausa di 100 ms
        } else {
            perror("Errore nel blocco del file");
            fclose(file);
            return -1;
        }
    }

    // Legge tutto il file in memoria
    char** righe = NULL;  // Array di righe
    size_t numRighe = 0;  // Numero di righe
    char buffer[1024];    // Buffer per leggere ogni riga

    while (fgets(buffer, sizeof(buffer), file)) {
        righe = realloc(righe, (numRighe + 1) * sizeof(char*));
        if (!righe) {
            perror("Errore nella realloc");
            fclose(file);
            return -1;
        }
        righe[numRighe] = strdup(buffer);  // Duplica la riga letta
        numRighe++;
    }

    // Modifica o aggiunge righe
    if (numeroRiga > numRighe) {
        // Aggiungi righe vuote fino alla riga richiesta
        righe = realloc(righe, numeroRiga * sizeof(char*));
        for (size_t i = numRighe; i < numeroRiga - 1; i++) {
            righe[i] = strdup("\n");  // Righe vuote
        }
        righe[numeroRiga - 1] = strdup(data);  // Nuova riga
        numRighe = numeroRiga;
    } else {
        // Se la riga esiste, modifica in base alla modalità
        if (mode == 'o') {
            // Sovrascrivi il contenuto della riga
            free(righe[numeroRiga - 1]);
            righe[numeroRiga - 1] = strdup(data);
        } else if (mode == 'a') {
            // Rimuovi il newline alla fine della riga esistente
            size_t len = strlen(righe[numeroRiga - 1]);
            if (len > 0 && righe[numeroRiga - 1][len - 1] == '\n') {
                righe[numeroRiga - 1][len - 1] = '\0';
            }
            // Concatena il nuovo testo
            char* nuovoContenuto = malloc(len + strlen(data) + 2);
            if (!nuovoContenuto) {
                perror("Errore nella malloc");
                fclose(file);
                return -1;
            }
            sprintf(nuovoContenuto, "%s%s\n", righe[numeroRiga - 1], data); // Nessuno spazio extra
            free(righe[numeroRiga - 1]);
            righe[numeroRiga - 1] = nuovoContenuto;
        }
    }

    // Riscrive il contenuto nel file
    rewind(file);
    for (size_t i = 0; i < numRighe; i++) {
        fprintf(file, "%s", righe[i]);
        if (righe[i][strlen(righe[i]) - 1] != '\n') {
            fprintf(file, "\n");  // Aggiungi newline se mancante
        }
        free(righe[i]);  // Libera la memoria per ogni riga
    }
    free(righe);  // Libera l'array di righe

    // Trunca il file a lunghezza corrente
    if (ftruncate(fd, ftell(file)) == -1) {
        perror("Errore nel troncamento del file");
        fclose(file);
        return -1;
    }

    fflush(file);

    // Sblocca il file
    if (flock(fd, LOCK_UN) == -1) {
        perror("Errore nello sblocco del file");
        fclose(file);
        return -1;
    }

    fclose(file);
    return 0;
}

int readSecure(char* filename, char* data, int numeroRiga) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        perror("Errore nell'apertura del file");
        return -1;
    }

    int fd = fileno(file);
    if (fd == -1) {
        perror("Errore nel recupero del file descriptor");
        fclose(file);
        return -1;
    }

    // Blocca il file per lettura condivisa
    while (flock(fd, LOCK_SH) == -1) {
        if (errno == EWOULDBLOCK) {
            usleep(100000);  // Pausa di 100 ms
        } else {
            perror("Errore nel blocco del file");
            fclose(file);
            return -1;
        }
    }

    // Leggi fino alla riga richiesta
    int rigaCorrente = 1;
    char buffer[1024];  // Buffer temporaneo per leggere le righe
    while (fgets(buffer, sizeof(buffer), file)) {
        if (rigaCorrente == numeroRiga) {
            // Copia la riga nel buffer di output
            strncpy(data, buffer, 1024);
            data[1023] = '\0';  // Assicurati che sia terminata correttamente
            break;
        }
        rigaCorrente++;
    }

    // Controlla se abbiamo raggiunto la riga desiderata
    if (rigaCorrente < numeroRiga) {
        fprintf(stderr, "Errore: Riga %d non trovata nel file.\n", numeroRiga);
        flock(fd, LOCK_UN);
        fclose(file);
        return -1;
    }

    // Sblocca il file
    if (flock(fd, LOCK_UN) == -1) {
        perror("Errore nello sblocco del file");
        fclose(file);
        return -1;
    }

    fclose(file);
    return 0;
}

void writeMsg(int pipeFds, Message* msg, char* error, FILE* file){
  if (write(pipeFds, msg, sizeof(Message)) == -1) {
        fprintf(file,"Error: %s\n", error);
        fflush(file);
        perror(error);
        exit(EXIT_FAILURE);
    }  
}

void readMsg(int pipeFds, Message* msgOut, char* error, FILE* file){   
    if (read(pipeFds, msgOut, sizeof(Message)) == -1){
        fprintf(file, "Error: %s\n", error);
        fflush(file);
        perror(error);
        exit(EXIT_FAILURE);
    }
}

void writeInputMsg(int pipeFds, inputMessage* msg, char* error, FILE* file){
    if (write(pipeFds, msg, sizeof(inputMessage)) == -1) {
        fprintf(file,"Error: %s\n", error);
        fflush(file);
        perror(error);
        exit(EXIT_FAILURE);
    }  
}
void readInputMsg(int pipeFds, inputMessage* msgOut, char* error, FILE* file){
    if (read(pipeFds, msgOut, sizeof(inputMessage)) == -1){
        fprintf(file, "Error: %s\n", error);
        fflush(file);
        perror(error);
        exit(EXIT_FAILURE);
    }
}

void fdsRead (int argc, char* argv[], int* fds){
    if (argc < 2) {
        fprintf(stderr, "Uso: %s <fd_str>\n", argv[0]);
        exit(1);
    }

     // FDs reading
    char *fd_str = argv[1];
    int index = 0;

    char *token = strtok(fd_str, ",");
    token = strtok(NULL, ",");

    // FDs extraction
    while (token != NULL && index < 4) {
        fds[index] = atoi(token);
        index++;
        token = strtok(NULL, ",");
    }
}

int writePid(char* file, char mode, int row, char id){

    int pid = (int)getpid();
    char dataWrite[80];
    snprintf(dataWrite, sizeof(dataWrite), "%c%d,",id, pid);

    if (writeSecure(file, dataWrite, row, mode) == -1) {
        perror("Error in writing in log.txt");
        exit(1);
    }

    return pid;
}

void printInputMessageToFile(FILE *file, inputMessage* msg) {
    fprintf(file, "\n");
    fprintf(file, "msg: %c\n", msg->msg);
    fprintf(file, "name: %s\n", msg->name);
    fprintf(file, "input: %s\n", msg->input);
    fprintf(file, "difficulty: %d\n", msg->difficulty);
    fprintf(file, "level: %d\n", msg->level);
    fprintf(file,"Score: %d\n", msg->score);
    fprintf(file, "droneInfo:\n");
    fprintf(file, "  x: %d\n", msg->droneInfo.x);
    fprintf(file, "  y: %d\n", msg->droneInfo.y);
    fprintf(file, "  speedX: %.2f\n", msg->droneInfo.speedX);
    fprintf(file, "  speedY: %.2f\n", msg->droneInfo.speedY);
    fprintf(file, "  forceX: %.2f\n", msg->droneInfo.forceX);
    fprintf(file, "  forceY: %.2f\n", msg->droneInfo.forceY);
    fflush(file);
}

void printMessageToFile(FILE *file, Message* msg) {
    fprintf(file, "\n");
    fprintf(file, "msg: %c\n", msg->msg);
    fprintf(file, "level: %d\n", msg->level);
    fprintf(file, "difficulty: %d\n", msg->difficulty);
    fprintf(file, "input: %s\n", msg->input);

    fprintf(file, "\ndroneInfo:\n");
    fprintf(file, "  x: %d\n", msg->drone.x);
    fprintf(file, "  y: %d\n", msg->drone.y);
    fprintf(file, "  speedX: %.2f\n", msg->drone.speedX);
    fprintf(file, "  speedY: %.2f\n", msg->drone.speedY);
    fprintf(file, "  forceX: %.2f\n", msg->drone.forceX);
    fprintf(file, "  forceY: %.2f\n", msg->drone.forceY);

    fprintf(file, "\nTargets:\n");
    for(int i = 0; i < MAX_TARGET; i++ ){
            fprintf(file, "targ[%d] = %d,%d,%d\n", i, msg->targets.x[i], msg->targets.y[i], msg->targets.value[i]);
        }

    fprintf(file, "\nObstacles:\n");
    for(int i = 0; i < MAX_OBSTACLES; i++ ){
            fprintf(file, "obs[%d] = %d,%d\n", i, msg->obstacles.x[i], msg->obstacles.y[i]);
            fflush(file);
        }
    fflush(file);
}

void msgInit(Message* status){
    status->msg = 'R';
    status->level = 0;
    status->difficulty =0;
    strcpy(status->input,"Reset");
    status->drone.x = 0;
    status->drone.y = 0;
    status->drone.speedX = 0;
    status->drone.speedY = 0;
    status->drone.forceX = 0;
    status->drone.forceY = 0;
    
    for(int i = 0; i < MAX_TARGET; i++){
        status->targets.x[i] = 0;
        status->targets.y[i] = 0;
        status->targets.value[i] = 0;
    }

    for(int i = 0; i < MAX_OBSTACLES; i++){
        status->obstacles.x[i] = 0;
        status->obstacles.y[i] = 0;
    }
}

void inputMsgInit(inputMessage* status){
    status->msg = 'R';
    strcpy(status->name,"Default");
    strcpy(status->input,"Reset");
    status->difficulty =0;
    status->level = 0;
    status->score = 0;
    status->droneInfo.x = 0;
    status->droneInfo.y = 0;
    status->droneInfo.speedX = 0;
    status->droneInfo.speedY = 0;
    status->droneInfo.forceX = 0;
    status->droneInfo.forceY = 0;
}

void handler(int id) {

    char log_entry[256];
    time_t rawtime;
    struct tm *timeinfo;
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    strftime(log_entry, sizeof(log_entry), "%H:%M:%S", timeinfo);
    writeSecure("log/log.txt", log_entry, id + 3, 'o');
}

// Funzione helper per ottenere il timestamp formattato
void getFormattedTime(char *buffer, size_t size) {
    time_t currentTime = time(NULL);
    snprintf(buffer, size, "%.*s", (int)(strlen(ctime(&currentTime)) - 1), ctime(&currentTime));
}