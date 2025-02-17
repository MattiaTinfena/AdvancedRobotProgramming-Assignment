
# Advanced & Robot Programming Assignment

## Description
This repository is the delivery of the first assignment of the Advanced and Robot Programming course. The project, implemented in C, simulates a drone moving in a 2D space to reach targets in the environment while avoiding obstacles. The GUI is implemented using ncurses, and communication between processes is via pipes. 

## Installation Instructions and Requirements
To set up the project, follow these steps:
1. Clone the repository: ```git@github.com:MattiaTinfena/AdvancedRobotProgramming-Assignment.git ```
2. Make the installation script executable: ```chmod +x install.sh ```
3. Run the script: ```./install.sh```

The `install.sh` scripts automates the installation of required dependencies for the project and performs the following actions:
1. Updates the package lists and upgrades installed packages.
2. Installs necessary terminal emulators (Terminator and Konsole).
3. Installs development libraries for ncurses and cJSON.
4. Cleans up previous build files and compiles the project.


## Usage and Recompilation
After installation, you can run the project using one of the following methods:
- Direct execution: ``` ./bin/main```
- Using the vscode environment with the launch.json file provided in the .vscode folder.

If some changes are made to the code, it is necessary to run the make command to recompile the project before running it. Otherwise, you can use the "Run Code" if you are using vscode.

## Project Architecture
The project architecture of the assignment includes 6 active components, a parameter file ("appsettings.json"), and a log folder ("log") for all logger file. 

![Project Architecture Overview](docs\ARP1-ass1.png)

### Blackboard
The blackboard serves as the central communication center between components:

- Receives target and obstacle positions from their respective components.
- Sends target and obstacle positions to the drone and receives drone position updates and applied forces.
- Processes user input and forwards it to the drone component for position updates.

Additionally, the blackboard:
- Displays a real-time map of the environment, showing the drone, targets, and obstacles;
- Displays the score, the player's name, the difficulty of the level, the time remaining and the level;
- Computes an overall score based on  number of targets and difficulty.


The primitives of the blackboard are the following:
- File Manipulation: Functions like fopen(), fwrite(), and fclose() are used to open a log file, write messages, and close it.
- Process Management and Signal Handling:
    - writePid(), which retrieves the process ID using getpid().
    - sigaction() handles signals from the watchdog (WD), manages termination cleanup, and detects window resizing.
    - kill() sends a signal to the watchdog
    - sigset_t mask ensures safe signal handling with pselect() and sigfillset(&mask) blocks all signals during pselect().
- Interprocess Communication (IPC) via Pipes:
    - write() and read() facilitate data exchange between processes.
- UI Handling with Ncurses:
    - initscr(), start_color(), curs_set(0), noecho(), cbreak() configure input and display settings.
    - getmaxyx() retrieves terminal size for dynamic UI.
    - newwin(), box(), wrefresh(), mvwprintw(), and werase() manage windows and content rendering.

### Watchdog
The watchdog continuously monitors system activity, detecting inactivity and triggering alerts when no computations occur. If necessary, it can halt the system to maintain proper functionality. It periodically sends a SIGUSR1 signal to all monitored processes to verify their responsiveness. If a process fails to respond, the watchdog logs the issue and terminates the unresponsive process.

#### Passparam file
The passparam file is used from the various process to write their process id so that the watchdog is able to read them and send them a signal to check if they are still running.


The watchdog utilizes the following primitives:
- File Manipulation: Functions like fopen(), fwrite(), and fclose() are used to open a log file, write messages, and close it.
- Process Management and Signal Handling:
    - Kill(): used to send a signal to the WD to tell that it's alive
    - getpid(): used to get the process ID of the watchdog and write it on the passParam file.
    - Sigaction(): used to initialize the signal handler to handle the signal sent by the WD
- Interprocess Communication (IPC) via Pipes:
    - write() and read() facilitate data exchange between processes.

In addition to those, writeSecure() and readSecure() are custom functions designed for safe file operations. writeSecure() allows for writing or modifying a specific line in a file while ensuring that no two processes modify it simultaneously. Meanwhile, readSecure() reads a specific line while maintaining safe concurrent access.


### Input
The input handles user input and displays relevant information using the ncurses library, including:

- The leaderboard
- The drone's position and speed
- The forces acting on the drone

The starting leaderboard is read from the "appsettings.json" file and updated at the end of the game when the user decides to close the game.

Upon startup, the user selects:
- The player's name;
- A key configuration between the default one and a custom one;
- A difficulty level between easy and difficult.

In easy mode, the map remains static, while in hard mode, it dynamically changes every few seconds, awarding double points.

After the user has selected the options, the game starts, and the user can control the drone using the key configuration of its choice. The eight external keys can be used to move the drone by adding a force in the respective direction. On the other hand, the central key is used to instantly zero all the forces, in order to see the inertia on the drone.

In addition, they can choose to pause the game at any time by pressing the 'p' key, or to quit the game by pressing the 'q' key. Other keys pressed are ignored.

The input utilizes the following primitives:

- File Manipulation: fopen(), fwrite(), and fclose() to open, write, and close log files.
- Process Management & Signal Handling: sigaction() handles signals from the watchdog (WD), manages termination cleanup, and detects window resizing.
- Interprocess Communication (IPC) via Pipes: write() and read() facilitate data exchange between processes.
- UI Handling with Ncurses:
    - initscr(), start_color(), curs_set(0), noecho(), cbreak(), and nodelay() configure input and display settings.
    - getmaxyx() retrieves terminal size for dynamic UI.
    - newwin(), box(), wrefresh(), mvwprintw(), and werase() manage windows and content rendering.
- Configuration & JSON Parsing:
    - cJSON_Parse(), cJSON_GetObjectItemCaseSensitive(), cJSON_Print(), and cJSON_Delete() handle reading, modifying, and freeing configuration data.

In addition, writeSecure() and readSecure() ensure safe file operations. writeSecure() modifies a specific line while preventing concurrent writes, and readSecure() reads a line with safe shared access. Lastly, readInputMsg() and writeInputMsg() facilitate IPC via pipes by reading and writing the InputMessage structure, logging errors, and terminating on failure.


### Target and Obstacle
The target and obstacle processes function similarly, generating random positions within the environment while ensuring they are not too close to the drone. These positions are then sent to the blackboard for display and processing. Additionally, obstacles are strategically placed to avoid being too close to targets, allowing the drone to reach all the targets without any issues.

The target and obstacle utilize the following primitives:
- File Manipulation: Functions like fopen(), fwrite(), and fclose() are used to open a log file, write messages, and close it.
- Process Management and Signal Handling:
    - writePid(), which retrieves the process ID using getpid().
    - Sigaction(): used to initialize the signal handler to handle the signal sent by the watchdog
- Interprocess Communication (IPC) via Pipes:
    - write() and read() facilitate data exchange between processes.

In addition, readMsg() and writeMsg() facilitate IPC via pipes by reading and writing the Message structure, logging errors, and terminating on failure.


### Drone
The drone handles movement and interaction with targets and obstacles, using force-based navigation. The formula used to calculate the next position of the drone is the following:

$$ x_i = \frac {2m\cdot x_{i-1} + Tk \cdot x_{i-1} + F_x \cdot T^2 - m \cdot x_{i-2}}{m + Tk} $$

where:
- $m$ is the drone's mass
- $x_{i - 1}$ and $x_{i-2}$ are the drone's position respectivelly at istance $i-1$ and istance $i-2$
- $T$ is the period
- $k$ is the viscous costant
- $F_x$ is the sum of all the forces in that given direction

For the y coordinate the formula is the same. Analyzing how the total force acting on the drone was calculated, this is given by:
- User input, where each key pressed adjusts the force vector by increasing the corresponding force applied to the drone.
- Repulsive force from the obstacles;
- Attractive force from the targets.

Each obstacle and target inside a given radius $\rho_0$ from the drone will add a force:
$$ F = \frac {\eta}{\rho} \cdot \left(\frac{1}{\rho} - \frac{1}{\rho_0}  \right)^2$$
where $\rho$ is the distance between the single obstacle/target and the drone. The force is then divided into its components along x and y.

The drone utilizes the following primitives:
- File Manipulation: Functions like fopen(), fwrite(), and fclose() are used to open a log file, write messages, and close it.
- Process Management and Signal Handling:
    - writePid(), which retrieves the process ID using getpid().
    - Sigaction() used to initialize the signal handler to handle the signal sent by the watchdog
- Interprocess Communication (IPC) via Pipes:
    - write() and read() facilitate data exchange between processes.

In addition, readMsg() and writeMsg() facilitate IPC via pipes by reading and writing a Message structure, logging errors, and terminating on failure.

## Parameter Management
All configurable parameters are stored in the appsettings.json file and can be modified. These parameters include:

- Player Settings: the default player name, game difficulty (e.g. Easy, Difficult), and default key bindings, which can be customized at the start of the game.
- Game Progression: the initial level when starting a new game, the time allocated per level (in seconds) and the additional time per level as the game progresses.
- Environment Settings: Number of targets and obstacles, determining the density of entities in each level, and their incremental increase as the game progresses.
- Physics Parameters for the drone dynamics.
- Leaderboard: Player rankings, which are updated at the end of each game session.

## Logging
Logs are available to assist developers in debugging the project and to provide users with insights into the execution process. Each component generates its own log file, storing relevant information, all of which are located in the logs folder. 
The level of detail in the logs varies based on the project's build mode. In debug mode, more detailed information is recorded, while in release mode, logging is minimized. This behavior is controlled by the USE_DEBUG flag.
