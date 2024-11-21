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
#include <math.h>
#include <signal.h>
//#include <errno.h>

// process that ask or receive
#define askwr 1
#define askrd 0
#define recwr 3
#define recrd 2

#define MAX_DIRECTIONS 80
#define PERIOD 100 //[Hz]
# define DRONEMASS 1
float K = 1.0;

int pid;


typedef struct {
    float x;
    float y;
    float previous_x[2];   // 0 is one before and 1 is is two before
    float previous_y[2];

} Drone;


const char* moves[] = {"up", "down", "right", "left", "upleft", "upright", "downleft", "downright"};


// Useful for debugging
void printPosition(Drone p) {
    printf("Position: (x = %f, y = %f)\n", p.x, p.y);
    printf("BB Pos: (x = %.0f, y = %.0f)\n", round(p.x), round(p.y));
}

// Simulate user input
void fillDirections(const char* filled[], int* size) {
    for (int i = 0; i < *size; i++) {
        int randomIndex = 7;
        filled[i] = moves[randomIndex]; 
    }
}



// Update drone position
Drone updatePosition(Drone* p, const char* direction, Force force) {

    Drone updated_drone = {
        p->x, p->y,
        p->x, p->y,
        p->previous_x[0], p->previous_y[0]
    };

    // store current position and slide the previous position to the pre-previous position
    // p->previous_x[1] = p->previous_x[0];
    // p->previous_y[1] = p->previous_y[0];
    // p->previous_x[0] = p->x;
    // p->previous_y[0] = p->y;

    // conditions to stay inside the window
    int up_condition = (p->y > 1) ? 1 : 0;
    int down_condition = (p->y < WINDOW_WIDTH - 1) ? 1 : 0;
    int right_condition = (p->x < WINDOW_LENGTH - 1) ? 1 : 0;
    int left_condition = (p->x > 1) ? 1 : 0;


    if (strcmp(direction, "up") == 0 && up_condition) updated_drone.x -= force.x;
    else if (strcmp(direction, "down") == 0 && down_condition) updated_drone.y += force.y;
    else if (strcmp(direction, "right") == 0 && right_condition) updated_drone.x += force.x;
    else if (strcmp(direction, "left") == 0 && left_condition) updated_drone.y -= force.y;

    else if (strcmp(direction, "upleft") == 0 && up_condition && left_condition) {
        updated_drone.x -= force.x / sqrt(2);
        updated_drone.y -= force.y / sqrt(2);
        }
    else if (strcmp(direction, "upright") == 0 && up_condition && right_condition) {
        updated_drone.x += force.x / sqrt(2);
        updated_drone.y -= force.y / sqrt(2);
    }
    else if (strcmp(direction, "downleft") == 0 && down_condition && left_condition) {
        updated_drone.x -= force.x / sqrt(2);
        updated_drone.y += force.y / sqrt(2);
    }
    else if (strcmp(direction, "downright") == 0 && down_condition && right_condition) {
        updated_drone.x += force.x / sqrt(2);
        updated_drone.y += force.y / sqrt(2);
    }
    return updated_drone;
}

// Remove first element of the array
void removeFirstElement(const char* directions[], int* size, Drone drone) {
    if (*size == 0) {
        *size = MAX_DIRECTIONS;
        printPosition(drone);
        fillDirections(directions, size);  // Refill the array if empty
    } else {
        for (int i = 1; i < *size; ++i) {
            directions[i - 1] = directions[i]; 
        }
        (*size)--;
    }
}


Drone_bb DroneToDrone_bb(Drone* drone) {
    Drone_bb bb = {(int)round(drone->x), (int)round(drone->y)};
    return bb;
}


Force drone_force(Drone* p, float mass, float K) {

    Force force;

    float derivative1x = (p->x - p->previous_x[0])/PERIOD;
    float derivative2x = (p->previous_x[1] + p->x -2 * p->previous_x[0])/(PERIOD * PERIOD);
    force.x = (mass * derivative2x + K * derivative1x == 0) ? 1: mass * derivative2x + K * derivative1x;

    float derivative1y = (p->y - p->previous_y[0])/PERIOD;
    float derivative2y = (p->previous_y[1] + p->y -2 * p->previous_y[0])/(PERIOD * PERIOD);
    force.y = (mass * derivative2y + K * derivative1y == 0) ? 1: mass * derivative2y + K * derivative1y;  

    return force;
}

Force total_force(Force drone, Force obstacle, Force target) {
    Force total;
    total.x = drone.x + obstacle.x + target.x;
    total.y = drone.y + obstacle.y + target.y;
    
    return total;
}

void sig_handler(int signo) {
    if (signo == SIGUSR1) {
        handler(DRONE, 100);
    }
}


int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Uso: %s <fd_str>\n", argv[0]);
        exit(1);
    }
    
    // Opening log file
    FILE *file = fopen("outputdrone.txt", "a");
    if (file == NULL) {
        perror("Errore nell'apertura del file");
        exit(1);
    }

    // FDs reading
    char *fd_str = argv[1];
    int fds[4]; 
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
    snprintf(dataWrite, sizeof(dataWrite), "d%d,", pid);
    
    if(writeSecure("log.txt", dataWrite, 1, 'a') == -1){
        perror("Error in writing in log.txt");
        exit(1);
    }

    //Closing unused pipes heads to avoid deadlock
    close(fds[askrd]);
    close(fds[recwr]);

    signal(SIGUSR1, sig_handler);

    // Simulate user input
    const char* directions[MAX_DIRECTIONS];
    int directionCount = MAX_DIRECTIONS;
    fillDirections(directions, &directionCount);        
 

    Drone drone = {10.0, 20.0, {10.0, 20.0}, {10.0, 20.0}};  
    Drone_bb drone_bb;
    Force force_d, force_o, force_t;
    Force force;

    char drone_str[80];
    char forceO_str[80];
    char forceT_str[80];

    while (1) {

        force_d = drone_force(&drone, DRONEMASS, K);
        
        drone = updatePosition(&drone, directions[0], force);
        drone_bb = DroneToDrone_bb(&drone);

        snprintf(drone_str, sizeof(drone_str), "%d,%d", drone_bb.x, drone_bb.y);

        if (write(fds[askwr], drone_str, strlen(drone_str)) == -1) { 
            perror("[DRONE] Error sending drone position");
            exit(EXIT_FAILURE);
        }
        
        
        if (read(fds[recrd], &forceO_str, sizeof(forceO_str)) == -1) {
            perror("[DRONE] Error receiving force_o");
            exit(EXIT_FAILURE);
        }
    //     if (sscanf(forceO_str, "%f,%f", &force_o.x, &force_o.y) != 2) {
    //     fprintf(file, "Error parsing force: %s\n", forceO_str);
    //     fflush(file);
    //     exit(EXIT_FAILURE);
    // }
        fprintf(file, "force_o\n");
        fflush(file);

        fromStringtoForce(&force_o, forceO_str, file);
        
        if (read(fds[recrd], &forceT_str, sizeof(forceT_str)) == -1) {
            perror("[DRONE] Error receiving force_t");
            exit(EXIT_FAILURE);
        }
        fprintf(file, "force_o\n");
        fflush(file);
        fromStringtoForce(&force_t, forceT_str, file);

        force = total_force(force_d, force_o, force_t);

        removeFirstElement(directions, &directionCount, drone);
        usleep(1000000/PERIOD); 
    }
    
    // Chiudiamo il file
    fclose(file);
    return 0;
}
