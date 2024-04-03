#pragma once

#include "cards.h"

typedef struct player_hand
{
    int idx;
    int leading_player_idx;
    int trick_counter;
    card_t hand_cards[PLAYER_HAND_COUNT];
    card_t played_cards[PLAYER_HAND_COUNT];
    int trick_results[PLAYER_HAND_COUNT];
} player_hand_t;

void player_hand_init(player_hand_t* hand, int player_idx);

void player_hand_print_hand(player_hand_t* hand);

int player_hand_find_strongest(player_hand_t* hand, int color);

int player_hand_has_stronger(player_hand_t* hand, card_t other);

int player_hand_find_weakest(player_hand_t* hand, int color);

int player_hand_select_leading_card(player_hand_t* hand);

void player_hand_print_played_card(player_hand_t *hand, int played_idx);
