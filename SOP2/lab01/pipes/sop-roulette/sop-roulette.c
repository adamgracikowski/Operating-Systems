#include "pipe-utils.h"

#define MIN_N 1
#define MIN_M 100
#define MAX_ROULETTE 37
#define PROBABILITY 10
#define PRIZE_FACTOR 35

typedef unsigned int UINT;
typedef int pipe_t[2];
typedef void (*sighandler_t)(int);

void usage(char *name);
void read_command_line_argument(int argc, char **argv, int *n, int *m);
int place_bet(int *amount);
int should_resign(int probability);
void player_work(int wp, int rp, int m);
void server_work(int *wps, int *rps, int n);

int main(int argc, char **argv)
{
    if (sethandler(SIG_IGN, SIGPIPE))
        ERR("sethandler");

    int n, m;
    read_command_line_argument(argc, argv, &n, &m);
    fprintf(stderr, "n = %d, m = %d\n", n, m);

    pipe_t *to_player = malloc(n * sizeof(pipe_t));
    if (!to_player)
        ERR("malloc");
    create_pipes(to_player, n);

    pipe_t *from_player = malloc(n * sizeof(pipe_t));
    if (!from_player)
        ERR("malloc");
    create_pipes(from_player, n);

    pid_t pid;
    int money = m;
    for (int i = 0; i < n; i++)
    {
        switch ((pid = fork()))
        {
        case 0: // child
            fprintf(stdout, "[%d]: I have %d and I'm going to play roulette!\n", getpid(), money);
            close_pipes_except(to_player, n, i, READ);
            close_pipes_except(from_player, n, i, WRITE);
            int rp = to_player[i][READ];
            int wp = from_player[i][WRITE];
            player_work(wp, rp, m);
            close_pipe(rp);
            close_pipe(wp);
            free(to_player);
            free(from_player);
            exit(EXIT_SUCCESS);
            break;
        case -1:
            ERR("fork");
            break;
        }
    }
    close_pipes_all_one_end(to_player, n, READ);
    close_pipes_all_one_end(from_player, n, WRITE);

    int *rps = malloc(n * sizeof(int));
    if (!rps)
        ERR("malloc");
    int *wps = malloc(n * sizeof(int));
    if (!wps)
        ERR("malloc");
    for (int i = 0; i < n; i++)
    {
        rps[i] = from_player[i][READ];
        wps[i] = to_player[i][WRITE];
    }
    free(to_player);
    free(from_player);

    server_work(wps, rps, n);

    for (int i = 0; i < n; i++)
    {
        if (rps[i] != 0)
            close_pipe(rps[i]);
        if (wps[i] != 0)
            close_pipe(wps[i]);
    }

    free(rps);
    free(wps);
    while (wait(NULL) > 0)
        ;
}

void usage(char *name)
{
    fprintf(stderr, "USAGE: %s N M\n", name);
    fprintf(stderr, "N: N >= 1 - number of players\n");
    fprintf(stderr, "M: M >= 100 - initial amount of money\n");
    exit(EXIT_FAILURE);
}

void read_command_line_argument(int argc, char **argv, int *n, int *m)
{
    if (argc != 3)
        usage(argv[0]);
    *n = atoi(argv[1]);
    if (*n < MIN_N)
        usage(argv[0]);
    *m = atoi(argv[2]);
    if (*m < MIN_M)
        usage(argv[0]);
}

int place_bet(int *amount)
{
    int bet = rand() % *amount + 1;
    *amount -= bet;
    return bet;
}

int should_resign(int probability)
{
    return rand() % probability == 0;
}

void player_work(int wp, int rp, int m)
{
    srand((unsigned)time(NULL) * getpid());
    int ret, lucky_number;
    int amount = m;
    int pid = getpid();

    while (amount > 0)
    {
        if (should_resign(PROBABILITY))
        {
            fprintf(stdout, "[%d]: I saved %d$!\n", pid, amount);
            break;
        }
        int bet = place_bet(&amount);
        int guess = rand() % MAX_ROULETTE;
        if ((ret = write(wp, &pid, sizeof(int))) < 0)
            ERR("write");
        if ((ret = write(wp, &guess, sizeof(int))) < 0)
            ERR("write");
        if ((ret = write(wp, &bet, sizeof(int))) < 0)
            ERR("write");
        if ((ret = read(rp, &lucky_number, sizeof(int))) < 0)
            ERR("read");
        if (lucky_number == guess)
        {
            fprintf(stdout, "[%d]: Whoa, I won %d!\n", pid, PRIZE_FACTOR * bet);
            amount += PRIZE_FACTOR * bet;
        }
    }
    if (amount <= 0)
    {
        fprintf(stdout, "[%d]: I'm broke :(\n", pid);
    }
}

void server_work(int *wps, int *rps, int n)
{
    srand((unsigned)time(NULL) * getpid());
    int player_bet, player_guess, player_pid, ret, any;

    while (1)
    {
        int lucky_number = rand() % MAX_ROULETTE;
        any = 0;
        for (int i = 0; i < n; i++)
        {
            if (rps[i] == 0)
                continue;
            any = 1;
            if ((ret = read(rps[i], &player_pid, sizeof(int))) < 0)
                ERR("read");
            if (ret == 0)
            {
                close_pipe(rps[i]);
                rps[i] = 0;
                continue;
            }
            if ((ret = read(rps[i], &player_guess, sizeof(int))) < 0)
                ERR("read");
            if (ret == 0)
            {
                close_pipe(rps[i]);
                rps[i] = 0;
                continue;
            }
            if ((ret = read(rps[i], &player_bet, sizeof(int))) < 0)
                ERR("read");
            if (ret == 0)
            {
                close_pipe(rps[i]);
                rps[i] = 0;
                continue;
            }

            errno = 0;
            if ((ret = write(wps[i], &lucky_number, sizeof(int))) < 0)
            {
                if (errno == EPIPE)
                {
                    if (wps[i] != 0)
                    {
                        close_pipe(wps[i]);
                        wps[i] = 0;
                        continue;
                    }
                }
                else
                    ERR("write in server");
            }

            fprintf(stdout, "Croupier: [%d] placed %d on a %d!\n", player_pid, player_bet, player_guess);
        }

        if (any == 0)
            break;

        fprintf(stdout, "Croupier: %d is the lucky number!\n", lucky_number);
    }

    fprintf(stdout, "Croupier: Casino always wins!\n");
}