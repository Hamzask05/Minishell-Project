/* ============================================================
 * minishell_skeleton.c  –  Squelette TD Mini-Shell  (ET3)
 * Compilez avec : gcc -Wall -o minishell minishell_skeleton.c
 * ============================================================ */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <termios.h>

/* ─── Constantes ────────────────────────────────────────────── */
#define MAX_LINE  1024
#define MAX_ARGS  64

/* ─── Variables globales shell ──────────────────────────────── */
int   shell_terminal;
pid_t shell_pgid;
struct termios shell_tmodes;

/* ─── Structure Job ─────────────────────────────────────────── */
typedef enum { RUNNING, STOPPED, DONE } job_status_t;

typedef struct job {
    int           job_id;
    pid_t         pgid;
    char          command[256];
    job_status_t  status;
    struct job   *next;
} job_t;

job_t *job_list = NULL;
int    next_job_id = 1;

/* ─── Prototypes ────────────────────────────────────────────── */
void   init_shell(void);
int    parse_line(char *line, char **argv, int max_args);
void   execute_command(int argc, char **argv);
void   launch_job(int argc, char **argv, int foreground);
void   wait_for_job(pid_t pgid);
job_t *add_job(pid_t pgid, const char *cmd);
void   remove_job(pid_t pgid);
void   update_job_statuses(void);
void   builtin_jobs(void);
void   builtin_fg(char *arg);
void   builtin_bg(char *arg);

/* TODO 3a : (hint) définir une struct pour gérer les redirections ici   */

typedef struct {
    char *input_file;    // fichier pour 
    char *output_file;   // fichier pour > ou >>
    int   append;        // 1 si >>, 0 si >
} redirection_t;

// var glob redirection_t
redirection_t redir;

/* ================================================================
 * init_shell : prendre le contrôle du terminal
 * ================================================================ */
void init_shell(void) {
    shell_terminal = STDIN_FILENO;

    /* Attendre d'être en foreground */
    while (tcgetpgrp(shell_terminal) != (shell_pgid = getpgrp()))
        kill(-shell_pgid, SIGTTIN);

    /* TODO 4a : ignorer SIGINT, SIGQUIT, SIGTSTP, SIGTTIN, SIGTTOU */

    /* TODO 4b : créer notre propre groupe de processus (setpgid) */

    /* TODO 4c : prendre le contrôle du terminal (tcsetpgrp) */

    /* TODO 4d : sauvegarder les attributs du terminal (tcgetattr) */
}

/* ================================================================
 * parse_line : découpe `line` en tokens dans argv[]
 * Retourne le nombre de tokens.
 * ================================================================ */
int parse_line(char *line, char **argv, int max_args) {
    int argc = 0;
    char *token = strtok(line, " \t");

    /* TODO 3a (hint) : gestion des tokens de redirection */

    redir.input_file  = NULL;
    redir.output_file = NULL;
    redir.append      = 0;

    /////////////////////////////////////////////////////////////////////

   while (token && argc < max_args - 1) {
        if (strcmp(token, ">") == 0) {
            redir.output_file = strtok(NULL, " \t");
            redir.append = 0;
        }
        else if (strcmp(token, ">>") == 0) {
            redir.output_file = strtok(NULL, " \t");
            redir.append = 1;
        }
        else if (strcmp(token, "<") == 0) {
            redir.input_file = strtok(NULL, " \t");
        }
        else {
            argv[argc++] = token;
        }
        token = strtok(NULL, " \t");
    }
    argv[argc] = NULL;
    return argc;

}

/* ================================================================
 * execute_command : point d'entrée principal
 * Détecte les builtins, les pipes, les redirections, puis délègue
 * ================================================================ */
// part 3 suite
 void execute_pipe(char **argv_left, char **argv_right)
{
    int pipefd[2];
    if (pipe(pipefd) < 0) { perror("pipe"); return; }

    pid_t pid_left = fork();
    if (pid_left == 0) {
        /* Fils gauche : stdout → pipefd[1] */
        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[0]);
        close(pipefd[1]);
        execvp(argv_left[0], argv_left);
        perror(argv_left[0]);
        exit(EXIT_FAILURE);
    }

    pid_t pid_right = fork();
    if (pid_right == 0) {
        /* Fils droit : stdin ← pipefd[0] */
        dup2(pipefd[0], STDIN_FILENO);
        close(pipefd[0]);
        close(pipefd[1]);
        execvp(argv_right[0], argv_right);
        perror(argv_right[0]);
        exit(EXIT_FAILURE);
    }

    /* Père : fermer les deux extrémités puis attendre les deux fils */
    close(pipefd[0]);
    close(pipefd[1]);
    waitpid(pid_left,  NULL, 0);
    waitpid(pid_right, NULL, 0);
}


void execute_command(int argc, char **argv) {

    // part 3 premiere partie de l'implementation
       /* Détecter un pipe | */
    for (int i = 0; i < argc; i++) {
        if (strcmp(argv[i], "|") == 0) {
            argv[i] = NULL;
            execute_pipe(argv, argv + i + 1);
            return;
        }
    }

    /* TODO 2a : détecter le builtin "cd" et appeler chdir() */
    // le code ci dessous est pour le cd
    if (strcmp(argv[0], "cd") == 0) {
        char *dir = argv[1];
        if (dir == NULL)
            dir = getenv("HOME");   // cd sans argument → aller dans $HOME
        if (chdir(dir) < 0)
            perror("cd");
        return;   // pas de fork pour un builtin
    }

     /* 2a — Commande externe : fork + exec + wait */
    pid_t pid = fork();

    if (pid == 0) {
        /* Redirection entrée */
        if (redir.input_file) {
            int fd = open(redir.input_file, O_RDONLY);
            if (fd < 0) { perror(redir.input_file); exit(EXIT_FAILURE); }
            dup2(fd, STDIN_FILENO);
            close(fd);
        }
        /* Redirection sortie */
        if (redir.output_file) {
            int flags = O_WRONLY | O_CREAT | (redir.append ? O_APPEND : O_TRUNC);
            int fd = open(redir.output_file, flags, 0644);
            if (fd < 0) { perror(redir.output_file); exit(EXIT_FAILURE); }
            dup2(fd, STDOUT_FILENO);
            close(fd);
        }

        execvp(argv[0], argv);
        perror(argv[0]);
        exit(EXIT_FAILURE);
    }
    else if (pid > 0) {
        int status;
        waitpid(pid, &status, 0);
    }
    else {
        perror("fork");
    }

    /* TODO 5a : détecter le builtin "jobs" */

    /* TODO 5b : détecter les builtins "fg" et "bg" */

    /* TODO 3b : détecter un pipe '|' dans argv et appeler execute_pipe() (à écrire) */

    /* TODO 2b : détecter '&' en dernier token -> background */
    //à ajouter à la partie 4 int foreground = 1;
    /* ... */

    //à ajouter à la partie 4 launch_job(argc, argv, foreground); 
}

/* ================================================================
 * launch_job : fork + exec avec gestion des groupes de processus
 * ================================================================ */
void launch_job(int argc, char **argv, int foreground) {
    pid_t pid = fork();
    if (pid == 0) {
        /* TODO 4e : mettre le fils dans son propre groupe (setpgid) */

        /* TODO 4f : si foreground, donner le terminal au fils (tcsetpgrp) */

        /* TODO 4g : rétablir les signaux par défaut (SIGINT, SIGTSTP…) */

        /* TODO 3a : appliquer les redirections éventuelles ici */

        execvp(argv[0], argv);
        perror(argv[0]);
        exit(EXIT_FAILURE);

    } else if (pid > 0) {
        /* TODO 4h : setpgid depuis le père (évite la race condition) */

        /* TODO 5c : ajouter le job à job_list */

        if (foreground) {
            /* TODO 4i : donner le terminal au fils depuis le père */
            wait_for_job(pid);
            /* TODO 4j : reprendre le terminal (tcsetpgrp vers shell_pgid) */
        } else {
            printf("[%d] %d\n", next_job_id - 1, pid);
        }
    } else {
        perror("fork");
    }
}

/* ================================================================
 * wait_for_job : attendre qu'un job passe en STOPPED ou DONE
 * ================================================================ */
void wait_for_job(pid_t pgid) {
    int status;
    pid_t p;
    /* TODO 4k : boucler sur waitpid(-pgid, &status, WUNTRACED) */
    /* Sortir quand le processus est terminé ou stoppé */
    (void)pgid; (void)status; (void)p; /* retirer quand implémenté */
}

/* ================================================================
 * Gestion de la liste de jobs
 * ================================================================ */
job_t *add_job(pid_t pgid, const char *cmd) {
    job_t *j = malloc(sizeof(job_t));
    /* TODO 5d : initialiser les champs, insérer en tête de job_list */
    (void)pgid; (void)cmd;
    return j;
}

void remove_job(pid_t pgid) {
    /* TODO 5e : retirer le job avec ce pgid de job_list, libérer */
    (void)pgid;
}

void update_job_statuses(void) {
    /* TODO 5f : waitpid(-1, &status, WNOHANG|WUNTRACED) en boucle
       Mettre à jour status dans job_list, afficher les jobs Done */
}

void builtin_jobs(void) {
    /* TODO 5g : afficher job_list avec format [id] Status command */
}

void builtin_fg(char *arg) {
    /* TODO 5h : SIGCONT au groupe, rendre le terminal, wait_for_job */
    (void)arg;
}

void builtin_bg(char *arg) {
    /* TODO 5i : SIGCONT au groupe sans rendre le terminal */
    (void)arg;
}

/* ================================================================
 * main
 * ================================================================ */
int main(void)
{
    char  line[MAX_LINE];
    char *argv[MAX_ARGS];

    init_shell();   // sera complété en partie 4, pour l'instant laisse vide

    while (1)
    {
        /* TODO 5j : update_job_statuses() — partie 5 */

        /* 1a — Afficher le prompt */
        printf("minishell> ");
        fflush(stdout);   // force l'affichage avant fgets()

        /* 1b — Lire une ligne */
        if (fgets(line, MAX_LINE, stdin) == NULL)
        {
            /* 1c — EOF (Ctrl-D) : quitter proprement */
            printf("\n");
            break;
        }

        /* 1d — Supprimer le '\n' final */
        line[strcspn(line, "\n")] = '\0';

        /* Parser */
        int argc = parse_line(line, argv, MAX_ARGS);
        if (argc == 0)
            continue;   // ligne vide → reprendre la boucle

        /* 1e — Builtin exit : quitter proprement */
        /* suppression du doublon qui existait avant */
        if (strcmp(argv[0], "exit") == 0)
            break;

        /* 1e — Builtin echo : afficher les arguments */
        /* on boucle sur argv[1..argc-1] pour afficher mot par mot */
        /* continue évite d'appeler execute_command pour un builtin */
        if (strcmp(argv[0], "echo") == 0) {
            for (int i = 1; i < argc; i++) {
                printf("%s", argv[i]);
                if (i < argc - 1)
                    printf(" ");   // espace entre les mots, pas après le dernier
            }
            printf("\n");
            continue;   // retourner au début de la boucle, pas de fork
        }

        /* exécution de la commande (gère cd, pipes, redirections, fork/exec) */
        /* un seul appel — suppression du doublon execute_command qui existait avant */
        execute_command(argc, argv);
    }

    return 0;
}