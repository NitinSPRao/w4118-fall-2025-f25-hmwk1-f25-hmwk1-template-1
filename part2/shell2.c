#define _GNU_SOURCE

#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>

#define MAX_ARGS 128
#define MAX_LINE 1024

extern char **environ;

static int tokenize(char *line, char *argv[], int max)
{
    int argc = 0;

    while (*line == ' ') line++;

    while (*line != '\0')
    {
        if (argc >= max - 1) {
            syscall(SYS_write, 2, "error: too many arguments (max 127)\n", 37);
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
static void sh_err_msg(const char *m) {
    syscall(SYS_write, STDERR_FILENO, "error: ", 7);
    syscall(SYS_write, STDERR_FILENO, m, strlen(m));
    syscall(SYS_write, STDERR_FILENO, "\n", 1);
}

static void sh_errno(void) {
    int e = errno;
    const char *pfx = "error: ";
    const char *msg = strerror(e);
    syscall(SYS_write, STDERR_FILENO, pfx, 7);
    syscall(SYS_write, STDERR_FILENO, msg, strlen(msg));
    syscall(SYS_write, STDERR_FILENO, "\n", 1);
}

static int handle_builtin(char *argv[])
{
    if (argv[0] == NULL) return 1;

    if (strcmp(argv[0], "exit") == 0) return 0;

    if (strcmp(argv[0], "cd") == 0) {
        if (argv[1] == NULL || argv[2] != NULL) {
            sh_err_msg("usage: cd <dir>");
        } else if (syscall(SYS_chdir, argv[1]) == -1) {
            sh_errno();
        }
        return 1;
    }
    return 2;
}

static void set_sigint_default(void)
{
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = SIG_DFL;
    syscall(SYS_rt_sigaction, SIGINT, &sa, NULL, 8);
}

static void install_signal_handlers(void) {
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = SIG_IGN;
    syscall(SYS_rt_sigaction, SIGINT, &sa, NULL, 8);
}


static pid_t sh_fork(void) {
    long r;
#ifdef SYS_fork
    r = syscall(SYS_fork);
#else
    r = syscall(SYS_clone, SIGCHLD, 0, 0, 0, 0);
#endif
    if (r < 0) return -1;
    return (pid_t)r;
}

static void sh_execve(char *const path, char *const argv[]) {
    long rc = syscall(SYS_execve, path, argv, environ);
    (void)rc;
}

static int sh_waitpid(pid_t pid, int *status) {
    long rc = syscall(SYS_wait4, (long)pid, status, 0, NULL);
    if (rc < 0) return -1;
    return 0;
}

static ssize_t sh_readline(char *buf, size_t cap) {
    size_t i = 0;
    while (i < cap - 1) {
        char c;
        long n = syscall(SYS_read, STDIN_FILENO, &c, 1);
        if (n <= 0) {
            return (i == 0) ? -1 : (ssize_t)i;
        }
        if (c == '\n') break;
        buf[i++] = c;
    }
    buf[i] = '\0';
    return (ssize_t)i;
}

static void run_external(char *argv[]) {
    pid_t pid = sh_fork();
    if (pid < 0) {
        sh_errno();
        return;
    } else if (pid == 0) {
        set_sigint_default();
        sh_execve(argv[0], argv);
        sh_errno();
        _exit(127);
    } else {
        int status = 0;
        if (sh_waitpid(pid, &status) == -1) {
            sh_errno();
        }
    }
}

int main(void)
{
    install_signal_handlers();

    char   line[MAX_LINE];
    ssize_t nread;

    syscall(SYS_write, 1, "$", 1);

    while ((nread = sh_readline(line, MAX_LINE)) != -1) {

        if (nread > 0 && line[nread - 1] == '\n')
            line[nread - 1] = '\0';

        if (line[0] == '\0') {
            syscall(SYS_write, 1, "$", 1);
            continue;
        }
        
        char *argv[MAX_ARGS];
        int argc = tokenize(line, argv, MAX_ARGS);
        if (argc < 0) {
            syscall(SYS_write, 1, "$", 1);
            continue;
        }

        int builtin = handle_builtin(argv);
        if (builtin == 0) {
            break;
        } else if (builtin == 1) {
            syscall(SYS_write, 1, "$", 1);
            continue;
        }

        run_external(argv);

        syscall(SYS_write, 1, "$", 1);
    }
    syscall(SYS_write, 1, "\n", 1);
    return 0;
}