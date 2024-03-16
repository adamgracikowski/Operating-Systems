# Bingo:

Napisz program, który symuluje prostą wersję gry w BINGO. Losującym liczby jest proces rodzic, a graczami - jego procesy potomne. Komunikacja między nimi odbywa się za pomocą kolejek komunikatów POSIX. Proces rodzic tworzy n procesów potomnych ($0 < n < 100$, gdzie $n$ to parametr programu) oraz dwie kolejki komunikatów. Pierwsza kolejka `pout` służy do przekazywania co sekundę losowanych liczb z przedziału $[0, 9]$ do procesów potomnych, druga `pin` do odbierania od procesów potomnych informacji o wygranej lub zakończeniu gry.

## Zasady gry:

Procesy potomne na początku losują swoją liczbę oczekiwaną (wygrywającą) $E \in [0, 9]$ oraz $N \in [0, 9]$, która decyduje o tym, ile razy proces potomny odczyta liczbę z kolejki.
Następnie cyklicznie konkurują o dostęp do danych w kolejce `pout` - jedna wysłana liczba może być odebrana tylko przez jeden proces, a nie przez wszystkie naraz. Procesy potomne porównują odczytaną z pout liczbę do swojej liczby $E$ i, jeśli jest to ta sama liczba, to poprzez drugą kolejkę `pin` przekazują informację o jej wartości, a następnie kończą działanie. Po wykonaniu $N$ sprawdzeń proces potomny przed zakończeniem wysyła przez kolejkę pin swój numer porządkowy (z przedziału $[1,n]$).

Dodatkowo:
- Proces rodzica cały czas, asynchronicznie względem wysyłania liczb, ma odbierać komunikaty z `pin` i wyświetlać odpowiednie treści na ekranie. Gdy wszystkie procesy potomne zakończą działanie, proces rodzic również kończy działanie i usuwa kolejki.
- Rozmiar komunikatów w kolejce jest ograniczony do 1 bajta!
