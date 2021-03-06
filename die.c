#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define BAD_ARGUMENTS         1
#define ILLEGAL_CHARACTER     2
#define ILLEGAL_ORDER         3
#define UNABLE_TO_EXECUTE_PS  4
#define COULD_NOT_FIND_PID    5
#define ABORT_SUCCESS         6
#define ABORT_FAILURE         7

#define DEBUG 0

int print_error_message(int code);

int main (int argc, char *argv[]) {
  if (argc != 2) {
    return print_error_message(BAD_ARGUMENTS);
  }

  if (!strcmp(argv[1], "abort")) {
    int abortStatus;
    printf("Aborting!\n");
    #ifdef __WIN32
    abortStatus = system("shutdown -a");
    #elif defined __unix__
    abortStatus = system("shutdown -c");
    #else
    char buffer[500];
    FILE* fp;

    // we want to parse the output of our command; need to treat it as a file
    fp = popen("sudo ps aux | grep \"shutdown\"", "r");
    if (fp == NULL) {
      return print_error_message(UNABLE_TO_EXECUTE_PS);
    }

    int pid, scan_success;
    while (fgets(buffer, sizeof(buffer)-1, fp) !=  NULL) {
      if (strlen(buffer) > 4 && strncmp("root", buffer, 4) == 0) {
        scan_success = sscanf(&buffer[4], "%d", &pid);
        break;
      }
    }

    if (scan_success != 1 || pid <= 0) {
      return print_error_message(COULD_NOT_FIND_PID);
    }

    sprintf(buffer, "sudo kill %d", pid);

    abortStatus = system(buffer);
    #endif

    if (abortStatus != 0) {
      return print_error_message(ABORT_FAILURE);
    }
    print_error_message(ABORT_SUCCESS);
    return 0;
  }

  int hours = 0, minutes = 0, seconds = 0;
  char h,m,s;

  // count the number of chars passed, parse accordingly
  int charsInString = sscanf (argv[1], "%*d%c%*d%c%*d%c", &h,&m,&s);

  if (charsInString == 3) {
    // they have been passed in the correct order, assign them to their values
    if (h == 'h' && m == 'm' && s == 's') {
      sscanf (argv[1], "%d%*c%d%*c%d%*c", &hours, &minutes, &seconds);
    }
    else {
      return print_error_message(ILLEGAL_ORDER);
    }
  }
  else if (charsInString == 2) {
    // three cases: {hours and minutes}, {minutes and seconds}, and {hours and
    // seconds}
    if (h == 'h' && m == 'm') {
      sscanf (argv[1], "%d%*c%d%*c", &hours, &minutes);
    }
    else if (h == 'm' && m == 's') {
      sscanf (argv[1], "%d%*c%d%*c", &minutes, &seconds);
    }
    else if (h == 'h' && m == 's') {
      sscanf (argv[1], "%d%*c%d%*c", &hours, &seconds);
    }
    // well, they also may have used illegal chars
    else {
      return print_error_message(ILLEGAL_CHARACTER);
    }
  }
  else if (charsInString == 1) {
    // three cases: {hours}, {minutes}, and {seconds}
    if (h == 'h') {
      sscanf (argv[1], "%d%*c", &hours);
    }
    else if (h == 'm') {
      sscanf (argv[1], "%d%*c", &minutes);
    }
    else if (h == 's') {
      sscanf (argv[1], "%d%*c", &seconds);
    }
    else {
      return print_error_message(ILLEGAL_CHARACTER);
    }
  }
  else {
    return print_error_message(ILLEGAL_ORDER);
  }

  printf ("shutting down in");

  if (hours) {
    printf("\n\t%d hour%s", hours, hours == 1 ? "" : "s");
  }
  if (minutes) {
    printf("\n\t%d minute%s", minutes, minutes == 1 ? "" : "s");
  }
  if (seconds) {
    printf("\n\t%d second%s", seconds, seconds == 1 ? "" : "s");

  }
  printf("\n");

  char number[10];
  char command[30];

  // convert the integer number into a string and append it to "shutdown -s -t"
  #ifdef __WIN32
  sprintf(number, "%d", (int)(hours*60*60 + minutes*60 + seconds));
  #else
  int shutdown_time = (int)(hours*60 + minutes + seconds/60);
  if (shutdown_time == 0) {
    printf("Unix doesn't support seconds - reverting to 1 minute.\n");
    shutdown_time = 1;
  }
  sprintf(number, "%d", shutdown_time);
  #endif
  #ifdef __WIN32
  strcpy (command, "shutdown -s -t ");
  #elif defined __unix__
  strcpy(command, "sudo shutdown -H +");
  #else
  strcpy(command, "sudo shutdown -h +");
  #endif

  strcat(command, number);

  if (DEBUG) {
    printf("%s\n", command);
  }
  else {
    #ifdef __unix__
    int fork_id;

    fork_id = fork();
    if (fork_id == 0) {
      setsid();
      FILE* p = popen(command, "r");
      fclose(p);
      return 0;
    }
    else {
      int lastExitCode;
      while(fork_id != wait(&lastExitCode))
        continue;
      return 0;
    }
    #else
    system(command);
    #endif
  }

  return 0;
}

int print_error_message(int code) {
  switch (code) {
    case BAD_ARGUMENTS:
      printf("Error - must pass one argument.\n\tex:\tdie 40m\n");
      break;
    case ILLEGAL_CHARACTER:
      printf("Error - illegal character. Must use h, m, and/or s.\n");
      break;
    case ILLEGAL_ORDER:
      printf("Error - must format in decreasing order (h, m, then s) preceded "
          "by a number,\n\tex:\tdie 1h30m40s\n\t\tdie 30h25s\n\t\tdie 40s\n");
      break;
    case UNABLE_TO_EXECUTE_PS:
      printf("Error - unable to execute ps; could not kill shutdown.\n");
      break;
    case COULD_NOT_FIND_PID:
      printf("Error - could not find shutdown pid.\n");
      break;
    case ABORT_SUCCESS:
      printf("Successfully aborted shutdown.\n");
      break;
    case ABORT_FAILURE:
      printf("Failed to abort shutdown!\n");
      break;
    default:
      printf("Error!");
  }

  return code;
}
