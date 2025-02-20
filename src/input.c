#include <ncurses.h>
#include <stdio.h>
#include <string.h> 
#include <stdlib.h>
#include <unistd.h>
#include "auxfunc.h"
#include <signal.h>
#include <cjson/cJSON.h>
#include "keyboardMap.h"
#include "input.h"


int nh, nw;
float scaleh = 1.0, scalew = 1.0;

int btnValues[9] ={0};
char *droneInfoText[6] = {"Position x: ", "Position y: ", "Force x: ", "Force y: ", "Speed x ", "Speed y: "};
char *menuBtn[2] = {"Press P to pause", "Press Q to save & quit"};

int pid;
int fds[4]; 

int level = 0;

float droneInfo[6] = {0.0};

int mode = MENU;   
int disp = CHOOSENAME;

WINDOW * winBut[9]; 
WINDOW * win;
WINDOW* control;

FILE *settingsfile = NULL;
FILE *inputFile = NULL;

Drone_bb drone = {0, 0, 0, 0, 0, 0};
Force force = {0, 0};
Speed speed = {0, 0};

inputMessage inputMsg;
inputMessage inputStatus;
Message msg;
Message status;

Player leaderboard[10];


int main(int argc, char *argv[]) {
    
    if (argc < 2) {
        fprintf(stderr, "Uso: %s <fd_str>\n", argv[0]);
        exit(1);
    }
    
    // Opening log file
    inputFile = fopen("log/input.log", "a");
     
    if (inputFile == NULL) {
        perror("Errore nell'apertura del file");
        exit(1);
    }

    //Open config file
    settingsfile = fopen("appsettings.json", "r");
    if (settingsfile == NULL) {
        perror("Error opening the file");
        return EXIT_FAILURE;
    }

    //Read configuration from json file
    readConfig();

    LOGLEADERBOARD(leaderboard);

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

    pid = (int)getpid();
    char dataWrite [80] ;
    snprintf(dataWrite, sizeof(dataWrite), "i%d,", pid);

    if(writeSecure("log/passParam.txt", dataWrite,1,'a') == -1){
        perror("[INPUT]Error in writing in passParam.txt");
        exit(1);
    }

    //Closing unused pipes heads to avoid deadlock
    close(fds[askrd]);
    close(fds[recwr]);

    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = sig_handler;
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
    nodelay(stdscr, TRUE);

    init_pair(1, COLOR_RED , COLOR_BLACK);

    getmaxyx(stdscr, nh, nw);
    win = newwin(nh, (int)(nw / 2), 0, 0); 
    control = newwin(nh, (int)(nw / 2) - 1, 0, (int)(nw / 2) + 1);

    mainMenu();
    btnSetUp((int)(((float)nh/2)/2),(int)((((float)nw / 2) - 35)/2));
    mode = PLAY;

    inputMsgInit(&inputStatus);

    while (1) {

        int ch;

        if(mode == PLAY){
            if ((ch = getch()) == ERR) {
                usleep(100000);

                werase(win);
                werase(control);
                box(win, 0, 0);
                wrefresh(win); 
                box(control, 0 ,0);               
                wrefresh(control);   
                drawBtn(99); //to make all the buttons white
                drawInfo();
            }else {             
                
                inputStatus.msg = 'I';
                strcpy(inputStatus.input, "reset");

                int btn;
                
                if (ch == btnValues[0]) {
                    btn = LEFTUP;
                    strcpy(inputStatus.input, moves[btn]);
                } else if (ch == btnValues[1]) {
                    btn = UP;
                    strcpy(inputStatus.input, moves[btn]);
                } else if (ch == btnValues[2]) {
                    btn = RIGHTUP;
                    strcpy(inputStatus.input, moves[btn]);
                } else if (ch == btnValues[3]) {
                    btn = LEFT;
                    strcpy(inputStatus.input, moves[btn]);
                } else if (ch == btnValues[4]) {
                    btn = CENTER;
                    strcpy(inputStatus.input, moves[btn]);
                } else if (ch == btnValues[5]) {
                    btn = RIGHT;
                    strcpy(inputStatus.input, moves[btn]);
                } else if (ch == btnValues[6]) {
                    btn = LEFTDOWN;
                    strcpy(inputStatus.input, moves[btn]);
                } else if (ch == btnValues[7]) {
                    btn = DOWN;
                    strcpy(inputStatus.input, moves[btn]);
                } else if (ch == btnValues[8]) {
                    btn = RIGHTDOWN;
                    strcpy(inputStatus.input, moves[btn]);
                }else if (ch == MY_KEY_p || ch == MY_KEY_P){
                    btn = 112; //Pause
                    inputStatus.msg = 'P';
                    mode = PAUSE;
                    LOGSTUATUS(mode);
                    LOGDRONEINFO(inputStatus.droneInfo);
                } else if (ch == MY_KEY_q || ch == MY_KEY_Q){
                    btn = 109; //Quit
                    inputStatus.msg = 'q';

                }else{
                    btn = 99;   //Any of the direction buttons pressed
                } 

                werase(win);
                box(win, 0, 0);       
                wrefresh(win);
                drawBtn(btn);
                usleep(100000);
                werase(win);
                box(win, 0, 0);       
                wrefresh(win);
                drawBtn(99); //to make all the buttons white

                LOGDIRECTION(inputStatus.input);
                
                writeInputMsg(fds[askwr], &inputStatus, 
                            "[INPUT] Error sending message", inputFile);

                readInputMsg(fds[recrd], &inputStatus, 
                            "Error reading ack", inputFile);
                if(inputStatus.msg == 'A'){
                    LOGACK(inputStatus);
                }
                if(inputStatus.msg == 'S'){                    
                    saveGame();

                    inputStatus.msg = 'R';

                    writeInputMsg(fds[askwr], &inputStatus, 
                            "[INPUT] Error sending message", inputFile);
                    
                }

            }  
        }else if(mode == PAUSE){

            while ((ch = getch()) != MY_KEY_P && ch != MY_KEY_p && ch != MY_KEY_Q && ch != MY_KEY_q) {
                pauseMenu();
                usleep(10000);
            }
            if(ch == MY_KEY_P || ch == MY_KEY_p){
                
                wrefresh(stdscr);
                mode = PLAY;
                LOGSTUATUS(mode);
                inputStatus.msg = 'P';
                strcpy(inputStatus.input, "reset");

                writeInputMsg(fds[askwr], &inputStatus, 
                            "[INPUT] Error sending play", inputFile);

            }else if(ch == MY_KEY_Q || ch == MY_KEY_q){

                wrefresh(stdscr);
                inputStatus.msg = 'q';
                strcpy(inputStatus.input, "reset");

                writeInputMsg(fds[askwr], &inputStatus, 
                            "[INPUT] Error sending play", inputFile);

                readInputMsg(fds[recrd], &inputStatus, 
                            "Error reading ack", inputFile);

                if(inputStatus.msg == 'S'){
                    saveGame();

                    inputStatus.msg = 'R';
                    
                    writeInputMsg(fds[askwr], &inputStatus, 
                            "[INPUT] Error sending message", inputFile);
                    
                }
            }
        }

    }
    return 0;
}

/*********************************************************************************************************************/
/***********************************************GUI FUNCTIONS*********************************************************/
/*********************************************************************************************************************/

void btnSetUp (int row, int col){

    for(int i = 0; i < BUTTONS; i++){
        int newRow = row + ((i/3)*BTNDISTR);
        int newCol = col + ((i%3)*BTNDISTC);
        winBut[i] = newwin(BTNSIZER, BTNSIZEC, newRow, newCol);
    }
}

void drawBtn(int b) {
    char btn[10] = "";

    for (int i = 0; i < BUTTONS; i++) {
        if (btnValues[i] != 0) {        

            snprintf(btn, sizeof(btn), "%c", (char)btnValues[i]); 
        } else {
            snprintf(btn, sizeof(btn), " "); 
        }

        werase(winBut[i]);

        int r = 2;
        int c = (BTNSIZEC - strlen(btn)) / 2;

        if (i == b) {
            wattron(winBut[i], COLOR_PAIR(1));  
        }
        box(winBut[i], 0, 0);
        mvwprintw(winBut[i], r, c, "%s", btn);
        wrefresh(winBut[i]);
        if (i == b) {
            wattroff(winBut[i], COLOR_PAIR(1));  
        }
    }
}

void drawName(){
    const int prompt_row = nh / 2 - 2;
    const int name_row = nh / 2; 
    const char *prompt = "Choose a name:";

    werase(stdscr);
    box(stdscr, 0, 0);
    mvwprintw(stdscr, prompt_row, (nw - strlen(prompt)) / 2, "%s", prompt);
    mvwprintw(stdscr, name_row, (nw - strlen(inputStatus.name)) / 2, "%s", inputStatus.name);
    wrefresh(stdscr);
}

void setName() {
    werase(stdscr);
    box(stdscr, 0, 0);

    const char *prompt = "Choose a name:";
    const int prompt_row = nh / 2 - 2; 
    const int name_row = nh / 2;
    const int error_row = nh / 2 + 2; 
    mvwprintw(stdscr, prompt_row, (nw - strlen(prompt)) / 2, "%s", prompt);
    mvwprintw(stdscr, name_row, (nw - strlen(inputStatus.name)) / 2, "%s", inputStatus.name);
    wrefresh(stdscr);

    int ch = 0;
    unsigned long pos = strlen(inputStatus.name); 
    while (ch != MY_KEY_ENTER) {
        ch = getch();

        if (ch == MY_KEY_BACK) {
            if (pos > 0) {
                pos--;
                inputStatus.name[pos] = '\0';
            }
        } else if (ch >= 32 && ch <= 126) {
            if (pos < sizeof(inputStatus.name) - 1 && pos < MAX_LINE_LENGTH - 1) {
                inputStatus.name[pos++] = ch;
                inputStatus.name[pos] = '\0';
            } else if (pos >= MAX_LINE_LENGTH - 1) {
                mvwprintw(stdscr, error_row, (nw - strlen("Name too long, try again.")) / 2, "%s", "Name too long, try again.");
            }
        } else {
            mvwprintw(stdscr, error_row, (nw - strlen("Invalid key, try again.")) / 2, "%s", "Invalid key, try again.");
        }

        werase(stdscr);
        box(stdscr, 0, 0);
        mvwprintw(stdscr, prompt_row, (nw - strlen(prompt)) / 2, "%s", prompt);
        mvwprintw(stdscr, name_row, (nw - strlen(inputStatus.name)) / 2, "%s", inputStatus.name);
        wrefresh(stdscr);
    }

    werase(stdscr);
    box(stdscr, 0, 0);
    char confirmation[110];
    snprintf(confirmation, sizeof(confirmation), "Name set: %s", inputStatus.name);
    mvwprintw(stdscr, nh / 2, (nw - strlen(confirmation)) / 2, "%s", confirmation);
    wrefresh(stdscr);
}

void drawDifficulty() {
    werase(stdscr);
    box(stdscr, 0, 0);

    const char *line1 = "Choose the difficulty you want to play";
    const char *line2 = "1 - easy: Target and obstacles are static";
    const char *line3 = "2 - hard: Target and obstacles are moving";

    int y1 = nh / 2 - 1;
    int y2 = nh / 2;
    int y3 = nh / 2 + 1; 

    int x1 = (nw - strlen(line1)) / 2;
    int x2 = (nw - strlen(line2)) / 2;
    int x3 = (nw - strlen(line3)) / 2;

    mvwprintw(stdscr, y1, x1, "%s", line1);
    mvwprintw(stdscr, y2, x2, "%s", line2);
    mvwprintw(stdscr, y3, x3, "%s", line3);

    wrefresh(stdscr);
}

void setDifficulty(){
    drawDifficulty();
    int ch = 0;
    while (ch != 49 && ch != 50) {
        ch = getch();
        if(ch == 49){
            inputStatus.difficulty = 1;
        }else if(ch == 50){
            inputStatus.difficulty = 2;
        }
        drawDifficulty();
        usleep(10000); 
    }
    werase(stdscr);
}

int keyAlreadyUsed(int key, int index ){
    for(int i = 0; i < index + 1; i++){
        if(key == btnValues[i] || key == MY_KEY_p || key == MY_KEY_q || key == MY_KEY_Q || key == MY_KEY_P){
            mvwprintw(stdscr, 13, 17, "%s", "Key already used");
            wrefresh(stdscr);
            return 1;
        }
    }
    return 0;
}

void setBtns(){
    werase(stdscr);
    box(stdscr, 0, 0);
    const char *line1 = "Do you want to use the default key configuration?";
    const char *line2 = "y - yes";
    const char *line3 = "n - no";

    int col1 = (nw - strlen(line1)) / 2;
    int col2 = (nw - strlen(line2)) / 2;
    int col3 = (nw - strlen(line3)) / 2;

    mvwprintw(stdscr, 10, col1, "%s", line1);
    mvwprintw(stdscr, 11, col2, "%s", line2);
    mvwprintw(stdscr, 12, col3, "%s", line3);

    btnSetUp(14,(col1 + col2)/2);

    wrefresh(stdscr); 

    drawBtn(99);
    usleep(10000);
    int ch = getch();
    if (ch == MY_KEY_N || ch == MY_KEY_n) {
        for(int i = 0; i < BUTTONS; i++){
            werase(stdscr);
            box(stdscr, 0, 0);   
            mvwprintw(stdscr, 10, 10, "%s", "Choose wich key do you want to use for the highlighted direction?"); 
            wrefresh(stdscr);
            drawBtn(i);
            while ((ch = getch()) == ERR || keyAlreadyUsed(ch, i)) {
                usleep(100000);
            }
            
            btnValues[i] = ch;
            usleep(100000);
        }
        werase(stdscr);
    }else if(ch == 121){
        return;
    }else{
        setBtns();
    }
}

void pauseMenu(){
    werase(stdscr);
    box(stdscr, 0, 0);
    const char *prompt = "Press P to play";
    const char *prompt2 = "Press Q to save & quit";
    int prompt_row = nh / 2 - 2;

    mvwprintw(stdscr, prompt_row, (nw - strlen(prompt)) / 2, "%s", prompt);
    mvwprintw(stdscr, prompt_row + 1, (nw - strlen(prompt)) / 2, "%s", prompt2);

    wrefresh(stdscr);
}
 
void mainMenu(){
   
    mode = MENU;
    disp = CHOOSENAME;
    setName();
    disp = CHOOSEBUTTON;
    setBtns();
    disp = CHOOSEDIFF;
    setDifficulty();
    LOGCONFIG(inputStatus);
    
    werase(stdscr);
    wrefresh(stdscr);

    inputStatus.msg = 'A';
    strncpy(inputStatus.input, "left", 10);

    writeInputMsg(fds[askwr], &inputStatus, 
                "Error sending settings", inputFile);

    readInputMsg(fds[recrd], &inputStatus, 
                "Error reading ack", inputFile);

    LOGACK(inputStatus);

    readInputMsg(fds[recrd], &inputStatus, 
                "Error reading drone info", inputFile);
    LOGDRONEINFO(inputStatus.droneInfo);

}

void drawInfo() {

    //Menu part 1

    int initialrow = (int)((nh/3) - 6)/2 > 0 ? (int)((nh/3) - 6)/2 : 0;
    char droneInfoStr[6][20];

    for (int i = 0; i < 6; i++) {

        switch (i) {
            case 0:
                snprintf(droneInfoStr[i], sizeof(droneInfoStr[i]), "%d", inputStatus.droneInfo.x);
                break;
            case 1:
                snprintf(droneInfoStr[i], sizeof(droneInfoStr[i]), "%d", inputStatus.droneInfo.y);
                break;
            case 2:
                snprintf(droneInfoStr[i], sizeof(droneInfoStr[i]), "%.3f", inputStatus.droneInfo.speedX);
                break;
            case 3:
                snprintf(droneInfoStr[i], sizeof(droneInfoStr[i]), "%.3f", inputStatus.droneInfo.speedY);
                break;
            case 4:
                snprintf(droneInfoStr[i], sizeof(droneInfoStr[i]), "%.3f", inputStatus.droneInfo.forceX);
                break;
            case 5:
                snprintf(droneInfoStr[i], sizeof(droneInfoStr[i]), "%.3f", inputStatus.droneInfo.forceY);
                break;
        }
       
        int textLen = strlen(droneInfoText[i]);
        int valueLen = strlen(droneInfoStr[i]);

        int totalLen = textLen + valueLen; 

        int col = ((nw / 2) - totalLen) / 2;

        mvwprintw(control, initialrow + i, col, "%s%s", droneInfoText[i], droneInfoStr[i]);
        wrefresh(control);
    }

     // Menu part 2 

    int initialrow2 = ((nh / 3) + ((nh / 3 - 2) / 2) > 0) ? (( nh / 3) + ((nh / 3 - 2) / 2)) : (nh / 3);
    

    for (int i = 0; i < 2; i++) {

        int textLen = strlen(menuBtn[i]);

        int col = ((nw / 2) - textLen) / 2;

        mvwprintw(control, initialrow2 + i, col, "%s", menuBtn[i]);
        wrefresh(control);
    }

    // Menu part 3 

    int initialrow3 = ((2 * nh / 3) + ((nh / 3 - 10) / 2) > 0) ? (( 2 * nh / 3) + ((nh / 3 - 10) / 2)) : (2 * nh / 3);
    
    mvwprintw(control, initialrow3 - 1, ((nw / 2) - strlen("LEADERBOARD:")) / 2, "%s", "LEADERBOARD:");
    for (int i = 0; i < 10; i++) {

    char playerInfo[150];
    snprintf(playerInfo, sizeof(playerInfo), "Player: %s, score: %d, level: %d", leaderboard[i].name, leaderboard[i].score, leaderboard[i].level);

    int textLen = strlen(playerInfo);

    int col = ((nw / 2) - textLen) / 2;

    mvwprintw(control, initialrow3 + i, col, "%s", playerInfo);
    wrefresh(control);
    }
}

/*********************************************************************************************************************/
/***********************************************SIGNAL HANDLER********************************************************/
/*********************************************************************************************************************/

void sig_handler(int signo) {
    if (signo == SIGUSR1) {
        handler(INPUT);
    }else if(signo == SIGTERM){
        LOGPROCESSDIED();
        fclose(inputFile);
        close(fds[recrd]);
        close(fds[askwr]);
        exit(EXIT_SUCCESS);
    } else if(signo == SIGWINCH){
        resizeHandler();
    }
}

void resizeHandler(){
    if (mode == PLAY){
        getmaxyx(stdscr, nh, nw);
        scaleh = ((float)nh / (float)WINDOW_LENGTH);
        scalew = (float)nw / (float)WINDOW_WIDTH;
        endwin();
        initscr();
        start_color();
        curs_set(0);
        noecho();
        werase(win);
        werase(control);
        werase(stdscr);
        
        wrefresh(stdscr);
        win = newwin(nh, (int)(nw / 2) - 1, 0, 0); 
        control = newwin(nh, (int)(nw / 2) - 1, 0, (int)(nw / 2) + 1);
        box(win, 0, 0); 
        wrefresh(win);      
        box(control, 0, 0);
        wrefresh(control);
        btnSetUp((int)(((float)nh/2)/2),(int)((((float)nw / 2) - 35)/2));
        drawBtn(99); //to make all the buttons white
        drawInfo();
    }else if( mode == MENU && disp == CHOOSENAME){
        getmaxyx(stdscr, nh, nw);  
        scaleh = ((float)nh / (float)WINDOW_LENGTH);
        scalew = (float)nw / (float)WINDOW_WIDTH;
        endwin();
        initscr();
        start_color();
        curs_set(0);
        noecho();
        werase(win);
        werase(control);
        werase(stdscr);
        win = newwin(nh, (int)(nw / 2) - 1, 0, 0); 
        control = newwin(nh, (int)(nw / 2) - 1, 0, (int)(nw / 2) + 1);

        drawName();
    }else if( mode == MENU && disp == CHOOSEBUTTON){
        getmaxyx(stdscr, nh, nw);  
        scaleh = ((float)nh / (float)WINDOW_LENGTH);
        scalew = (float)nw / (float)WINDOW_WIDTH;
        endwin();
        initscr();
        start_color();
        curs_set(0);
        noecho();
        werase(win);
        werase(control);
        werase(stdscr);
        win = newwin(nh, (int)(nw / 2) - 1, 0, 0); 
        control = newwin(nh, (int)(nw / 2) - 1, 0, (int)(nw / 2) + 1);

        btnSetUp((int)(((float)nh/2)/2),(int)((((float)nw / 2) - 35)/2));
        drawBtn(99); //to make all the buttons white
    }else if (mode == MENU && disp == CHOOSEDIFF){
        getmaxyx(stdscr, nh, nw);
        scaleh = ((float)nh / (float)WINDOW_LENGTH);
        scalew = (float)nw / (float)WINDOW_WIDTH;
        endwin();
        initscr();
        start_color();
        curs_set(0);
        noecho();
        werase(win);
        werase(control);
        werase(stdscr);
        win = newwin(nh, (int)(nw / 2) - 1, 0, 0); 
        control = newwin(nh, (int)(nw / 2) - 1, 0, (int)(nw / 2) + 1);

        drawDifficulty();
    }
}

/*********************************************************************************************************************/
/**********************************************R/W CONFIGURATION******************************************************/
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

    strcpy(inputStatus.name, cJSON_GetObjectItemCaseSensitive(json, "PlayerName")->valuestring);
    inputStatus.difficulty = cJSON_GetObjectItemCaseSensitive(json, "Difficulty")->valueint;
    inputStatus.level = cJSON_GetObjectItemCaseSensitive(json, "StartingLevel")->valueint;
    
    cJSON *numbersArray = cJSON_GetObjectItemCaseSensitive(json, "DefaultBTN");
    LOGINPUTCONFIGURATION(numbersArray);

    cJSON *playersArray = cJSON_GetObjectItemCaseSensitive(json, "Players");
    int playersArraySize = cJSON_GetArraySize(playersArray);

    for (int i = 0; i < playersArraySize; ++i) {
        cJSON *player = cJSON_GetArrayItem(playersArray, i);
        if (cJSON_IsObject(player)) {
            strcpy(leaderboard[i].name, cJSON_GetObjectItemCaseSensitive(player, "name")->valuestring);
            leaderboard[i].score = cJSON_GetObjectItemCaseSensitive(player, "score")->valueint;
            leaderboard[i].level = cJSON_GetObjectItemCaseSensitive(player, "level")->valueint;
        }
    }

    cJSON_Delete(json); 
}

void updateLeaderboard() {

    Player currentPlayer;
    strcpy(currentPlayer.name, inputStatus.name);
    currentPlayer.score = inputStatus.score;
    currentPlayer.level = inputStatus.level;

    int position = -1;
    
    int i = 9;
    while(currentPlayer.score > leaderboard[i].score && i >= 0){
        position = i;
        i--;
    }

    if (position == -1) {
        return;
    }

    for (int i = 9; i > position; i--) {
        leaderboard[i] = leaderboard[i - 1];
    }

    leaderboard[position] = currentPlayer;
    LOGLEADERBOARD(leaderboard);
}

void updatePlayersInConfig() {
    FILE *settingsfile = fopen("appsettings.json", "r+");
    if (settingsfile == NULL) {
        perror("Error opening the file");
        return;
    }

    int len = fread(jsonBuffer, 1, sizeof(jsonBuffer), settingsfile);
    if (len <= 0) {
        perror("Error reading the file");
        return;
    }
    fclose(settingsfile);

    cJSON *json = cJSON_Parse(jsonBuffer);
    if (json == NULL) {
        perror("Error parsing the file");
        return;
    }

    cJSON *playersArray = cJSON_GetObjectItemCaseSensitive(json, "Players");
    if (!playersArray || !cJSON_IsArray(playersArray)) {
        perror("Error finding the Players array in the file");
        cJSON_Delete(json);
        return;
    }

    for (int i = 0; i < 10; i++) {
        cJSON *player = cJSON_GetArrayItem(playersArray, i);
        if (cJSON_IsObject(player)) {
            cJSON_ReplaceItemInObjectCaseSensitive(player, "name", cJSON_CreateString(leaderboard[i].name));
            cJSON_ReplaceItemInObjectCaseSensitive(player, "score", cJSON_CreateNumber(leaderboard[i].score));
            cJSON_ReplaceItemInObjectCaseSensitive(player, "level", cJSON_CreateNumber(leaderboard[i].level));
        }
    }

    settingsfile = fopen("appsettings.json", "w");
    if (settingsfile == NULL) {
        perror("Error opening the file for writing");
        cJSON_Delete(json);
        return;
    }

    char *jsonString = cJSON_Print(json);
    fprintf(settingsfile, "%s", jsonString);
    fflush(settingsfile);

    free(jsonString);
    cJSON_Delete(json);
    fclose(settingsfile);
}

void saveGame(){
    LOGAMESAVING();
    updateLeaderboard();
    updatePlayersInConfig();
    LOGAMESAVED();
}

