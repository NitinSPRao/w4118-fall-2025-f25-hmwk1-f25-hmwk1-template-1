#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>

#define MAX_ARGS 128
static int tokenize(char *line, char *argv[], int max)
{
    int argc = 0;

    while (*line == ' ') line++;

    while (*line != '\0')
    {
        if (argc >= max - 1) {
            fprintf(stderr, "error: too many arguments (max %d)\n", max - 1);
            return -1;
        }
        argv[argc++] = line;

        while (*line != '\0' && *line != ' ') line++;

        if (*line == '\0')
            break;

        *line++ = '\0';

        while (*line == ' ') line++;
    }

    argv[argc] = NULL;
    return argc;
}

static int handle_builtin(char *argv[])
{
    if (argv[0] == NULL) return 1;

    if (strcmp(argv[0], "exit") == 0) {
        return 0;
    }

    if(strcmp(argv[0], "cd") == 0) {
        if (argv[1] == NULL || argv[2] != NULL) {
            fprintf(stderr, "error: %s\n", "usage: cd <dir>");
        } else if (chdir(argv[1]) == -1) {
            fprintf(stderr, "error: %s\n", strerror(errno));
        }
        return 1;
    }
    
    return 2;
}

static void install_signal_handlers(void) {
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = SIG_IGN;
    sigaction(SIGINT, &sa, NULL);
}

static void run_external(char *argv[]) {
    pid_t pid = fork();
    if (pid < 0) {
        fprintf(stderr, "error: %s\n", strerror(errno));
        return;
    } else if (pid == 0) {
        struct sigaction sa_ch;
        memset(&sa_ch, 0, sizeof(sa_ch));
        sa_ch.sa_handler = SIG_DFL;
        sigaction(SIGINT, &sa_ch, NULL);

        execv(argv[0], argv);
        fprintf(stderr, "error: %s\n", strerror(errno));
        _exit(127);
    } else {
        int status;
        if (waitpid(pid, &status, 0) == -1) {
            fprintf(stderr, "error: %s\n", strerror(errno));
        }
    }
}

int main(void)
{
    install_signal_handlers();

    char   *line = NULL;
    size_t  cap  = 0;
    ssize_t nread;

    printf("$");
    fflush(stdout);

    while ((nread = getline(&line, &cap, stdin)) != -1) {

        if (nread > 0 && line[nread - 1] == '\n')
            line[nread - 1] = '\0';

        /* empty line -> reprompt */
        if (line[0] == '\0') {
            printf("$");
            fflush(stdout);
            continue;
        }
        
        char *argv[MAX_ARGS];
        int argc = tokenize(line, argv, MAX_ARGS);
        if (argc < 0) {
            printf("$");
            fflush(stdout);
            continue;
        }

        int builtin = handle_builtin(argv);
        if (builtin == 0) {
            break;
        } else if (builtin == 1) {
            printf("$");
            fflush(stdout);
            continue;
        }

        run_external(argv);

        printf("$");
        fflush(stdout);
    }
    free(line);
    return 0;
}