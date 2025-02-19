#include <ncurses.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>  
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/select.h>
#include <sys/file.h>
#include <fcntl.h>
#include <errno.h>
#include "auxfunc.h"
#include <signal.h>
#include "log.h"
#include "cjson/cJSON.h"

#define nfds 19

#define HMARGIN 5
#define WMARGIN 5
#define BMARGIN 2
#define SCALERATIO 1

#define PERIODBB 10000

#define EASY 1
#define HARD 2

#define MAPRESET 5

int nh, nw;
float scaleh = 1.0, scalew = 1.0;

float second = 1000000;

int pid;

int fds[4][4] = {0};
int mode = PLAY;

int levelTime = 0;
int numTarget = 0;
int numObstacle = 0;
int incTime = 0;
int incTarget = 0;
int incObstacle = 0;

WINDOW * win;
WINDOW * map;

FILE *logFile = NULL;
FILE *settingsfile = NULL;

Drone_bb prevDrone = {0, 0, 0, 0, 0, 0};

Message status;
Message msg;
inputMessage inputMsg;
inputMessage inputStatus;

int pids[6] = {0, 0, 0, 0, 0, 0};

int collision = 0;
int targetsHit = 0;

float resetMap = MAPRESET;

float elapsedTime = 0;
int remainingTime = 0;
char difficultyStr[10];

int main(int argc, char *argv[]) {

    // Log file opening
    logFile = fopen("log/logfile.log", "w");
    if (logFile == NULL) {
        perror("Errore nell'aprire il file di log");
        return 1;
    }
    
    if (argc < 5) {
        fprintf(stderr, "Uso: %s <fd_str[0]> <fd_str[1]> <fd_str[2]> <fd_str[3]>\n", argv[0]);
        exit(1);
    }

    for (int i = 0; i < 4; i++) {
        char *fd_str = argv[i + 1];

        int index = 0;

        // Tokenization each value and discard ","
        char *token = strtok(fd_str, ",");
        token = strtok(NULL, ",");

        // FDs ectraction
        while (token != NULL && index < 4) {
            fds[i][index] = atoi(token);
            index++;
            token = strtok(NULL, ",");
        }
    }

    pid = (int)getpid();
    char dataWrite [80] ;
    snprintf(dataWrite, sizeof(dataWrite), "b%d,", pid);

    if(writeSecure("log/passParam.txt", dataWrite,1,'a') == -1){
        perror("Error in writing in passParan.txt");
        exit(1);
    }
    
    //Open config file
    settingsfile = fopen("appsettings.json", "r");
    if (settingsfile == NULL) {
        perror("Error opening the file");
        return EXIT_FAILURE;
    }

    // closing the unused fds to avoid deadlock
    close(fds[DRONE][askwr]);
    close(fds[DRONE][recrd]);
    close(fds[INPUT][askwr]);
    close(fds[INPUT][recrd]);
    close(fds[OBSTACLE][askwr]);
    close(fds[OBSTACLE][recrd]);
    close(fds[TARGET][askwr]);
    close(fds[TARGET][recrd]);

    fd_set readfds;
    struct timespec ts;
    
    //Setting select timeout
    ts.tv_sec = 0;
    ts.tv_nsec = 1000 * 1000;

    
    struct sigaction sa;
    sa.sa_handler = sig_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;

    if (sigaction(SIGUSR1, &sa, NULL) == -1) {
        perror("Error while setting sigaction for SIGUSR1");
        exit(EXIT_FAILURE);
    }

    if (sigaction(SIGTERM, &sa, NULL) == -1) {
        perror("Error while setting sigaction for SIGTERM");
        exit(EXIT_FAILURE);
    }

    if (sigaction(SIGWINCH, &sa, NULL) == -1) {
        perror("Error while setting sigaction for SIGWINCH");
        exit(EXIT_FAILURE);
    }

    initscr();
    start_color();
    curs_set(0);
    noecho();
    cbreak();
    getmaxyx(stdscr, nh, nw);
    win = newwin(nh, nw, 0, 0);
    map = newwin(nh - 2, nw, 2, 0); 
    scaleh = (float)(nh - 2) / (float)WINDOW_LENGTH;
    scalew = (float)nw / (float)WINDOW_WIDTH;

    init_pair(1, COLOR_BLUE, COLOR_BLACK); 
    init_pair(2, COLOR_RED , COLOR_BLACK);  
    init_pair(3, COLOR_GREEN, COLOR_BLACK); 

    usleep(500000);

    char datareaded[200];
    if (readSecure("log/passParam.txt", datareaded,1) == -1) {
        perror("Error reading the passParam file");
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
        }else if (type == 'w') {
            pids[WATCHDOG] = number;
        }
        token = strtok(NULL, ",");
    }

    readConfig();

    inputStatus.score = 0;

    readInputMsg(fds[INPUT][askrd], &inputStatus, 
                "Error reading input", logFile);

    inputStatus.msg = 'A';
    writeInputMsg(fds[INPUT][recwr], &inputStatus, 
                "Error sending ack", logFile);

    if(inputStatus.difficulty == 1){
        strcpy(difficultyStr,"Easy");
        status.difficulty = inputStatus.difficulty;
    } else if(inputStatus.difficulty == 2){
        strcpy(difficultyStr,"Hard");
        status.difficulty = inputStatus.difficulty;
    }

    LOGCONFIG(inputStatus);

    mapInit(logFile);

    elapsedTime = 0;

    while (1) {
        
        elapsedTime += PERIODBB/second;            
        remainingTime = levelTime + (int)(incTime*status.level/status.difficulty) - (int)elapsedTime;


        if (remainingTime < 0){
            elapsedTime = 0;
            LOGENDGAME(status, inputStatus);

            int y_center = (nh - 2) / 2;
            int x_center = nw / 2;

            const char *msg1 = "Time's up! Game over!";
            const char *msg2 = "Press q to quit";

            mvwprintw(map, y_center, x_center - strlen(msg1) / 2, "%s", msg1);
            mvwprintw(map, y_center + 1, x_center - strlen(msg2) / 2, "%s", msg2);
            wrefresh(map);

            quit(); 
        }

        if(elapsedTime >= resetMap && inputStatus.difficulty == HARD){
            resetMap += MAPRESET;
            createNewMap();
        }

        if (targetsHit >= numTarget + status.targets.incr) {
                LOGENDLEVEL(status, inputStatus);
                status.level++;

                if(status.level > 5){

                int y_center = (nh - 2) / 2;
                int x_center = nw / 2;

                const char *msg1 = "Congratulations! You Won!";
                const char *msg2 = "Press q to quit";

                mvwprintw(map, y_center, x_center - strlen(msg1) / 2, "%s", msg1);
                mvwprintw(map, y_center + 1, x_center - strlen(msg2) / 2, "%s", msg2);
                wrefresh(map);

                quit();
            }
                
            inputStatus.level = status.level;  
            status.targets.incr = status.level*status.difficulty*incTarget;
            status.obstacles.incr = status.level*status.difficulty*incObstacle;
            msg.targets.incr = status.targets.incr;
            msg.obstacles.incr = status.obstacles.incr;

            if(numTarget + status.targets.incr > MAX_TARGET){
                status.targets.incr = MAX_TARGET - numTarget;
            }

            if(numObstacle + status.obstacles.incr > MAX_OBSTACLES){
                status.obstacles.incr = MAX_OBSTACLES - numObstacle;
            }
            targetsHit = 0;
            elapsedTime = 0;
            collision = 0;
            resetTargetValue(&status);
            createNewMap();
        }

        // Update the main window
        werase(win);
        werase(map);
        box(map, 0, 0);

        drawMenu(win);
        drawDrone(map);
        drawObstacle(map);
        drawTarget(map);
        wrefresh(win);
        wrefresh(map);
        
        sigset_t mask;
        sigfillset(&mask);

        //FDs setting for select
        FD_ZERO(&readfds);
        FD_SET(fds[DRONE][askrd], &readfds);
        FD_SET(fds[INPUT][askrd], &readfds);

        int fdsQueue [4];
        int ready = 0;

        

        int sel = pselect(nfds, &readfds, NULL, NULL, &ts, &mask);
        
        if (sel == -1) {
            perror("Pselect error");
            break;
        } 

        if (FD_ISSET(fds[DRONE][askrd], &readfds)) {
            fdsQueue[ready] = fds[DRONE][askrd];
            ready++;
        }
        if (FD_ISSET(fds[INPUT][askrd], &readfds)) {
            fdsQueue[ready] = fds[INPUT][askrd];
            ready++;
        }
        if (FD_ISSET(fds[OBSTACLE][askrd], &readfds)) {
            fdsQueue[ready] = fds[OBSTACLE][askrd];
            ready++;
        }
        if (FD_ISSET(fds[TARGET][askrd], &readfds)) {
            fdsQueue[ready] = fds[TARGET][askrd];
            ready++;
        }

        if(ready > 0){
            unsigned int rand = randomSelect(ready);
            int selected = fdsQueue[rand];
            detectCollision(&status, &prevDrone);

            if (selected == fds[DRONE][askrd]){
                LOGPROCESSELECTED(DRONE);
         
                readMsg(fds[DRONE][askrd], &msg,
                                "[BB] Error reading drone position", logFile);

                LOGDRONEINFO(status.drone);

                if(msg.msg == 'R'){
                    
                    if (collision) {
                        LOGTARGETHIT(status);
                        LOGCONFIG(inputStatus);
                        collision = 0;
                        
                        writeMsg(fds[DRONE][recwr], &status, 
                                "[BB] Error asking drone position", logFile);

                        storePreviousPosition(&status.drone);

                        readMsg(fds[DRONE][askrd], &status,
                                "[BB] Error reading drone position", logFile);
                        LOGDRONEINFO(status.drone);

                    }else{
                        
                        status.msg = 'A';

                        writeMsg(fds[DRONE][recwr], &status, 
                                "[BB] Error asking drone position", logFile);

                        status.msg = '\0';

                        storePreviousPosition(&status.drone);

                        readMsg(fds[DRONE][askrd], &status,
                                "[BB] Error reading drone position", logFile);
                        LOGDRONEINFO(status.drone);
                    }
                }
                  
            } else if (selected == fds[INPUT][askrd]){
                
                LOGPROCESSELECTED(INPUT);
                
                readInputMsg(fds[INPUT][askrd], &inputMsg, 
                                "[BB] Error reading input", logFile);

                LOGINPUTMESSAGE(inputMsg);

                if(inputMsg.msg == 'P'){
                    LOGDRONEINFO(status.drone)
                    mode = PAUSE;
                    LOGSTUATUS(mode);

                    inputStatus.msg = 'A';

                    writeInputMsg(fds[INPUT][recwr], &inputStatus, 
                                "[BB] Error sending ack", logFile);
                  
                    inputMsg.msg = 'A';

                    while(inputMsg.msg != 'P' && inputMsg.msg != 'q'){
                        
                    readInputMsg(fds[INPUT][askrd], &inputMsg, 
                                "[BB] Error reading input", logFile);

                    }
                    if(inputMsg.msg == 'P'){
                        mode = PLAY;
                        LOGSTUATUS(mode);
                    }else if(inputMsg.msg == 'q'){
                        
                        inputStatus.msg = 'S';
                        LOGAMESAVING();
                        
                        writeInputMsg(fds[INPUT][recwr], &inputStatus, 
                                    "[BB] Error sending ack", logFile);
                        
                        readInputMsg(fds[INPUT][askrd], &inputMsg, 
                                    "[BB] Error reading input", logFile);
                        
                        if(inputMsg.msg == 'R'){

                            LOGAMESAVED();
                            closeAll();
                        }           
                    }
                    continue;

                }else if (inputMsg.msg == 'q'){
                    LOGAMESAVING();
                    inputStatus.msg = 'S';
                    writeInputMsg(fds[INPUT][recwr], &inputStatus, 
                                "[BB] Error sending ack", logFile);
                    
                    readInputMsg(fds[INPUT][askrd], &inputMsg, 
                                "[BB] Error reading input", logFile);
                    
                    if(inputMsg.msg == 'R'){

                        LOGAMESAVED();
                        closeAll();
                    }           
                }

                readMsg(fds[DRONE][askrd], &msg,
                                "[BB] Error reading drone ready", logFile);

                if(msg.msg == 'R'){

                    status.msg = 'I';
                    strncpy(status.input, inputMsg.input, sizeof(inputStatus.input));
                    
                    writeMsg(fds[DRONE][recwr], &status, 
                                "[BB] Error asking drone position", logFile);     
                }

                storePreviousPosition(&status.drone);

                readMsg(fds[DRONE][askrd], &status,
                                "[BB] Error reading drone position", logFile);
                LOGDRONEINFO(status.drone);

                inputStatus.droneInfo = status.drone;
                
                writeInputMsg(fds[INPUT][recwr], &inputStatus, 
                                "[BB] Error asking drone position", logFile); 

            } else if (selected == fds[OBSTACLE][askrd] || selected == fds[TARGET][askrd]){
                LOGPROCESSELECTED(OBSTACLE);
            }else{
                LOGPROCESSELECTED(999);
            }
        }
        usleep(PERIODBB);
    }

    return 0;
}

/*********************************************************************************************************************/
/***********************************************GUI FUNCTIONS*********************************************************/
/*********************************************************************************************************************/

void drawDrone(WINDOW * win){
    int row = (int)(status.drone.y * scaleh);
    int col = (int)(status.drone.x * scalew);
    wattron(win, A_BOLD);
    wattron(win, COLOR_PAIR(1));   
    mvwprintw(win, row - 1, col, "|");     
    mvwprintw(win, row, col + 1, "--");
    mvwprintw(win, row, col, "+");
    mvwprintw(win, row + 1, col, "|");     
    mvwprintw(win, row , col -2, "--");
    wattroff(win, COLOR_PAIR(1)); 
    wattroff(win, A_BOLD);
}

void drawObstacle(WINDOW * win){
    wattron(win, A_BOLD);
    wattron(win, COLOR_PAIR(2)); 
    for(int i = 0; i < numObstacle + status.obstacles.incr; i++){
        mvwprintw(win, (int)(status.obstacles.y[i] *scaleh), (int)(status.obstacles.x[i]*scalew), "0");
    }
    wattroff(win, COLOR_PAIR(2)); 
    wattroff(win, A_BOLD); 
}

void drawTarget(WINDOW * win) {
    wattron(win, A_BOLD);
    wattron(win, COLOR_PAIR(3)); 
    for(int i = 0; i < numTarget + status.targets.incr; i++){
        if (status.targets.value[i] == 0) continue;
        char val_str[2];
        sprintf(val_str, "%d", (status.targets.value[i] * status.difficulty));
        mvwprintw(win, (int)(status.targets.y[i] * scaleh), (int)(status.targets.x[i] * scalew), "%s", val_str); 
    } 
    wattroff(win, COLOR_PAIR(3)); 
    wattroff(win, A_BOLD);
}

void drawMenu(WINDOW* win) {

    wattron(win, A_BOLD);

    char score_str[10], diff_str[10], time_str[10], level_str[10];
    sprintf(score_str, "%d", inputStatus.score);
    sprintf(diff_str, "%s", difficultyStr);
    sprintf(time_str, "%d", remainingTime);
    sprintf(level_str, "%d", status.level);

    const char* labels[] = { "Score: ", "Player: ", "Difficulty: ", "Time: ", "Level: " };
    const char* values[] = { score_str, inputStatus.name, diff_str, time_str, level_str };

    int num_elements = 5;

    int total_length = 0;
    for (int i = 0; i < num_elements; i++) {
        total_length += strlen(labels[i]) + strlen(values[i]);
    }

    int remaining_space = nw - total_length;
    int spacing = remaining_space / (num_elements + 1);

    int current_position = spacing;
    for (int i = 0; i < num_elements; i++) {
        mvwprintw(win, 0, current_position, "%s%s", labels[i], values[i]);
        current_position += strlen(labels[i]) + strlen(values[i]) + spacing;
    }
    wattroff(win, A_BOLD);
    wrefresh(win);
}

/*********************************************************************************************************************/
/********************************************READING CONFIGURATION****************************************************/
/*********************************************************************************************************************/

void readConfig() {

    int len = fread(jsonBuffer, 1, sizeof(jsonBuffer), settingsfile); 
    if (len <= 0) {
        perror("Error reading the file");
        return;
    }
    fclose(settingsfile);

    cJSON *json = cJSON_Parse(jsonBuffer); 

    if (json == NULL) {
        perror("Error parsing the file");
    }

    levelTime = cJSON_GetObjectItemCaseSensitive(json, "LevelTime")->valueint;
    incTime = cJSON_GetObjectItemCaseSensitive(json, "TimeIncrement")->valueint;
    numTarget = cJSON_GetObjectItemCaseSensitive(json, "TargetNumber")->valueint;
    numObstacle = cJSON_GetObjectItemCaseSensitive(json, "ObstacleNumber")->valueint;
    incTarget = cJSON_GetObjectItemCaseSensitive(json, "TargetIncrement")->valueint;
    incObstacle = cJSON_GetObjectItemCaseSensitive(json, "ObstacleIcrement")->valueint;

    cJSON_Delete(json);
}

/*********************************************************************************************************************/
/***********************************************SIGNAL HANDLER********************************************************/
/*********************************************************************************************************************/

void sig_handler(int signo) {
    if (signo == SIGUSR1) {
        handler(BLACKBOARD);
    } else if (signo == SIGTERM) {
        LOGBBDIED();
        fclose(logFile);
        close(fds[DRONE][recwr]);
        close(fds[DRONE][askrd]);
        close(fds[INPUT][recwr]);
        close(fds[INPUT][askrd]);
        close(fds[OBSTACLE][recwr]);
        close(fds[OBSTACLE][askrd]);
        close(fds[TARGET][recwr]);
        close(fds[TARGET][askrd]);
        exit(EXIT_SUCCESS);
    } else if(signo == SIGWINCH){
        resizeHandler();
    }
}

void resizeHandler(){
    getmaxyx(stdscr, nh, nw);
    scaleh = ((float)(nh - 2) / (float)WINDOW_LENGTH);
    scalew = (float)nw / (float)WINDOW_WIDTH;
    endwin();
    initscr();
    start_color();
    curs_set(0);
    noecho();
    win = newwin(nh, nw, 0, 0);
    map = newwin(nh - 2, nw, 2, 0); 
}


/*********************************************************************************************************************/
/************************************************OTHER FUCTIONS*******************************************************/
/*********************************************************************************************************************/


void storePreviousPosition(Drone_bb *drone) {

    prevDrone.x = drone ->x;
    prevDrone.y = drone ->y;
}



void resetTargetValue(Message* status){
    for(int i = 0; i < MAX_TARGET; i++){
        if(i < numTarget + status->targets.incr){
            status->targets.value[i] = i + 1;
        }
        else{
            status->targets.value[i] = 0;
        }
    }
}

void mapInit(){

    readMsg(fds[DRONE][askrd], &status,
                "[BB] Error reading drone position\n", logFile);
    LOGDRONEINFO(status.drone);
    
    status.level = inputStatus.level;
    status.difficulty = inputStatus.difficulty;

    status.targets.incr = status.level*status.difficulty*incTarget;
    status.obstacles.incr = status.level*status.difficulty*incObstacle;

    msg.targets.incr = status.targets.incr;
    msg.obstacles.incr = status.obstacles.incr;

    resetTargetValue(&status);

    writeMsg(fds[TARGET][recwr], &status, 
            "[BB] Error sending drone position to [TARGET]\n", logFile);
    
    readMsg(fds[TARGET][askrd], &status,
            "[BB] Error reading target\n", logFile);
    
    writeMsg(fds[OBSTACLE][recwr], &status, 
            "[BB] Error sending drone and target position to [OBSTACLE]\n", logFile);

    readMsg(fds[OBSTACLE][askrd], &status,
            "[BB] Error reading obstacles positions\n", logFile);

    writeMsg(fds[DRONE][recwr], &status, 
            "[BB] Error sending updated map\n", logFile);
    
    LOGDRONEINFO(status.drone);

    inputStatus.droneInfo = status.drone;
    inputStatus.msg = 'B';
    writeInputMsg(fds[INPUT][recwr], &inputStatus, 
                "Error sending ack", logFile);

    LOGNEWMAP(status);
}


int randomSelect(int n) {
    unsigned int random_number;
    int random_fd = open("/dev/urandom", O_RDONLY);
    
    if (random_fd == -1) {
        perror("Error opening /dev/urandom");
        return -1;
    }

    if (read(random_fd, &random_number, sizeof(random_number)) == -1) {
        perror("Error reading from /dev/urandom");
        close(random_fd);
        return -1;
    }
    
    close(random_fd);
    
    return random_number % n;
}

void detectCollision(Message* status, Drone_bb * prev) {
    
    for (int i = 0; i < numTarget + status->targets.incr; i++) {
        
        if (status->targets.value[i] && (((prev->x <= status->targets.x[i] + 2 && status->targets.x[i] - 2 <= status->drone.x)  &&
            (prev->y <= status->targets.y[i] + 2 && status->targets.y[i]- 2 <= status->drone.y) )||
            ((prev->x >= status->targets.x[i] - 2 && status->targets.x[i] >= status->drone.x + 2) &&
            (prev->y >= status->targets.y[i] - 2 && status->targets.y[i] >= status->drone.y + 2) ))){
                inputStatus.score += (status->targets.value[i]* status->difficulty);
                status->targets.value[i] = 0;
                collision = 1;
                targetsHit++;   
        }
    }

}

void createNewMap(){

    readMsg(fds[TARGET][askrd],  &msg,
            "[BB] Error reading ready from target", logFile);
    
    if(msg.msg == 'R'){

        writeMsg(fds[TARGET][recwr], &status, 
            "[BB] Error sending drone position to target", logFile);

        readMsg(fds[TARGET][askrd], &status,
            "[BB] Error reading target positions", logFile);
        
        writeMsg(fds[OBSTACLE][recwr], &status, 
            "[BB] Error sending drone and target position to [OBSTACLE]", logFile);

        readMsg(fds[OBSTACLE][askrd], &status,
            "[BB] Reading obstacles positions", logFile);

        storePreviousPosition(&status.drone);

        readMsg(fds[DRONE][askrd],  &msg,
            "[BB] Reading ready from [DRONE]", logFile);

        if(msg.msg == 'R'){

            LOGDRONEINFO(status.drone);

            status.msg = 'M';

            writeMsg(fds[DRONE][recwr], &status, 
                    "[BB] Error asking drone position", logFile);

            storePreviousPosition(&status.drone);

            readMsg(fds[DRONE][askrd], &status,
                    "[BB] Error reading drone position", logFile);
        }
        LOGNEWMAP(status);
    }
}


void closeAll(){

    for(int j = 0; j < 6; j++){
        if (j != BLACKBOARD && pids[j] != 0){ 
            if (kill(pids[j], SIGTERM) == -1) {
            fprintf(logFile,"Process %d is not responding or has terminated\n", pids[j]);
            fflush(logFile);
            }
            LOGPROCESSDIED(pids[j])
        }
    }
    LOGBBDIED();
    fclose(logFile);
    exit(EXIT_SUCCESS);

}

void quit(){
    LOGQUIT();

    readInputMsg(fds[INPUT][askrd], &inputMsg, 
                "[BB] Error reading input", logFile);

    while(inputMsg.msg != 'q'){
                
        fprintf(logFile, "Waiting for quit\n");
        fflush(logFile);

        readInputMsg(fds[INPUT][askrd], &inputMsg, 
                "[BB] Error reading input", logFile);

    }
    
    LOGAMESAVING();
    inputStatus.msg = 'S';
    
    writeInputMsg(fds[INPUT][recwr], &inputStatus, 
                "[BB] Error sending ack", logFile);
    
    readInputMsg(fds[INPUT][askrd], &inputMsg, 
                "[BB] Error reading input", logFile);
    
    if(inputMsg.msg == 'R'){    //input ready, all data are saved
        LOGAMESAVED();
        closeAll();
    }          
}
