#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>

static void die(const char *s)
{
    perror(s);
    exit(1);
}

int main(void)
{
    char   *line = NULL;
    size_t  cap  = 0;
    ssize_t nread;
    pid_t   pid;
    int     status;

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

        pid = fork();
        if (pid < 0) {
            die("fork error");
        } else if (pid == 0) {
            /* child process */
            execl(line, line, (char *)0);
            /* if execl returns, it failed */
            fprintf(stderr, "error: %s\n", strerror(errno));
            _exit(127);
        } else {
            /* parent process */
            if (waitpid(pid, &status, 0) != pid)
                die("waitpid failed");
        }

        printf("$");
        fflush(stdout);
    }

    /* EOF or error from getline */
    free(line);
    return 0;
}