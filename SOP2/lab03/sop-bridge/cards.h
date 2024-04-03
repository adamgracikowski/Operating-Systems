#pragma once

#include <stdio.h>

#define PLAYERS_COUNT 4
#define CARDS_COUNT 52
#define PLAYER_HAND_COUNT (CARDS_COUNT / PLAYERS_COUNT)

#define CARDS_IN_COLOR 13
#define CLUBS_IDX 0
#define DIAMONDS_IDX 1
#define HEARTS_IDX 2
#define SPADES_IDX 3

#define ACE_IDX 12
#define KING_IDX 11
#define QUEEN_IDX 10
#define JACK_IDX 9

#define INVALID_CARD 0xff
#define CARD_TAKEN 1

#define PREV_PLAYER_IDX(IDX) ((IDX + PLAYERS_COUNT - 1) % PLAYERS_COUNT)
#define CARD_COLOR(CARD) (CARD / CARDS_IN_COLOR)
#define CARD_POWER(CARD) (CARD % CARDS_IN_COLOR)

typedef unsigned char card_t;

void shuffle(card_t *array, size_t n);
void print_card(int card_number);
int is_card_stronger_than(card_t reference_card, card_t card);