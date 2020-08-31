#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <readline/readline.h>

#define TRUE 1
#define FALSE 0

#define CMD_AFTER_PIPE 0
#define CMD_PIPE_PIPE 1
#define CMD_BEFORE_PIPE 2

#define READ_END 0
#define WRITE_END 1

int GLOBAL_PIPE[2];
int GLOBAL_PIPE2[2];
int ODD_PIPES = FALSE;

char **parseInput(char *input) {
  int argc = 1;
  int i;
  char * cmd;

  for(i=0;i<strlen(input);i++) // Counts total of commands
    if(isspace(input[i]) != 0)
      argc++;

  char **argv = (char **) malloc(argc * sizeof(char *)); // Aloccates argv

  i = 0;
  cmd = strtok(input, " "); // Gets first command
  while(cmd != NULL) { // Fill argv with commands

    argv[i] = (char*) malloc(strlen(cmd) * sizeof(char));
    argv[i] = cmd;
    i++;

    cmd = strtok(NULL, " ");
  }

  free(cmd);

  argv[i] = NULL; // Terminates argv with NULL
  return argv;
}

void close_fd(int fd) {
  if(fd != 0 && fd != 1)
    close(fd);
}

void handleAfterPipeCall(char **argv) { // First command
  printf("Processing first command\n");

  ODD_PIPES = TRUE;

  pid_t pid;
  pipe(GLOBAL_PIPE);
  printf("Creating pipe 1\n");
  pid = fork();

  if(pid == 0) { //Child process

    close_fd(GLOBAL_PIPE[READ_END]);
    dup2(GLOBAL_PIPE[WRITE_END], STDOUT_FILENO);

    if(execvp(argv[0], argv) < 0)
      printf("Error during command execution\n");

  } else if(pid > 0) { // Main process

      close(GLOBAL_PIPE[WRITE_END]);

      int status;
      waitpid(pid, &status, 0);

      printf("First command finished: Wrote output on pipe1\n");

  } else { // Fork error
      perror("fork()");
  }
}

void handlePipePipeCall(char **argv) { // Middle command
  printf("Processing pipe pipe call\n");

  pid_t pid;
  if(ODD_PIPES) { // Last pipe created is GLOBAL_PIPE
    pipe(GLOBAL_PIPE2);
    printf("Creating pipe2\n");
  } else {
    pipe(GLOBAL_PIPE);
    printf("Creating pipe1\n");
  }

  pid = fork();
  if(pid == 0) { // Child Process

    if(ODD_PIPES) { // Last pipe created is GLOBAL_PIPE
      dup2(GLOBAL_PIPE[READ_END], STDIN_FILENO);
      dup2(GLOBAL_PIPE2[WRITE_END], STDOUT_FILENO);
    } else { // Last pipe created is GLOBAL_PIPE2
      dup2(GLOBAL_PIPE2[READ_END], STDIN_FILENO);
      dup2(GLOBAL_PIPE[WRITE_END], STDOUT_FILENO);
    }


    if(execvp(argv[0], argv) < 0)
      printf("Error during command execution\n");

  } else if(pid > 0) { // Main Process

    if(ODD_PIPES) { // Last pipe created is GLOBAL_PIPE
      close_fd(GLOBAL_PIPE[READ_END]);
      close_fd(GLOBAL_PIPE2[WRITE_END]);
    } else {
      close_fd(GLOBAL_PIPE2[READ_END]);
      close_fd(GLOBAL_PIPE[WRITE_END]);
    }

    int status;
    waitpid(pid, &status, 0);

    if(ODD_PIPES)
      printf("Middle command finished: Read input from pipe1 and wrote output on pipe2\n");
    else
      printf("Middle command finished: Read input from pipe2 and wrote output on pipe1\n");

  } else { // Fork error
    perror("fork()");
  }

  ODD_PIPES = !ODD_PIPES;

}

void handleBeforePipeCall(char **argv) { // Last command

  printf("Processing last command\n");

  pid_t pid;
  pid = fork();

  if(pid == 0) { //Child process

    if(ODD_PIPES) { // Last pipe created is GLOBAL_PIPE
      dup2(GLOBAL_PIPE[READ_END], STDIN_FILENO);
    } else {
      dup2(GLOBAL_PIPE2[READ_END], STDIN_FILENO);
    }


    if(execvp(argv[0], argv) < 0)
      printf("Error during command execution\n");

  } else if(pid > 0) { // Main process

      if(ODD_PIPES) { // Last pipe created is GLOBAL_PIPE
        close_fd(GLOBAL_PIPE[READ_END]);
      } else {
        close_fd(GLOBAL_PIPE2[READ_END]);
      }

      int status;
      waitpid(pid, &status, 0);

    if(ODD_PIPES)
      printf("Last command finished: Read input from pipe1\n");
    else
      printf("Last command finished: Read input from pipe2\n");

  } else { // Fork error
      perror("fork()");
  }

}


int charCounter(char* str, char character) {
  int i;
  int total = 0;


  for(i=0;i<strlen(str);i++) {
    if(str[i] == '|')
      total++;
  }

  return total;
}

int * getPipeIndexes(char **argv) {
  int pipeCount = 0;
  int * pipeIndexes = NULL;
  int i = 0; // Holds argv pointer
  int j = 0; // Holds pipeIndexes pointer

  while(argv[i] != NULL) {

    if(argv[i][0] == '|') {
      pipeCount++;
      pipeIndexes = (int *) realloc(pipeIndexes, (pipeCount * sizeof(int)));
      pipeIndexes[j] = i;

      j++;
    }

    i++;
  }

  return pipeIndexes;
}

void handlePipeCall(char** argv, int pipeCount) {

  int i = 0; // Holds argv index
  int j = 0; // Holds cmd index
  int beforePipe = FALSE;

  char** cmd = (char**) malloc(1024*sizeof(char));

  while(argv[i] != NULL) {

    if(argv[i][0] == '|') { // Found a pipe after the command
      cmd[j] = NULL;

      if(beforePipe == TRUE) // If there is a pipe before it, then it's a PipePipeCall
        handlePipePipeCall(cmd);
      else                   // If there is not a pipe before it, then it's a AfterPipeCall
        handleAfterPipeCall(cmd);

      j = 0;
      beforePipe = TRUE;
    } else {
        cmd[j] = argv[i];
        j++;
    }

    i++;

  }

  //Last command is always a BeforePipeCall
  cmd[j] = NULL;
  handleBeforePipeCall(cmd);

}

void handleUnitCall(char** argv) {

  pid_t pid;
  pid = fork();

  if(pid == 0) { //Child process

    if(execvp(argv[0], argv) < 0)
      printf("Error during command execution");

  } else if(pid > 0) { // Main process

      int status;
      waitpid(pid, &status, 0);

  } else { // Fork error
      perror("fork()");
  }

}

int main() {
  char *input;
  char **argv;
  int pipeCount;

	while(1) { // Shell loop

    input = readline("\033[34mshellzada >> \033[0m"); // Reads user input
    pipeCount = charCounter(input, '|'); // Gets total of pipes

    argv = parseInput(input); // Parses user input and creates argv

    if(pipeCount > 0)
      handlePipeCall(argv, pipeCount); // Handle commands with pipes
    else
      handleUnitCall(argv); // Handle unit command

    free(input);
    free(argv);

  }

return 0;
}

