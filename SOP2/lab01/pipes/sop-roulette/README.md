# Ruletka:

Napisz program symulujący grę w ruletkę, używając procesów komunikujących się za pomocą łączy nienazwanych (pipe). 

Program przyjmuje dwa parametry: $N$ - liczbę graczy ($N \geq 1$) oraz $M$ - startową sumę pieniędzy ($M \geq 100$).

## Zasady gry:

W tej wersji ruletki gracz może obstawić zakład wyłącznie na pojedynczy numer (0-36) z wypłatą 35:1. Wyłonienie zwycięzcy polega na wylosowaniu liczby z zakresu 0-36 poprzez zakręcenie kołem ruletki.

## Etapy:
1. Główny proces (krupier) tworzy $N$ procesów graczy. Każdy z procesów graczy wypisuje na standardowe wyjście: `[process_id]: I have [amount]$ and I'm going to play roulette!`.
2. Gracze komunikują się z krupierem za pomocą łączy pipe. Każdy z graczy wysyła do krupiera losową (niezerową) kwotę zakładu (na którą go stać) i wybrany numer. Po zebraniu wszystkich zakładów krupier odsyła wszystkim graczom wylosowany numer (0-36). Krupier wypisuje na standardowe wyjście: `Croupier: [process_id] placed [amount]$ on a [number].`. po przyjęciu zakładu, po czym wypisuje: `Croupier: [number] is the lucky number.`. Po jednej rundzie wszyscy gracze i krupier kończą pracę.
3. Gra jest kontynuowana tak dopóki wszyscy gracze nie stracą wszystkich swoich pieniędzy. Jeżeli graczowi skończą się pieniądze, wypisuje on: `[process_id]: I'm broke!` i kończy pracę. Przy wygranej natomiast, gracz wypisuje: `[process_id]: Whoa, I won [amount]$!`. Po wyjściu wszystkich graczy krupier wypisuje: `Croupier: Casino always wins!` i kończy pracę.
4. Podczas każdej tury każdy z graczy ma 10% szans na zrezygnowanie z dalszej gry. Proces gracza kończący wcześnie wypisuje: `[process_id]: I saved [amount]$!` i kończy pracę.
