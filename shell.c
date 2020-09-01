#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <readline/readline.h>

#define TRUE 1
#define FALSE 0

#define CMD_AFTER_PIPE 0
#define CMD_PIPE_PIPE 1
#define CMD_BEFORE_PIPE 2

#define READ_END 0
#define WRITE_END 1

//Memory limits
#define CMD_BUFFER_SIZE 100
#define ARGV_AMOUNT_OF_LINES 20
#define FILENAME_SIZE 20

int GLOBAL_PIPE[2];
int GLOBAL_PIPE2[2];
int ODD_PIPES = FALSE;

char** parseInput(char *input) {
  int i;
  char* cmd = (char*) malloc(CMD_BUFFER_SIZE*sizeof(char));

  char **argv = (char**) malloc(ARGV_AMOUNT_OF_LINES * sizeof(char *)); // Aloccates argv

  i = 0;
  cmd = strtok(input, " "); // Gets first command
  while(cmd != NULL) { // Fill argv with commands

    argv[i] = (char*) malloc(CMD_BUFFER_SIZE * sizeof(char));
    argv[i] = cmd;
    i++;

    cmd = strtok(NULL, " ");
  }

  argv[i] = NULL; // Terminates argv with NULL
  return argv;
}

void close_fd(int fd) {
  if(fd != 0 && fd != 1)
    close(fd);
}

void handleAfterPipeCall(char **argv, char* fileName) { // First command
  ODD_PIPES = TRUE;

  pid_t pid;
  pipe(GLOBAL_PIPE);

  pid = fork();

  if(pid == 0) { //Child process

    close_fd(GLOBAL_PIPE[READ_END]);
    dup2(GLOBAL_PIPE[WRITE_END], STDOUT_FILENO);

    if(fileName) {
      int fd = open(fileName, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
      dup2(fd, STDIN_FILENO);
      close(fd);
    }

    if(execvp(argv[0], argv) < 0)
      printf("Error during command execution\n");

  } else if(pid > 0) { // Main process

      close(GLOBAL_PIPE[WRITE_END]);

      int status;
      waitpid(pid, &status, 0);


  } else { // Fork error
      perror("fork()");
  }

}

void handlePipePipeCall(char **argv) { // Middle command

  pid_t pid;
  if(ODD_PIPES) { // Last pipe created is GLOBAL_PIPE
    pipe(GLOBAL_PIPE2);
  } else {
    pipe(GLOBAL_PIPE);
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

  } else { // Fork error
    perror("fork()");
  }

  ODD_PIPES = !ODD_PIPES;

}

void handleBeforePipeCall(char** argv, char* fileName, int append) { // Last command

  pid_t pid;
  pid = fork();

  if(pid == 0) { //Child process

    if(ODD_PIPES) { // Last pipe created is GLOBAL_PIPE
      dup2(GLOBAL_PIPE[READ_END], STDIN_FILENO);
    } else {
      dup2(GLOBAL_PIPE2[READ_END], STDIN_FILENO);
    }

    if(fileName) {

      int fd;
      if(append == TRUE) {
        fd = open(fileName, O_RDWR | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR);
      } else {
        fd = open(fileName, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
      }

      dup2(fd, STDOUT_FILENO);
      close(fd);
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

void handlePipeCall(char** argv, int pipeCount) {

  int i = 0; // Holds argv index
  int j = 0; // Holds cmd index
  int beforePipe = FALSE;
  char* fileNameOut = NULL;
  char* fileNameIn = NULL;
  int append = FALSE;

  char** cmd = (char**) malloc(ARGV_AMOUNT_OF_LINES * sizeof(char*));

  while(argv[i] != NULL) {

    if(argv[i][0] == '|') { // Found a pipe operator after the command
      cmd[j] = NULL;

      if(beforePipe == TRUE) // If there is a pipe before it, then it's a PipePipeCall
        handlePipePipeCall(cmd);
      else                   // If there is not a pipe before it, then it's a AfterPipeCall
        handleAfterPipeCall(cmd, fileNameIn);

      j = 0;
      beforePipe = TRUE;
    }
    else if(argv[i][0] == '>') { // Found a redirect stdout operator

        if(argv[i][1] == '>') {
        append = TRUE;
        fileNameOut = (char*) malloc(FILENAME_SIZE * sizeof(char));
        fileNameOut = argv[i+1];
        i += 2;
      } else {
        append = FALSE;
        fileNameOut = (char*) malloc(FILENAME_SIZE * sizeof(char));
        fileNameOut = argv[i+1];
        j += 2;
      }


    }
    else if(argv[i][0] == '<'){ // Found a redirect stdin operator
        fileNameIn = (char*) malloc(FILENAME_SIZE * sizeof(char));
        fileNameIn = argv[i+1];
        j += 2;
    }
    else { // Simple command
      cmd[j] = (char*) malloc(CMD_BUFFER_SIZE * sizeof(char*));
      cmd[j] = argv[i];
      j++;
    }

    i++;

  }

  //Last command is always a BeforePipeCall
  cmd[j] = NULL;
  handleBeforePipeCall(cmd,  fileNameOut, append);
}

void handleUnitCall(char** argv) {

  int i = 0; // Holds argv index
  int j = 0; // Holds cmd index
  char* fileNameIn = NULL;
  char* fileNameOut = NULL;
  int append = FALSE;
  char** cmd = (char**) malloc(ARGV_AMOUNT_OF_LINES*sizeof(char*));

  while(argv[i] != NULL) { // Takes fileNameIn and fileNameOut if present and builds the cmd
    if(argv[i][0] == '>'){

      if(argv[i][1] == '>') {
        append = TRUE;
        fileNameOut = (char*) malloc(FILENAME_SIZE * sizeof(char));
        fileNameOut = argv[i+1];
        i += 2;
      } else {
        append = FALSE;
        fileNameOut = (char*) malloc(FILENAME_SIZE * sizeof(char));
        fileNameOut = argv[i+1];
        i += 2;
      }

    } else if(argv[i][0] == '<') {
      fileNameIn = (char*) malloc(FILENAME_SIZE * sizeof(char));
      fileNameIn = argv[i+1];
      i += 2;
    } else { // Simple command or argument
      cmd[j] = (char*) malloc(CMD_BUFFER_SIZE * sizeof(char));
      cmd[j] = argv[i];
      i++;
      j++;
    }
  }

  cmd[j] = NULL;

  pid_t pid;
  pid = fork();

  if(pid == 0) { //Child process

    if(fileNameIn) {
     int fd = open(fileNameIn, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
     dup2(fd, STDIN_FILENO);
     close(fd);
    }
    if(fileNameOut) {
      int fd;
      if(append == TRUE) {
        fd = open(fileNameOut, O_RDWR | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR);
      } else {
        fd = open(fileNameOut, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
      }

     dup2(fd, STDOUT_FILENO);
     close(fd);
    }

    if(execvp(cmd[0], cmd) < 0)
      printf("Error during command execution\n");

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
  }

return 0;
}

