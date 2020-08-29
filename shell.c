#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <readline/readline.h>


#define CWD_BUFSIZE 1024

char * getCwd() {
  char *cwd = (char*) malloc(CWD_BUFSIZE * sizeof(char));
  return getcwd(cwd, CWD_BUFSIZE);
}


char **parseInput(char *input) {
  int  argc = 1;
  int i;
  char *cmd;

  for(i=0;i<strlen(input);i++) // Counts total of commands
    if(isspace(input[i]) != 0)
      argc++;

  char **argv = (char **) malloc(argc * sizeof(char *)); // Aloccates argv

  i = 0;
  cmd = strtok(input, " "); // Gets first command
  while(cmd != NULL) { // Fill argv with commands

    argv[i] = cmd;
    i++;

    cmd = strtok(NULL, " ");
  }

  argv[i] = NULL; // Terminates argv with NULL
  return argv;
}

void execCmd(int argc, char **argv){

}


int main() {
  char *input;
  char **argv;

  pid_t pid;

	while(1) { // Shell loop

    input = readline(strcat(getCwd(), ">> ")); // Reads user input
    argv = parseInput(input); // Parses input and creates argv

    pid = fork();

    if(pid == 0) { // Child process

      execvp(argv[0], argv);

    }
    else if(pid > 0) { // Main process
      int status;
      waitpid(pid, &status, 0);

    }
    else { // Erro
      perror("fork()");
      return -1;
    }

  }


return 0;
}
