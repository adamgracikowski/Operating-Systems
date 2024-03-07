# Gra karciana:

Symulacja gry komputerowej dla wielu graczy. Program przyjmuje dwa argumenty będące liczbami naturalnymi: $2 ≤ N ≤ 5$ oraz $5 ≤ M ≤ 10$.

## Zasady gry:

W zadaniu symulujemy grę karcianą. Główny proces - nazywany dalej serwerem, tworzy $N$ procesów potomnych - nazywanych dalej graczami. Komunikacja między serwerem a graczami odbywa się za pomocą łączy nienazwanych (pipe). Każdy z procesów graczy ma na starcie do dyspozycji $M$ kart ponumerowanych od 1 do $M$. Serwer cyklicznie oznajmia graczom rozpoczęcie nowej rundy gry. Każdy z graczy musi wybrać i wysłać do serwera jedną (przyjmijmy losową) ze swoich pozostałych kart (nie może już jej użyć ponownie). Rundę zwycięża gracz z najwyższą kartą i otrzymuje $N$ punktów. W przypadku remisu punkty są dzielone między zwycięskich graczy zaokrąglając w dół (np. gdy przy 5 graczach mamy 2 zwycięzców otrzymają oni po dwa punkty). Po $M$ rundach gra się kończy i zwycięża gracz z największą liczbą punktów.

Dodatkowo:
- Wszystkie wiadomości wysyłane między serwerem a graczami mają zawsze 16 bajtów (w razie potrzeby można je uzupełniać zerami).
- Na początku każdej rundy serwer wypisuje na terminal `NEW ROUND [round_number]` i przesyła do każdego z graczy wiadomość `new_round`.
- Po otrzymaniu kart od wszystkich graczy serwer wyznacza zwycięzce(ów) rundy na podstawie zasad opisanych wyżej. Odsyła do każdego z graczy liczbę zdobytych przez niego punktów (w przypadku gdy gracz przegrał rundę wysyłane jest 0). Ponadto proces serwera wypisuje na terminal informację o tym, który gracz wysłał jaką kartę.
- Po odbyciu $M$ rund procesy graczy powinny się zakończyć. Proces serwera wypisuje tabelę wyników, oczekuje na zakończenie działania wszystkich procesów potomnych (graczy) i również kończy działanie.
- W przypadku gdy serwer nie może wysłać albo odebrać wiadomości do procesu gracza powinien wypisać komunikat na terminal oraz kontynuować grę bez tego gracza.
- Należy zamykać wszystkie nieużywane końce pipe-ów (oraz oczywiście zwolnić wszystkie inne zasoby gdy nie są już potrzebne).

## Etapy:
1. Poprawna inicjalizacja programu, tworzenie procesów graczy oraz pipów.
2. Każdy z procesów graczy przesyła do serwera losową liczbę naturalną $≤ M$, proces serwera wypisuje na terminal komunikaty postaci `Got number <X> from player [player_index]`.
3. Implementacja rund - serwer wysyła cyklicznie do graczy informacje o nowej rundzie, gracze w odpowiedzi odsyłają losową kartę.
4. Pełna implementacja zasad gry opisanych wyżej.
5. Po rozpoczęciu rundy proces gracza ma 5% szans na "awarię" (przedwczesne zakończenie). Proces serwera prawidłowo reaguje na taką sytuację.
