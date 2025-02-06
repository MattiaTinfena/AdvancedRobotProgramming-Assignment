#include <ncurses.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>  
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>       
#include "auxfunc.h"

#define ask 0       
#define receive 1

#define read 0
#define write 1 

#define PROCESSNUM 4
#define PIPEXPROCESS 2

int main() {

    // Pipes creation 

    int pipes [PROCESSNUM][PIPEXPROCESS][2];

    for (int i = 0; i < PROCESSNUM; i++) {
        for (int j = 0; j < 2; j++){
            if (pipe(pipes[i][j]) == -1) {          
                perror("Pipe creation error");      
                exit(1);
            } 
        }
    }

    
    char fd_str[PROCESSNUM][50];        
    char test[15];                      
    
    // inizialized to 0 to concat the fds
    for(int i = 0; i < PROCESSNUM; i++){    
        sprintf(fd_str[i], "%d", 0);
        strcat(fd_str[i], ",");
    }

    // filling fd_str with the values in pipes (separated by commas)
    for(int i = 0; i < PROCESSNUM; i++){
        for(int j = 0; j < PIPEXPROCESS; j++){
            for(int k = 0; k < 2; k++){
                sprintf(test, "%d", pipes[i][j][k]);
                strcat(fd_str[i], test);
                strcat(fd_str[i], ",");
            }
        }
    }

    FILE *passParamFile = fopen("log/passParam.txt", "w");
    if (passParamFile == NULL) {
        perror("Errore nell'apertura del file passParam");
        exit(1);
    }

    

    pid_t pids[PROCESSNUM];           

    for (int i = 0; i < PROCESSNUM; i++){
        pids[i] = fork();

        if (pids[i] < 0) {
            perror("Errore nella fork");
            exit(1);
        } 

        else if (pids[i] == 0) {
            char *args[] = { NULL, fd_str[i], NULL };

            switch (i) {
                case DRONE:{
                        args[0] = "./bin/drone";
                        if (execvp(args[0], args) == -1) {
                            perror("Errore in execvp per drone");
                            exit(1);
                        }
                    }
                    break;
                case INPUT:{
                        char *argi[] = {"konsole", "-e", "./bin/input", fd_str[i], NULL }; 

                        if (execvp(argi[0], argi) == -1) {
                            perror("Errore in execvp per input");
                            exit(1);
                        }
                    }
                    break;
                case OBSTACLE:{
                        args[0] = "./bin/obstacle";
                        if (execvp(args[0], args) == -1) {
                            perror("Errore in execvp per obstacle");
                            exit(1);
                        }
                    }
                    break;
                case TARGET:{
                        args[0] = "./bin/target";
                        if (execvp(args[0], args) == -1) {
                            perror("Errore in execvp per target");
                            exit(1);
                        }
                    }
                    break;
                default:
                    break;
            }
        }
    }

    // Fork for blackboard
    pid_t pidbb = fork();

    if (pidbb < 0) {
        perror("Blackboard fork error");
        exit(1);
    } 
    else if (pidbb == 0) { 
        //All the descriptor passed to the blackboard
        char *args[] = { "konsole","-e", "./bin/blackBoard", fd_str[0], fd_str[1], fd_str[2], fd_str[3], NULL };
        
        if (execvp(args[0], args) == -1) {
            perror("Error in execvp for blackBoard");
            exit(1);
        }
    }

    // Fork for watchdog
    pid_t pidwd = fork();

    if (pidwd < 0) {
        perror("Watchdog fork error");
        exit(1);
    } 
    else if (pidwd == 0) { 
        char *argw[] = {"./bin/watchdog", NULL };
        
        if (execvp(argw[0], argw) == -1) {
            perror("Error in execvp for watchdog");
            exit(1);
        }
    }

    // wait for the son processes termination
    for (int i = 0; i < PROCESSNUM + 2; i++) { // PROCESSNUM + 2 to include blackboard and watchdog
        wait(NULL);
    }

    fclose(passParamFile);

    return 0;
}
