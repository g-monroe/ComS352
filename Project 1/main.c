/*
    @Program: Project 1 | Unix Shell
    @Author: Gavin Monroe
    @Class: ComS 352
    @Assignment: Programming Assignment 1
    @Description: Creating a program that runs a child that will returning the result of the command entered.
    More Details can be found in the Assignment Porgramming Project Page 204 of "Operating Sysetm Concepts",
    10th Edition.

*/
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <limits.h>
#include <errno.h>

//Definitions
#define SIZE 10 //Max History Size
#define MAX 10 //Max Command Size
#define NOCMD "No command in history.\n" //Common Output MSG

//Global Variables
static int numHist = 0; //Number in History
static char *history[SIZE]; //History

/*
    @Type: static void
    @Function: execCommand
    @Author: Gavin Monroe
    @Description: Executes a command in another child process and returns result.
    @Parameters: const char pointer (cmd), ;
*/
static void execCommand(const char *line){
    char *CMD = strdup(line);//Copy String.
    char *params[10]; //Parameters
    int argc = 0; //Number of parameters

    //Split string into all of its parameters
    params[argc++] = strtok(CMD, " ");
    while(params[argc-1] != NULL){
        params[argc++] = strtok(NULL, " ");
    }
    argc--;

    //Determine to run in background.
    int back = 0;
    if(strcmp(params[argc-1], "&") == 0){
        back = 1;
        params[--argc] = NULL;
    }

    int fd[2] = {-1, -1};
    while(argc >= 3){
        if(strcmp(params[argc-2], ">") == 0){ //Output to file.
            fd[1] = open(params[argc-1], O_CREAT|O_WRONLY|O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP|S_IWGRP);
            if(fd[1] == -1){
                perror("open");
                free(CMD);
                return;
            }
            params[argc-2] = NULL;
            argc -= 2;
        }else if(strcmp(params[argc-2], "<") == 0){//Input from file.
            fd[0] = open(params[argc-1], O_RDONLY);
            if(fd[0] == -1){
                perror("open");
                free(CMD);
                return;
            }
            params[argc-2] = NULL;
            argc -= 2;
        }else{//Edge Case.
            break;
        }
    }

    int status;
    pid_t pid = fork();
    switch(pid){
        case -1://Bad PID   
            perror("fork");
            break;
        case 0://Child Process
            if(fd[0] != -1){
                if(dup2(fd[0], STDIN_FILENO) != STDIN_FILENO){
                    perror("dup2");
                    exit(1);
                }
            }
            if(fd[1] != -1){
                if(dup2(fd[1], STDOUT_FILENO) != STDOUT_FILENO){
                    perror("dup2");
                    exit(1);
                }
            }
            execvp(params[0], params);
            perror("execvp");
            exit(0);
        default: //Parent Process
            
            close(fd[0]);
            close(fd[1]);
            
            if(!back){
                waitpid(pid, &status, 0);
            }
            break;
    }
    free(CMD);
}
/*
    @Type: static void
    @Function: addHistory
    @Author: Gavin Monroe
    @Description: Adds command into History.
    @Parameters: const char pointer (cmd), ;
*/
static void addHistory(const char *cmd){
    //Adds Command to history.
    if(numHist == (SIZE-1)){
        int i;
        free(history[0]);
        for(i=1; i < numHist; i++){
            history[i-1] = history[i];
        }
        numHist--;
    }
    //Add Command
    history[numHist++] = strdup(cmd);
}

/*
    @Type: static void
    @Function: cmdHistory
    @Author: Gavin Monroe
    @Description: Runs command if found in history.
    @Parameters: const char pointer (cmd), ;
*/
static void cmdHistory(const char *cmd){
    //This runs the command that is currently in history.
    int i = 0;
    if(numHist == 0){//Would make a MACRO for this in CPP
        printf(NOCMD);
        return ;
    }
    if(cmd[1] == '!'){ //Run Latest Command.
        i = numHist-1;
    }else{//Find command 
        i = atoi(&cmd[1]) - 1;//Command Index
        if((i < 0) || (i > numHist)){//Couldn't Find Command
            fprintf(stderr, "No command in found history.\n");
            return;
        }
    }

    //Run Command.
    printf("%s\n", cmd);
    execCommand(history[i]);
}

static void returnHistory(){
    //Check if commands exist in history.
    if (numHist == 0){//Would make a MACRO for this in CPP
        printf(NOCMD);
        return;
    }

    int i;
    for(i=numHist-1; i >=0 ; i--){ //Show latest one at the top.
        printf("%i %s\n", i+1, history[i]);
    }
}

static void signalHandler(const int pid){
    //Function handles signal from running console.
    switch(pid){
        case SIGTERM://Signal Terminated.
        case SIGINT://Signal Sent.
            break;
        case SIGCHLD://Wait for response from child process stops
            while (waitpid(-1, NULL, WNOHANG) > 0);
            break;
    }

}
int main(int argc, char *argv[]){
    struct sigaction act, act_old;
    act.sa_handler = signalHandler;
    act.sa_flags = 0;
    sigemptyset(&act.sa_mask);
    if((sigaction(SIGINT, &act, &act_old) == -1) || (sigaction(SIGCHLD, &act, &act_old) == -1)){
        perror("signal");
        return 1;
    }

    size_t line_size = 100;
    char * line = (char*) malloc(sizeof(char)*line_size);
    if(line == NULL){
            perror("malloc");
            return 1;
    }

    int inter = 0;
    while(1){
        if(!inter)
            printf("osh> ");
        if(getline(&line, &line_size, stdin) == -1){   
            if(errno == EINTR){
                clearerr(stdin);
                inter = 1;   
                continue;
            }
            perror("getline");
            break;
        }

        inter = 0;
        int size = strlen(line); //Get length of entered line.
        if(size == 1){
            continue;
        }
        line[size-1] = '\0';//End line with 0 byte making sure that the program knows its the end.

        if(strcmp(line, "exit") == 0){
            break;
        }else if(strcmp(line, "history") == 0){
            returnHistory();
        }else if(strcmp(line, "!!") == 0){
            cmdHistory(line);
        }else{
            addHistory(line);
            execCommand(line);
        }
    }
    free(line);
    return 0;
}
