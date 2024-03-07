#include "pipe-utils.h"

#define NUMBERS_MAX 49
#define NUMBERS 6
#define BET_PRICE 3
#define RESIGN_PROBABILITY 10
#define MSG_SIZE (NUMBERS * sizeof(int))

void usage(char *name);
void draw(int *numbers);
int compare(const int *bet, const int *numbers);
int get_reward(int matches);
void read_command_line_argument(int argc, char **argv, int *n, int *t);
int should_resign(int probability);
void player_work(int rp, int wp, int t);
void totalisator_work(int *rps, int *wps, int n, int t);

int main(int argc, char **argv)
{
    int n, t;
    read_command_line_argument(argc, argv, &n, &t);
    fprintf(stderr, "n = %d, t = %d\n", n, t);

    if (sethandler(SIG_IGN, SIGPIPE))
        ERR("sethandler");

    pipe_t to_player[n];
    create_pipes(to_player, n);
    pipe_t from_player[n];
    create_pipes(from_player, n);

    pid_t pid;
    for (int i = 0; i < n; i++)
    {
        switch ((pid = fork()))
        {
        case 0: // child
            fprintf(stderr, "[%d]: I'm going to play Lotto!\n", getpid());
            close_pipes_except(to_player, n, i, READ);
            close_pipes_except(from_player, n, i, WRITE);
            int rp = to_player[i][READ];
            int wp = from_player[i][WRITE];
            player_work(rp, wp, t);
            close_pipe(rp);
            close_pipe(wp);
            exit(EXIT_SUCCESS);
        case -1:
            ERR("fork");
        }
    }
    close_pipes_all_one_end(to_player, n, READ);
    close_pipes_all_one_end(from_player, n, WRITE);

    int rps[n];
    int wps[n];
    for (int i = 0; i < n; i++)
    {
        rps[i] = from_player[i][READ];
        wps[i] = to_player[i][WRITE];
    }

    totalisator_work(rps, wps, n, t);

    for (int i = 0; i < n; i++)
    {
        if (rps[i] != 0)
            close_pipe(rps[i]);
        if (wps[i] != 0)
            close_pipe(wps[i]);
    }
    while (wait(NULL) > 0)
        ;
    return EXIT_SUCCESS;
}

void usage(char *name)
{
    fprintf(stderr, "USAGE: %s N T\n", name);
    fprintf(stderr, "N: N >= 1 - number of players\n");
    fprintf(stderr, "T: T >= 1 - number of weeks (iterations)\n");
    exit(EXIT_FAILURE);
}

int should_resign(int probability)
{
    return rand() % probability == 0;
}

void draw(int *numbers)
{
    int numbers_all[NUMBERS_MAX];
    for (int i = 1; i <= NUMBERS_MAX; ++i)
    {
        numbers_all[i - 1] = i;
    }
    for (int i = 0; i < NUMBERS_MAX - 1; i++)
    {
        int j = i + rand() / (RAND_MAX / (NUMBERS_MAX - i) + 1);
        SWAP(numbers_all[i], numbers_all[j]);
    }
    for (int i = 0; i < NUMBERS; ++i)
    {
        numbers[i] = numbers_all[i];
    }
}

int compare(const int *bet, const int *numbers)
{
    int result = 0;
    for (int i = 0; i < NUMBERS; ++i)
    {
        int number = numbers[i];
        for (int j = 0; j < NUMBERS; ++j)
        {
            if (number == bet[j])
            {
                result++;
            }
        }
    }
    return result;
}

int get_reward(int matches)
{
    int rewards[] = {0, 0, 0, 24, 160, 6000, 10000000};
    return rewards[matches];
}

void read_command_line_argument(int argc, char **argv, int *n, int *t)
{
    if (argc != 3)
        usage(argv[0]);
    if ((*n = atoi(argv[1])) < 0)
        usage(argv[0]);
    if ((*t = atoi(argv[2])) < 0)
        usage(argv[0]);
}

void player_work(int rp, int wp, int t)
{
    srand((unsigned)time(NULL) * getpid());
    int ret, matches, reward;
    pid_t pid = getpid();
    int numbers[NUMBERS], winning[NUMBERS];

    for (int i = 0; i < t; i++)
    {
        if (should_resign(RESIGN_PROBABILITY))
        {
            fprintf(stdout, "[%d]: This is a waste of money!\n", getpid());
            break;
        }
        draw(numbers);
        if ((ret = write(wp, &pid, sizeof(pid_t))) < 0)
            ERR("read");
        if ((ret = write(wp, numbers, MSG_SIZE)) < 0)
            ERR("read");
        if ((ret = read(rp, winning, MSG_SIZE)) < 0)
            ERR("read");
        matches = compare(winning, numbers);
        reward = get_reward(matches);
        fprintf(stdout, "[%d]: I won %d$\n", getpid(), reward);
    }
}

void totalisator_work(int *rps, int *wps, int n, int t)
{
    srand((unsigned)time(NULL) * getpid());
    int ret, total_bet = 0, total_reward = 0, matches, reward, any;
    pid_t player_pid;
    int player_numbers[NUMBERS], winning[NUMBERS];

    for (int j = 0; j < t; j++)
    {
        any = 0;
        draw(winning);
        for (int i = 0; i < n; i++)
        {
            if (rps[i] == 0)
                continue;
            else
                any = 1;

            if ((ret = read(rps[i], &player_pid, sizeof(pid_t))) < 0)
                ERR("read");
            if (ret == 0)
            {
                close_pipe(rps[i]);
                rps[i] = 0;
                continue;
            }
            if ((ret = read(rps[i], player_numbers, MSG_SIZE)) < 0)
                ERR("read");

            fprintf(stdout, "Totalisator: [%d] bet: {%d, %d, %d, %d, %d, %d}\n",
                    player_pid, player_numbers[0], player_numbers[1], player_numbers[2],
                    player_numbers[3], player_numbers[4], player_numbers[5]);
            matches = compare(winning, player_numbers);
            reward = get_reward(matches);
            total_reward += reward;
            total_bet += BET_PRICE;
            errno = 0;
            if ((ret = write(wps[i], winning, MSG_SIZE)) < 0)
            {
                if (errno == ESPIPE)
                {
                    close_pipe(wps[i]);
                    wps[i] = 0;
                }
                else
                    ERR("write");
            }

            fprintf(stdout, "Totalisator: {%d, %d, %d, %d, %d, %d} are today lucky numbers!\n",
                    winning[0], winning[1], winning[2],
                    winning[3], winning[4], winning[5]);
        }
        if (any == 0)
            break;
    }
    fprintf(stdout, "Totalisator:\n\tTotal bet: %d\n\tTotal reward: %d\n", total_bet, total_reward);
}