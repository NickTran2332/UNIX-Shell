#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>

#define MAX_LINE 1024
#define MAX_ARGS 100
#define MAX_PATHS 100

char error_message[30] = "An error has occurred\n";
char *paths[MAX_PATHS];
int path_count = 1;
int is_interactive = 1;

void print_error() {
    write(STDERR_FILENO, error_message, strlen(error_message));
}

void set_path(char **args, int arg_count) {
    for (int i = 0; i < path_count; i++) {
        free(paths[i]);
    }
    path_count = 0;
    for (int i = 1; i < arg_count; i++) {
        paths[path_count++] = strdup(args[i]);
    }
}

void change_directory(char **args, int arg_count) {
    if (arg_count != 2) {
        print_error();
        return;
    }
    if (chdir(args[1]) != 0) {
        print_error();
    }
}

char *find_executable(char *cmd) {
    for (int i = 0; i < path_count; i++) {
        char full_path[MAX_LINE];
        snprintf(full_path, sizeof(full_path), "%s/%s", paths[i], cmd);
        if (access(full_path, X_OK) == 0) {
            return strdup(full_path);
        }
    }
    return NULL;
}

void execute_command(char **args, int arg_count, char *redirect_file) {
    char *exec_path = find_executable(args[0]);
    if (!exec_path) {
        print_error();
        return;
    }

    if (is_interactive) {
    printf("[DEBUG] Executing: ");
    for (int i = 0; i < arg_count; i++) printf("%s ", args[i]);
    if (redirect_file) printf("> %s", redirect_file);
    printf("\n");
    }


    if (redirect_file && 
        (strcmp(args[0], "cd") == 0 || strcmp(args[0], "exit") == 0 || strcmp(args[0], "path") == 0)) {
        print_error();
        free(exec_path);
        return;
    }

    pid_t pid = fork();
    if (pid == 0) {
        if (redirect_file) {
            int fd = open(redirect_file, O_CREAT | O_WRONLY | O_TRUNC, 0666);
            if (fd < 0) {
                print_error();
                exit(1);
            }
            dup2(fd, STDOUT_FILENO);
            dup2(fd, STDERR_FILENO);
            close(fd);
        }
        execv(exec_path, args);
        print_error();
        exit(1);
    } else if (pid > 0) {
        waitpid(pid, NULL, 0);
    } else {
        print_error();
    }
    free(exec_path);
}

void parse_and_run(char *line) {
    char *commands[MAX_ARGS];
    int cmd_count = 0;

    char *cmd = strtok(line, "&");
    while (cmd && cmd_count < MAX_ARGS) {
        while (*cmd == ' ' || *cmd == '\t') cmd++;  // trim leading whitespace
        commands[cmd_count++] = strdup(cmd);
        cmd = strtok(NULL, "&");
    }

    pid_t pids[MAX_ARGS];
    int pid_index = 0;

    for (int i = 0; i < cmd_count; i++) {
        char *redirect_file = NULL;
        char *redir = strstr(commands[i], ">");
        if (redir) {
            *redir = '\0';
            redir++;
            while (*redir == ' ' || *redir == '\t') redir++;
            char *end = strtok(redir, " \t\n");
            if (!end || strtok(NULL, " \t\n")) {
                print_error();
                continue;
            }
            redirect_file = end;
        }

        char *args[MAX_ARGS];
        int arg_count = 0;
        char *token = strtok(commands[i], " \t\n");
        while (token && arg_count < MAX_ARGS - 1) {
            args[arg_count++] = token;
            token = strtok(NULL, " \t\n");
        }
        args[arg_count] = NULL;

        if (arg_count == 0) continue;

        if (strcmp(args[0], "exit") == 0) {
            if (arg_count != 1 || redirect_file) {
                print_error();
            } else {
                exit(0);
            }
        } else if (strcmp(args[0], "cd") == 0) {
            if (redirect_file) {
                print_error();
            } else {
                change_directory(args, arg_count);
            }
        } else if (strcmp(args[0], "path") == 0) {
            if (redirect_file) {
                print_error();
            } else {
                set_path(args, arg_count);
            }
        } else {
            pid_t pid = fork();
            if (pid == 0) {
                execute_command(args, arg_count, redirect_file);
                exit(0);
            } else if (pid > 0) {
                pids[pid_index++] = pid;
            } else {
                print_error();
            }
        }
    }

    for (int i = 0; i < pid_index; i++) {
        waitpid(pids[i], NULL, 0);
    }

    for (int i = 0; i < cmd_count; i++) {
        free(commands[i]);
    }
}

int main(int argc, char *argv[]) {
    paths[0] = strdup("/bin");

    FILE *input = stdin;
    if (argc == 2) {
        input = fopen(argv[1], "r");
        if (!input) {
            print_error();
            exit(1);
        }
        is_interactive = 0;
    } else if (argc > 2) {
        print_error();
        exit(1);
    }

    char *line = NULL;
    size_t len = 0;

    while (1) {
        if (input == stdin) {
            printf("grsh> ");
        }

        if (getline(&line, &len, input) == -1) {
            if (input == stdin) printf("\n");
            break;
        }

        parse_and_run(line);
    }

    free(line);
    for (int i = 0; i < path_count; i++) {
        free(paths[i]);
    }
    if (input != stdin) fclose(input);
    return 0;
}
