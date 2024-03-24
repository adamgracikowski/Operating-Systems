# Monte Carlo:

Napisz program obliczający liczbę PI metodą Monte Carlo. 
Program ma dwa argument pozycyjne $0 < N < 30$ - liczbę procesów obliczeniowych oraz $10^2 < M < 10^6$ - liczbę iteracji Monte Carlo, które wykonuje każdy z procesów obliczeniowych.

## Zasady:

Proces główny mapuje dwa obszary pamięci. Pierwszy obszar służy do współdzielenia wyników obliczeń procesów potomnych. Ma rozmiar $4N$ bajtów. Każdy proces potomny zapisuje wynik swoich obliczeń do jednej $4$-bajtowej komórki pamięci jako `float`. Drugi obszar pamięci jest mapowanie pliku `montecarlo_log.txt`. Proces główny ustawia rozmiar tego pliku na $8N$ bajtów. Następnie procesy obliczeniowe zapisują tam swoje wyniki końcowe w postaci tekstowej - każdy w jednej linii o szerokości $7$.
