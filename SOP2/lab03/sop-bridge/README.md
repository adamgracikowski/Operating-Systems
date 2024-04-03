# Brydż:

Trzeba napisać program, który zasymuluje uproszczoną grę w brydża. Rozgrywka będzie bez atu i
bez licytacji. Najpierw przy pomocy shared memory zostanie utworzona ręka każdego gracza (rozdawanie)
i przy pomocy shared bariery zbierze się $4$ graczy. Później rozpocznie się rozgrywka. Zaczyna rozdający
pierwszy proces. Każdy proces musi dokładać do koloru. Gdy cala lewa jest na stole, każdy ocenia czy
linia bierze czy nie. Wtedy odkłada swoja kartę z oznaczeniem zebrana/oddana. Po $13$ lewach każdy z
graczy wypisuje na ekran swój wynik. Gdy wszyscy wypiszą swój wynik dealer musi sprzątnąć shmem.

## Etapy:

1. Zaimplementuj inicjalizacje stołu w pamięci dzielonej (funkcja table init). Proces gracza czeka na $3$ innych graczy i się kończy. Pamięć dzielona powinna być inicjalizowana tylko przez pierwszego gracza.
2. Każdy gracz po dołączeniu do stołu powinien otrzymać swoje miejsce przy stole (numer $0$, $1$, $2$ lub $3$). Po zebraniu się $4$ graczy przy stole należy prawidłowo zniszczyć obiekt pamięci dzielonej i zwolnić wszystkie zasoby. W tym celu zaimplementuj funkcje `table_destroy` oraz `table_close`.
3. Po zebraniu wszystkich czterech graczy każdy gracz powinien pobrać $13$ kart ze stołu i posortować $13$ kart w swojej ręce. W tym celu zaimplementuj funkcję `start_game`.
4. Dodaj logikę rozgrywki gracza. Przy dokładaniu kart musisz rozróżnić logikę wiodącego gracza (leading player) i dokładającego gracza. Po każdej lewie (zrzutce) należy ocenić która para graczy zebrała lewę. W tym celu zaimplementuj funkcje `play_trick` oraz `asses_result`.
