#include <ncurses.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>  
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/wait.h>
#include "watchdog.h"
#include "auxfunc.h"
#include <signal.h>
#include <time.h>

#define PROCESSTOCONTROL 5

int pids[PROCESSTOCONTROL] = {0};  // Initialize PIDs to 0

struct timeval start, end;
long elapsed_ms;

FILE *wdFile = NULL;

void sig_handler(int signo) {
    if(signo == SIGTERM){
        LOGWDDIED();
        fclose(wdFile);
        exit(EXIT_SUCCESS);
    }
}


void closeAll(int id){
    for(int i  = 0; i < PROCESSTOCONTROL; i++){
        if (i != id) {
            if (kill(pids[i], SIGTERM) == -1) {
                LOGPROCESSDIED(pids[i]);
            }
        }
    }
    LOGWDDIED();
    fclose(wdFile);
    exit(EXIT_SUCCESS);
}

int main() {
    // Open the output wdFile for writing
    wdFile = fopen("log/watchdog.log", "w");
    if (wdFile == NULL) {
        perror("Error opening the wdFile");
        exit(1);
    }

    int pid = (int)getpid();
    char dataWrite [80] ;
    snprintf(dataWrite, sizeof(dataWrite), "w%d,", pid);

    if(writeSecure("log/passParam.txt", dataWrite,1,'a') == -1){
        perror("[WATCHDOG] Error in writing in passParam.txt");
        exit(1);
    }

    sleep(1);

    char datareaded[200];
    if (readSecure("log/passParam.txt", datareaded,1) == -1) {
        perror("Error reading the passParam wdFile");
        exit(1);
    }

    // Parse the data and assign roles
    char *token = strtok(datareaded, ",");
    while (token != NULL) {
        char type = token[0];          // Get the prefix
        int number = atoi(token + 1);  // Convert the number part to int

        if (type == 'i') {
            pids[INPUT] = number;
        } else if (type == 'd') {
            pids[DRONE] = number;
        } else if (type == 'o') {
            pids[OBSTACLE] = number;
        } else if (type == 't') {
            pids[TARGET] = number;   
        } else if (type == 'b') {
            pids[BLACKBOARD] = number;
        }else{
            ;
        }

        token = strtok(NULL, ",");
    }

    // // Write the PID values to the output wdFile
    // for (int i = 0; i < PROCESSTOCONTROL; i++) {
    //     fprintf(wdFile, "pid[%d] = %d\n", i, pids[i]);
    //     fflush(wdFile);
    // }

       
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = sig_handler;
    sa.sa_flags = SA_RESTART;  // Riavvia read/write interrotte
    if (sigaction(SIGUSR1, &sa, NULL) == -1) {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }

    // while(1) {
    //     sleep(2);
    //     if (kill(pids[TARGET], SIGUSR1) == -1) {
    //         perror("[WATCHDOG] target not responding");
    //         exit(1);
    //     }
    // }
    
    for (int i = 0; i < PROCESSTOCONTROL; i++) {
            if (kill(pids[i], SIGUSR1) == -1) {
                LOG_PROCESS_NOT_RESPONDING(pids[i]);
                closeAll(i);
            }
        usleep(10000);
    }

    int interval = 0;

    while (1) {

        sleep(1);
        interval++;

        if(interval >= 4){
            interval = 0;
            for (int i = 0; i < PROCESSTOCONTROL; i++) {
                    if (kill(pids[i], SIGUSR1) == -1) {
                        LOG_PROCESS_NOT_RESPONDING(pids[i]);
                        closeAll(i);
                    }
                // usleep(10000);
            }
        }   

        usleep(10000);
        time_t rawtime;
        struct tm *timeinfo;
        time(&rawtime);
        timeinfo = localtime(&rawtime);

        char timeReaded[80];
        for(int i = 0; i < PROCESSTOCONTROL; i++){
            if(readSecure("log/passParam.txt", timeReaded, i + 3) == -1){
                perror("Error reading the passParam wdFile");
                fclose(wdFile);
                exit(1);
            }

            int hours, minutes, seconds;
            sscanf(timeReaded, "%d:%d:%d", &hours, &minutes, &seconds);
            long timeReadedInSeconds = hours * 3600 + minutes * 60 + seconds;
            
            long currentTimeInSeconds = timeinfo->tm_hour * 3600 + timeinfo->tm_min * 60 + timeinfo->tm_sec;
            long timeDifference = currentTimeInSeconds - timeReadedInSeconds;

            if (timeDifference > 5) {
                closeAll(i);
            }
        } 
    }                 
    
    //Close the wdFile
    fclose(wdFile);

    return 0;
}
