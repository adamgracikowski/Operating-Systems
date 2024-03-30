# Gra planszowa:

Napisz dwa programy - klienta i serwera. Proces serwera ma jeden parametr: $3 < N \leq 20$. Po uruchomieniu najpierw wypisuje komunikat `My PID is: <pid>`. Następnie tworzy segment pamięci dzielonej o nazwie `<pid>-board` i rozmiarze $1024$ bajtów. W pamięci dzielonej umieszcza mutex, $N$ oraz planszę - tablicę bajtów rozmiaru $N \times N$ wypełnioną losowymi liczbami z zakresu $[1,9]$. Następnie co trzy sekundy wypisuje stan planszy. Po otrzymaniu `SIGINT` wypisuje stan planszy po raz ostatni i się kończy.

Program klienta przyjmuje dwa parametry - rozmiar tablicy oraz PID serwera. Otwiera obszar pamięci stworzone przez serwer. Następnie wykonuje następującą procedurę:

1. Blokuje mutex.
2. Losuje liczbę z zakresu $[1,10]$. W przypadku wylosowania $1$ wypisuje `Ops...` i się kończy.
3. Losuje dwie liczby $x$ i $y$ z zakresu od $0$ do $N-1$ i wypisuje `trying to search field (x,y)`.
4. Sprawdza jaka liczba znajduje się na planszy na polu o współrzędnych $(x,y)$.
5. Jeśli nie jest to zero program dodaje tę liczbę do sumy swoich punktów, wypisuje `found <P> points`, zeruje to pole, odblokowywuje mutex, czeka sekundę i wraca do kroku 1.
6. Jeśli jest to zero program odblokowywuje mutex, wypisuje `GAME OVER: score <X>` (gdzie $X$ to zdobyta suma punktów) i się kończy.
