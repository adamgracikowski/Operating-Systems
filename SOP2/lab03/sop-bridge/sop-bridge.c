#define _GNU_SOURCE

#include <stdio.h>
#include <string.h>

#include "hand.h"
#include "macros.h"
#include "table.h"

int compare_cards_sort(const void* lhs, const void* rhs);
void start_game(player_hand_t* hand, table_t* table);
void play_trick(player_hand_t* hand, table_t* table);
void asses_result(player_hand_t* hand, table_t* table);

int main(int argc, char* argv[])
{
    UNUSED(argc);
    UNUSED(argv);

    srand(time(NULL));

    int player_idx;

    table_t* table = table_init(&player_idx);
    printf("Four players ready. Starting the game.\n");

    player_hand_t hand;
    player_hand_init(&hand, player_idx);

    start_game(&hand, table);
    player_hand_print_hand(&hand);

    pthread_barrier_wait(&table->players_barrier);
    printf("Starting the game!\n");
    for (int i = 0; i < PLAYER_HAND_COUNT; ++i)
    {
        play_trick(&hand, table);
        asses_result(&hand, table);
    }

    if (pthread_barrier_wait(&table->players_barrier) == PTHREAD_BARRIER_SERIAL_THREAD)
    {
        printf("Destroying table\n");
        table_destroy(table);
    }
    else
    {
        printf("Leaving table\n");
        table_close(table);
    }

    return EXIT_SUCCESS;
}

int compare_cards_sort(const void* lhs, const void* rhs)
{
    return (*(card_t*)lhs > *(card_t*)rhs) - (*(card_t*)lhs < *(card_t*)rhs);
}

void start_game(player_hand_t* hand, table_t* table)
{
    memcpy(hand->hand_cards, table->cards + hand->idx * PLAYER_HAND_COUNT, sizeof(card_t) * PLAYER_HAND_COUNT);
    qsort(hand->hand_cards, PLAYER_HAND_COUNT, sizeof(card_t), compare_cards_sort);
    memset(table->cards + hand->idx * PLAYER_HAND_COUNT, INVALID_CARD, sizeof(card_t) * PLAYER_HAND_COUNT);
}

void play_trick(player_hand_t* hand, table_t* table)
{
    int played_idx = -1;

    if (hand->leading_player_idx != hand->idx)
    {
        pthread_mutex_lock(&table->placed_lock);
        int prev_idx = PREV_PLAYER_IDX(hand->idx);
        while (table->trick[prev_idx] == INVALID_CARD)
            pthread_cond_wait(&table->placed_cond, &table->placed_lock);

        if (player_hand_has_stronger(hand, table->trick[hand->leading_player_idx]))
            played_idx = player_hand_find_strongest(hand, CARD_COLOR(table->trick[hand->leading_player_idx]));
        else
            played_idx = player_hand_find_weakest(hand, CARD_COLOR(table->trick[hand->leading_player_idx]));
    }
    else
    {
        played_idx = player_hand_select_leading_card(hand);
        pthread_mutex_lock(&table->placed_lock);
    }

    table->trick[hand->idx] = hand->hand_cards[played_idx];
    pthread_mutex_unlock(&table->placed_lock);
    hand->played_cards[hand->trick_counter] = hand->hand_cards[played_idx];
    hand->hand_cards[played_idx] = INVALID_CARD;

    pthread_cond_broadcast(&table->placed_cond);
    pthread_barrier_wait(&table->players_barrier);
}

void asses_result(player_hand_t* hand, table_t* table)
{
    card_t strongest_card = table->trick[hand->leading_player_idx];
    int strongest_idx = hand->leading_player_idx;
    for (int i = 0; i < PLAYERS_COUNT; ++i)
    {
        if (is_card_stronger_than(strongest_card, table->trick[i]))
            strongest_idx = i;
    }

    hand->trick_results[hand->trick_counter] = (strongest_idx % 2 == hand->idx % 2);
    hand->leading_player_idx = strongest_idx;

    printf("Played card ");
    player_hand_print_played_card(hand, hand->trick_counter++);

    pthread_barrier_wait(&table->players_barrier);

    table->trick[hand->idx] = INVALID_CARD;
    pthread_barrier_wait(&table->players_barrier);
}
