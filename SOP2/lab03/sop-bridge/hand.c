#include "hand.h"

#include <stdio.h>

void player_hand_init(player_hand_t *hand, int player_idx)
{
    hand->idx = player_idx;
    hand->trick_counter = 0;
    hand->leading_player_idx = 0;
    for (int i = 0; i < CARDS_IN_COLOR; ++i)
    {
        hand->hand_cards[i] = INVALID_CARD;
        hand->played_cards[i] = INVALID_CARD;
        hand->trick_results[i] = 0;
    }
}

void player_hand_print_hand(player_hand_t *hand)
{
    printf("Hand: ");
    for (int i = 0; i < PLAYER_HAND_COUNT; ++i)
    {
        print_card(hand->hand_cards[i]);
        printf(" ");
    }
    printf("\n");
}

int player_hand_find_strongest(player_hand_t *hand, int color)
{
    int i = 0;
    int strongest_idx = -1;
    for (; i < CARDS_IN_COLOR; ++i)
    {
        if (hand->hand_cards[i] == INVALID_CARD)
            continue;

        if (CARD_COLOR(hand->hand_cards[i]) != color)
            continue;

        if (strongest_idx == -1)
        {
            strongest_idx = i;
            continue;
        }

        if (hand->hand_cards[i] > hand->hand_cards[strongest_idx])
            strongest_idx = i;
    }

    return strongest_idx;
}

int player_hand_has_stronger(player_hand_t *hand, card_t other)
{
    int strongest_idx = player_hand_find_strongest(hand, CARD_COLOR(other));
    if (strongest_idx == -1)
        return 0;
    return hand->played_cards[strongest_idx] > other;
}

int player_hand_find_weakest(player_hand_t *hand, int color)
{
    int i = 0;
    int weakest_idx = -1;
    for (; i < CARDS_IN_COLOR; ++i)
    {
        if (hand->hand_cards[i] == INVALID_CARD)
            continue;

        if (CARD_COLOR(hand->hand_cards[i]) != color)
            continue;

        if (weakest_idx == -1)
        {
            weakest_idx = i;
            continue;
        }

        if (hand->hand_cards[i] < hand->hand_cards[weakest_idx])
            weakest_idx = i;
    }

    if (weakest_idx != -1)
        return weakest_idx;

    i = 0;
    for (; i < CARDS_IN_COLOR; ++i)
    {
        if (hand->hand_cards[i] == INVALID_CARD)
            continue;

        if (weakest_idx == -1)
        {
            weakest_idx = i;
            continue;
        }

        if (CARD_POWER(hand->hand_cards[i]) < CARD_POWER(hand->hand_cards[weakest_idx]))
            weakest_idx = i;
    }

    return weakest_idx;
}

int player_hand_select_leading_card(player_hand_t *hand)
{
    int i = 0;
    int strongest_idx = -1;
    for (; i < CARDS_IN_COLOR; ++i)
    {
        if (hand->hand_cards[i] == INVALID_CARD)
            continue;

        if (strongest_idx == -1)
        {
            strongest_idx = i;
            continue;
        }

        if (CARD_POWER(hand->hand_cards[i]) > CARD_POWER(hand->hand_cards[strongest_idx]))
            strongest_idx = i;
    }

    return strongest_idx;
}

void player_hand_print_played_card(player_hand_t *hand, int played_idx)
{
    print_card(hand->played_cards[played_idx]);
    if (hand->trick_results[played_idx] == CARD_TAKEN)
        printf(" TAKEN\n");
    else
        printf(" GIVEN\n");
}
