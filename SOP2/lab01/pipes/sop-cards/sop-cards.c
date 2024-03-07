#include "pipe-utils.h"

#define MIN_N 2
#define MAX_N 5
#define MIN_M 5
#define MAX_M 10
#define RESIGN_PROBABILITY 5
#define MSG_SIZE (16 * sizeof(char))

void usage(char *name);
void read_command_line_argument(int argc, char **argv, int *n, int *m);
int should_resign(int probability);
int draw_card(int *cards, int *last);
void create_cards(int *cards, int m);
void prepare_message(int number, char *message, int message_size);
void prepare_text_message(const char *text, char *message, int message_size);
void player_work(int rp, int wp, int player_idx, int m);
void game_server_work(int *rps, int *wps, int n, int m);

int main(int argc, char **argv)
{

    int n, m;
    read_command_line_argument(argc, argv, &n, &m);
    fprintf(stderr, "n = %d, m = %d\n", n, m);

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
            close_pipes_except(to_player, n, i, READ);
            close_pipes_except(from_player, n, i, WRITE);
            int rp = to_player[i][READ];
            int wp = from_player[i][WRITE];
            player_work(rp, wp, i, m);
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

    game_server_work(rps, wps, n, m);

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
    fprintf(stderr, "USAGE: %s %d <= N <= %d, %d <= M <= %d\n", name, MIN_N, MAX_N, MIN_M, MAX_M);
    exit(EXIT_FAILURE);
}

void read_command_line_argument(int argc, char **argv, int *n, int *m)
{
    if (argc != 3)
        usage(argv[0]);
    *n = atoi(argv[1]);
    if (*n < MIN_N || *n > MAX_N)
        usage(argv[0]);
    *m = atoi(argv[2]);
    if (*m < MIN_M || *m > MAX_M)
        usage(argv[0]);
}

int should_resign(int probability)
{
    return rand() % probability == 0;
}

int draw_card(int *cards, int *last)
{
    if (*last < 0)
        ERR("draw_card");

    int card_idx = rand() % (*last + 1);
    int drawn = cards[card_idx];
    for (int i = card_idx; i < *last; i++)
    {
        cards[i] = cards[i + 1];
    }
    cards[(*last)--] = drawn;
    return drawn;
}

void create_cards(int *cards, int m)
{
    for (int i = 0; i < m; i++)
        cards[i] = i + 1;
}

void prepare_message(int number, char *message, int message_size)
{
    *((int *)(message)) = number;
    char *mes = message + sizeof(int);
    int to_zero_count = message_size - sizeof(int);
    memset(mes, 0, to_zero_count);
}

void prepare_text_message(const char *text, char *message, int message_size)
{
    size_t text_size = strlen(text) + 1;
    strcpy(message, text);
    char *mes = message + text_size;
    int to_zero_count = message_size - text_size;
    memset(mes, 0, to_zero_count);
}

void player_work(int rp, int wp, int player_idx, int m)
{
    srand((unsigned)time(NULL) * getpid());
    const char *new_round = "new_round";
    int ret, drawn, last = m - 1, score;
    char message[MSG_SIZE];
    
    fprintf(stdout, "Player %d: Joined the game!\n", player_idx);

    int cards[m];
    create_cards(cards, m);

    for (int i = 1; i <= m; i++)
    {
        if (should_resign(RESIGN_PROBABILITY))
        {
            fprintf(stdout, "Player %d: Leaving the game in round %d!\n", player_idx, i);
            break;
        }
        if ((ret = read(rp, message, MSG_SIZE)) < 0)
            ERR("read");
        if (strcmp(message, new_round) != 0)
            ERR("new_round");

        drawn = draw_card(cards, &last);
        prepare_message(drawn, message, MSG_SIZE);
        if ((ret = write(wp, message, MSG_SIZE)) < 0)
            ERR("write");
        fprintf(stdout, "Player %d: Sending %d in round %d!\n", player_idx, drawn, i);

        if ((ret = read(rp, message, MSG_SIZE)) < 0)
            ERR("read");
        score = *((int *)message);
        fprintf(stdout, "Player %d: Scoring %d in round %d!\n", player_idx, score, i);
    }
}

void game_server_work(int *rps, int *wps, int n, int m)
{
    srand((unsigned)time(NULL) * getpid());
    const char *new_round = "new_round";
    int ret, round_counter = 1, winning_card, winning_count, winning_score, score, any;
    char message[MSG_SIZE];

    int cards[n];
    int total_points[n];
    memset(total_points, 0, n * sizeof(int));

    fprintf(stdout, "Server: The game has started!\n");
    while (round_counter <= m)
    {
        fprintf(stdout, "Server: NEW ROUND %d!\n", round_counter);
        any = 0;

        // send new round
        prepare_text_message(new_round, message, MSG_SIZE);
        for (int i = 0; i < n; i++)
        {
            if (wps[i] == 0)
                continue;
            any = 1;
            
            errno = 0;
            if ((ret = write(wps[i], message, MSG_SIZE)) < 0)
            {
                if (errno == EPIPE)
                {
                    close_pipe(wps[i]);
                    wps[i] = 0;
                }
                else
                    ERR("write");
            }
        }
        
        // check if there are any active players
        if (any == 0)
            break;

        // get cards from players
        for (int i = 0; i < n; i++)
        {
            if (wps[i] == 0)
                continue;
            if ((ret = read(rps[i], message, MSG_SIZE)) < 0)
                ERR("write");
            if (ret == 0)
            {
                close_pipe(rps[i]);
                rps[i] = 0;
                continue;
            }
            cards[i] = *((int *)message);
            fprintf(stdout, "Server: Received %d from Player %d in round %d!\n", cards[i], i, round_counter);
        }

        // find winners
        winning_card = -1;
        winning_count = 0;
        for (int i = 0; i < n; i++)
        {
            if (wps[i] == 0)
                continue;
            if (cards[i] > winning_card)
            {
                winning_card = cards[i];
                winning_count = 1;
            }
            else if (cards[i] == winning_card)
            {
                winning_count++;
            }
        }

        // send scores to players
        winning_score = n / winning_count;
        for (int i = 0; i < n; i++)
        {
            if (wps[i] == 0)
                continue;
            score = cards[i] == winning_card ? winning_score : 0;
            total_points[i] += score;
            prepare_message(score, message, MSG_SIZE);
            if ((ret = write(wps[i], message, MSG_SIZE)) < 0)
            {
                if (errno == EPIPE)
                {
                    if (wps[i] != 0)
                        close_pipe(wps[i]);
                    wps[i] = 0;
                }
                else
                    ERR("write");
            }
        }

        fprintf(stdout, "Server: Winning card is %d, there are %d winners this round!\n", winning_card, winning_count);
        round_counter++;
    }

    // display summary
    fprintf(stdout, "Server: It's time to summarise the game!\n");
    for (int i = 0; i < n; i++)
    {
        fprintf(stdout, "\tPlayer %d: %d points\n", i, total_points[i]);
    }
}