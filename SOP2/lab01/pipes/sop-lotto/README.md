# Lotto:

Napisz program symulujący loterię Lotto, używając procesów komunikujących się za pomocą łącz pipe.

Program przyjmuje dwa parametry: $N$ - liczbę graczy ($N ­>= 1$) oraz $T$ - liczbę tygodni symulacji ($T ­>= 1$).

## Zasady gry:

Loteria polega na wytypowaniu wyników losowania 6 liczb z zakresu od 1 do 49. Podczas jednego losowania raz w tygodniu losowanych jest 6 z 49 liczb. Koszt zakładu ustalamy na 3 zł. W zależności od ilości wytypowanych liczb nagrody są następujące:

| Ilość trafień: | Kwota wygranej: |
|---------------|----------------|
| 6             | 10'000'000     |
| 5             | 6'000          |
| 4             | 160            |
| 3             | 24             |
| 2             | 0              |
| 1             | 0              |
| 0             | 0              |

## Etapy:
1. Główny proces (Totalizator Sportowy) tworzy $N$ procesów graczy. Każdy z procesów graczy wypisuje na standardowe wyjście: `[process id]: I’m going to play Lotto!`.
2. Gracze komunikują się z Totalizatorem Sportowym za pomocą łączy pipe. Każdy z graczy wysyła przez swoje łącze swoje wytypowane liczby. Po zebraniu wszystkich zakładów Totalizator Sportowy odsyła wszystkim graczom wylosowane liczby. Totalizator Sportowy wypisuje na standardowe wyjście `Sport Totaliser: [process id] bet: [numbers]$` po przyjęciu zakładu, po czym
wypisuje `Sport Totaliser: [numbers] are today’s lucky numbers`. Po jednym tygodniu wszyscy gracze i Totalizator Sportowy kończą pracę.
3. Gra jest kontynuowana przez $T$ tygodni. Przy wygranej gracz wypisuje `[process id]: I won [amount]$`. Po wyjściu wszystkich graczy Totalizator Sportowy wypisuje statystyki ile gracze wydali na zakłady i ile wydano łącznie nagród.
4. Podczas każdego tygodnia każdy z graczy ma 10% szans na zrezygnowanie z dalszej gry. Proces gracza kończący wcześnie wypisuje `[process id]: This is a waste of money` i kończy pracę.

## Autor zadania:
Autorem i pomysłodawcą zadania jest [Tomasz Herman](https://github.com/tomasz-herman).
