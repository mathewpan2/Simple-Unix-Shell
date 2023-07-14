#include <stdio.h> 
#include <unistd.h>
#include <limits.h>
#include <libgen.h>
#include <string.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <dirent.h>

//add error checks 

char* pidName[102];
char* argv[100]; 
int r_output[100] = {-1};
int r_input[100] = {-1}; 
int error = 0;
int r_append = 0; 
int jobs[102] = {-1};
int pid; 
int pid_index = 1; 
int pipeloc[100] = {-1};
int pipecount = 0;

char* pipe1[20];
char* pipe2[20];




void handle_sig() {}

void handle_sigint() {

}


void redirectOutput(char ** argv, int multi, int index) {
    //opens up a file descriptor, duplicate that file descriptor to stdout, the I run for example echo hello, which will print hello to the stdout, which is now the file 


    //only handles one output for now 
    fflush(stdout);

    //creates new file descriptor 
    int fd; 
    if(r_append) {
            fd = open(argv[r_output[index] + 1] ,O_WRONLY|O_CREAT|O_APPEND,S_IRUSR|S_IWUSR);
        }
        else {
            fd = open(argv[r_output[index] + 1] ,O_CREAT|O_WRONLY|O_TRUNC,S_IRUSR|S_IWUSR);
        }

    
    if (fd == -1 ) {
        perror("Error opening file"); 
        exit(0);
    }

    dup2(fd, STDOUT_FILENO); 
    close(fd);

    if(!multi) {
        argv[r_output[index]] = NULL; 
        execvp(argv[0], argv);
    }
    
}

void reidrectInput(char ** argv, int multi, int index) {
    int fd = open(argv[r_input[index] + 1], O_RDONLY);
    if (fd == -1 ) {
        fprintf(stderr, "Error: invalid file\n");
        fflush(stderr);
        exit(0);
    }

    dup2(fd, STDIN_FILENO);
    close(fd); 


    if(!multi) {
        argv[r_input[index]] = NULL; 
        execvp(argv[0], argv);
    }
}

void multipleRedirect(char ** argv, int index) {
   
        if(r_input[index] < r_output[index]) {
            //if input is first
            reidrectInput(argv, 1, index);
            redirectOutput(argv, 1, index);
            argv[r_input[index]] = NULL; 
            execvp(argv[0], argv);
        }
        else {
            redirectOutput(argv, 1, index);
            reidrectInput(argv, 1, index);
            argv[r_output[index]] = NULL; 
            execvp(argv[0], argv);
        }
    

}


void printPrompt() {
    char path[PATH_MAX];

    if(getcwd(path, sizeof(path)) != NULL) {
        printf("[nyush %s]$ ", basename(path));
    }
}

int commandParser(char* input) {

    char *token; 
    char *rest = input; 
    int size = 0; 
    int i_output = 0, i_input = 0;
    
    while((token = strtok_r(rest, " ", &rest))) {


         if(strcmp(token, "|") == 0) {
            pipeloc[pipecount++] = size;
            i_input++;
            i_output++;
            if(size == 0) {
                fprintf(stderr, "Error: invalid command\n");
                error = 1; 
            }    

        }

        if(strcmp(token, ">") == 0) {
            r_output[i_output] = size; 
            if(size == 0) {
                fprintf(stderr, "Error: invalid command\n");
                error = 1; 
            }
        }

        if(strcmp(token, "<") == 0) {
            r_input[i_input] = size; 
            if(size == 0) {
                fprintf(stderr, "Error: invalid command\n");
                error = 1; 
            }
        }

        if(strcmp(token, ">>") == 0) {
            r_output[i_output] = size; 
            r_append = 1;
            if(size == 0) {
                fprintf(stderr, "Error: invalid command\n");
                error = 1; 
            }
        }
       
        argv[size] = token; 
        size++; 
    }

    argv[size] = NULL; 


    if(r_input[0] > 0) {
        if(argv[r_input[0] + 1] == NULL) {
            fprintf(stderr, "Error: invalid command\n");
            error = 1; 
        }
    }

     if(r_output[0] > 0) {
        if(argv[r_output[0] + 1] == NULL) {
            fprintf(stderr, "Error: invalid command\n");
            error = 1; 
        }
    }

    if(pipeloc[0] > 0) {
       if(argv[pipeloc[0] + 1] == NULL) {
            fprintf(stderr, "Error: invalid command\n");
            error = 1; 
        } 
    }



    return size; 

}

int main() {


  
    signal(SIGINT, handle_sig);
    signal(SIGQUIT, handle_sig);
    signal(SIGTSTP, handle_sig);
    
    char input[100];
    int argsize;
    int status; 
    
    while(1) {
        int fd[2][2 ];

        printPrompt();
        fflush(stdout);
        //ctrl + d 
       if(fgets(input, 100, stdin) == NULL) {
        exit(0);
       } 

        if(input[0] == '\n') {
            continue; 
        }
       
        input[strlen(input) - 1] = '\0';
        fflush(stdin);

        argsize = commandParser(input);

        if(pipecount == 1) {
            int m = pipeloc[0] + 1; 
                int i = 0;
                while(argv[m] != NULL) {
                    pipe1[i++] = argv[m++];
                }
                pipe1[i + 1] = NULL;
        }
        
            
         if(pipecount > 1) {
                int m = pipeloc[0] + 1; 
                int i = 0;
                while(strcmp(argv[m], "|") != 0 && argv[m] != NULL) {
                    pipe1[i++] = argv[m++];
                }
                pipe1[i + 1] = NULL;

                     i = 0;
                     m = pipeloc[1] + 1; 
                while(argv[m] != NULL) {
                    pipe2[i++] = argv[m++];
                }

                pipe2[i + 1] = NULL;

            }  
        
        if(error > 0) {
            error = 0;
            r_output[0] = 0; 
            r_input[0] = 0;
            r_append = 0; 
            pipeloc[0] = 0; 
            continue;
        }

        if(strcmp(argv[0],"cd") == 0 ) {

            if(argsize > 2 || argsize == 1) {
                    fprintf(stderr, "Error: invalid command\n");
                    fflush(stderr);
                    continue;
                }

            //checks if directory exsists
            DIR *dir = opendir(argv[1]);
            if(dir == NULL ) {
                fprintf(stderr, "Error: invalid directory\n");
                fflush(stderr);
                closedir(dir);
                continue; 
            }
            //change directory command 
            chdir(argv[1]);
            continue;
            
            }

        if(strcmp(argv[0],"exit") == 0 ) {
                if(argsize > 1) {
                fprintf(stderr, "Error: invalid command\n");
                fflush(stderr);
                continue;
                }

                if(pid_index != 1) {
                    fprintf(stderr, "Error: there are suspended jobs\n");
                    continue;
                }

                exit(0); 
            }

            if(strcmp(argv[0], "fg") == 0) {
                int jobtokill = atoi(argv[1]);

                if(jobs[jobtokill] == -1) {
                    fprintf(stderr, "Error: invalid job\n");
                    fflush(stderr);
                    continue;
                }
                kill(jobs[jobtokill], SIGCONT);
                waitpid(jobs[jobtokill], &status, WUNTRACED);

            if (WIFEXITED(status)) {
                if(jobtokill == pid_index) {
                    pid_index--;
                    continue; 
                }

                else {
                    int i = jobtokill;
                    while(jobs[i + 1] != -1) {
                    strcpy(pidName[i], pidName[i + 1]);
                    jobs[i] = jobs[i + 1];
                    i++; 
                    }
                    free(pidName[i]);
                    pidName[i] = NULL;
                    jobs[i] = -1;
 
                } 

                pid_index--;
                continue;
            }

             if (WIFSTOPPED(status)) {

                //temp jobs stored at loc 101
                jobs[101] = jobs[jobtokill];
                pidName[101] = malloc(50);
                strcpy(pidName[101], pidName[jobtokill]); 


                 if(jobtokill == pid_index) {
                    pid_index--;
                    continue; 
                }

                else {
                    int i = jobtokill;
                    while(jobs[i + 1] != -1) {
                    strcpy(pidName[i], pidName[i + 1]);
                    jobs[i] = jobs[i + 1];
                    i++; 
                    }
                    jobs[i] = -1;
                } 

                jobs[pid_index - 1] = jobs[101]; 
                strcpy(pidName[pid_index - 1], pidName[101]);
                free(pidName[101]); 
                pidName[101] = NULL;
                continue;
            }

            continue;
        }
            if(strcmp(argv[0], "jobs") == 0) {
                int i = 1; 
                while(jobs[i] != -1) {
                    printf("[%d] %s\n", i, pidName[i]); 
                    i++;
                }
            }
            //if there is a pipe 
            if(pipecount > 0) { 
                for(int i = 0; i < pipecount; i++) {
                    pipe(fd[i]);
                }
                
            }

        fflush(stdout);
        //fix this, save the pid in a better place (prob a temp storage then move to array, more permanent storage)
        pid = fork(); 

        if (pid == 0) {
            //child process
            signal(SIGINT, SIG_DFL);
            signal(SIGQUIT, SIG_DFL);
            signal(SIGTSTP, SIG_DFL);

            if(pipecount > 0) {
                dup2(fd[0][1], STDOUT_FILENO); 
                close(fd[0][0]);
                close(fd[0][1]);
                if(pipecount == 2) {
                     close(fd[1][0]);
                     close(fd[1][1]);
                }
                argv[pipeloc[0]] = NULL; 
            }

            if(r_output[0] > 0 && r_input[0] > 0) {
                multipleRedirect(argv, 0);
            }

            if(r_output[0] > 0) {
            //going to try and direct the output stream, and then we run execvp to see if it works 
                redirectOutput(argv, 0, 0);
            }
            
            if(r_input[0] > 0) {
                reidrectInput(argv, 0, 0);
            }

            char root[99] = "/usr/bin/";
            strcat(root, argv[0]);
          
            if(strchr(argv[0], '/') == NULL) {
                 if(execv(root, argv) == -1) {
                     fprintf(stderr, "Error: invalid program\n");
                     fflush(stderr);
                     exit(0);
            }
            }

             if(execvp(argv[0], argv) == -1) {
                fprintf(stderr, "Error: invalid program\n");
                fflush(stderr);
                exit(0);
            }
           
        }

        else {
            int pid2 = -1;
            int pid3 = -1; 
            //parent process
            if(pipecount > 0) {
                    fflush(stdout);
                    pid2 = fork(); 
                  
                    if(pid2 == 0) {
                        dup2(fd[0][0], STDIN_FILENO);
                        close(fd[0][0]);
                        close(fd[0][1]);
                      
                
                        if(r_output[1] > 0 && r_input[1] > 0) {
                        multipleRedirect(pipe1, 1);
                         }

                        if(r_output[1] > 0) {
                            redirectOutput(argv, 1, 1);
                            int i = 0;
                            while(pipe1[i] != NULL) {
                                if(strcmp(pipe1[i], ">") == 0) {
                                    break; 
                                }
                                i++; 
                            }
                            pipe1[i] = NULL; 
                        }
            
                        if(r_input[1] > 0) {
                            reidrectInput(argv, 1, 1);
                            int i = 0;
                            while(pipe1[i] != NULL) {
                                if(strcmp(pipe1[i], "<") == 0) {
                                    break; 
                                }
                                i++; 
                            }
                           pipe1[i] = NULL;
                        } 

                        if(pipecount == 2) {
                            close(fd[1][0]);
                            dup2(fd[1][1], STDOUT_FILENO);
                            close(fd[1][1]);                 
                        }

                        execvp(pipe1[0], pipe1);
                    }

                    else{
                        if(pipecount == 2) {
                            fflush(stdout);
                        pid3 = fork(); 

                        if(pid3 == 0) {
                            dup2(fd[1][0], STDIN_FILENO);
                            close(fd[1][0]);
                            close(fd[0][1]);
                            close(fd[0][0]);
                            close(fd[1][1]);
                    
                            if(r_output[2] > 0) {
                        //going to try and direct the output stream, and then we run execvp to see if it works 
                            redirectOutput(argv, 1, 2);
                            int i = 0;
                            while(pipe2[i] != NULL) {
                                if(strcmp(pipe2[i], ">") == 0) {
                                    break; 
                                }
                                i++; 
                            }
                            pipe2[i] = NULL; 
                        }
            
                        if(r_input[2] > 0) {
                            reidrectInput(argv, 1, 2);
                            int i = 0;
                            while(pipe2[i] != NULL) {
                                if(strcmp(pipe2[i], "<") == 0) {
                                    break; 
                                }
                                i++; 
                            }
                           pipe2[i] = NULL;
                        }

                         execvp(pipe2[0], pipe2);
                        }
                        }
                        
                    }
            }

                close(fd[0][0]);
                close(fd[0][1]);
                 if(pipecount == 2) {
                     close(fd[1][0]);
                     close(fd[1][1]);
                }
                waitpid(pid, &status, WUNTRACED);
                if(pid2 != -1) {
                waitpid(pid2, NULL, 0);
                }

                if(pid3 != -1) {
                    waitpid(pid3, NULL, 0);
                }
                
            if (WIFSTOPPED(status)) {
            // child process suspended
            jobs[pid_index] = pid; 
            pidName[pid_index] = malloc(50);

            char str[100] = "";


        int i = 0; 
        char *temp;
        while(argv[i] != NULL) {
            temp = argv[i];
            strcat(str, temp);

            if(argv[i + 1] != NULL) {
                strcat(str, " ");
            }
            i++; 
        }
    
            strcpy(pidName[pid_index], str);
            pid_index++; 
            jobs[pid_index] = -1; 
        }
           
            for(int i = 0; i < 3; i++) {
                r_input[i] = 0;
                r_output[i] = 0;
            }
            r_append = 0; 
            pipecount = 0;


        }


        
    }


    return 1; 
}

//https://www.youtube.com/watch?v=cex9XrZCU14&list=PLfqABt5AS4FkW5mOn2Tn9ZZLLDwA3kZUY
//this playlist was very helpful 

