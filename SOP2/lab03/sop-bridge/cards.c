#include "cards.h"

#include <stdlib.h>

void shuffle(card_t *array, size_t n)
{
    if (n <= 1)
        return;
    for (size_t i = 0; i < n - 1; i++)
    {
        size_t j = i + rand() / (RAND_MAX / (n - i) + 1);
        int t = array[j];
        array[j] = array[i];
        array[i] = t;
    }
}

void print_card(int card_number)
{
    switch (CARD_POWER(card_number))
    {
    case ACE_IDX:
        printf("A");
        break;
    case KING_IDX:
        printf("K");
        break;
    case QUEEN_IDX:
        printf("Q");
        break;
    case JACK_IDX:
        printf("J");
        break;
    default:
        printf("%i", card_number % CARDS_IN_COLOR + 2);
        break;
    }

    switch (CARD_COLOR(card_number))
    {
    case CLUBS_IDX:
        printf("C");
        break;
    case DIAMONDS_IDX:
        printf("D");
        break;
    case HEARTS_IDX:
        printf("H");
        break;
    case SPADES_IDX:
        printf("S");
        break;
    }
    printf("(%i)", card_number);
}

int is_card_stronger_than(card_t reference_card, card_t card)
{
    return CARD_COLOR(reference_card) != CARD_COLOR(card) || CARD_POWER(card) > CARD_POWER(reference_card);
}
