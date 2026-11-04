# MX Delay — zatwierdzony kierunek rozwoju i GUI C (Modular Console)
Data decyzji: 2026-11-04. Status: zatwierdzona koncepcja i wymagania; NIE oznacza wdrożenia DSP/GUI ani pozytywnych testów.
Źródło: ustalenia z użytkownikiem w rozmowie, raport „MX Delay — raport rozwojowy” z 2026-11-04. Ten dokument jest źródłem wykonawczym dla Codex.

## Decyzja GUI
Wybrano **wariant C — Modular Console** z planszy czterech makiet (A/B/C/D). Pozostałe A, B, D nie są kierunkiem docelowym.
Makieta C to referencja koncepcyjna, a nie dokładny pixel-perfect projekt: dwie symetryczne sekcje A/B, ciemna metalowa konsola, czytelne wyświetlacze, realistyczne metalowe gałki, pionowe mierniki, dolny pas INPUT / FX CHAIN / REVERB / OUTPUT, górna nawigacja A/B, FX, REVERB, ROUTING, presety. Nie kopiować fikcyjnych etykiet ani ograniczać liczby parametrów do pokazanych na grafice.
Priorytet: ergonomia i pełna funkcjonalność > ozdobniki. Wszystkie parametry dostępne w odpowiednich panelach/zakładkach; czytelność przy 100% i na mniejszych ekranach; skalowanie 70–200% (docelowy zakres do zweryfikowania). Odrębne sekcje zaawansowane per silnik, pełne jednostki ms/nuty/dB/Hz/%; wskazania wartości i aktywności muszą odzwierciedlać DSP, nie udawane animacje. Efekty wizualne bez alokacji i blokad w audio thread.
**Prawy klik** na każdym potencjometrze/sterowniku przywraca 0 albo neutralne ustawienie właściwe dla funkcji (np. PAN center). Nie utożsamiać bezrefleksyjnie neutralnego z wartością fabryczną; neutralną wartość określić dla każdego parametru. Reset automatyzowalny, bez trzasków; Shift = precyzyjna regulacja. Zachować dostępność obsługi i kontrast.

## Zakres DSP i silniki
Zachować istniejące dziewięć algorytmów MX Delay 2 (zweryfikować rzeczywiste enumy/parametry w kodzie; nazwy historyczne: Volante, El Capistan, Olivera, EC1, Brig, Deco, DIG, Space Echo 202, DM101).
Dodać **pełne niezależne silniki**, oparte na osobnych źródłowych projektach, a nie uproszczonych implementacjach z MultiFX:
- https://github.com/jurekbak71-dev/JerzyMemoryDelay — 6 modeli obwodów BBD/SAD, kompander, modulacja, synchronizacja.
- https://github.com/jurekbak71-dev/JerzyReelEchoVST3 — elektromechaniczne echo taśmowe, dwie głowice, mechanika.
- https://github.com/jurekbak71-dev/JerzyOilDrumEcho — elektrostatyczny tor ładunku, do czterech pickupów, rotor, tor barwy, kompresja, spring.
Najpierw przeprowadzić audyt kodu, licencji (AGPL/GPL/JUCE), parametrów i różnic architektonicznych. Nie kopiować bezmyślnie wrapperów VST3 i identyfikatorów pluginów; integrować DSP w jeden host i zachować tożsamość obecnego VST3. Zidentyfikować funkcjonalne nakładanie się silników, lecz nie usuwać zatwierdzonych niezależnych modeli bez zgody.

## Muzyczność i sterowanie jak sprzętem
Regulatory mają odwzorowywać przyczynowo-skutkowe zachowanie fizycznego mechanizmu, nie tylko mnożniki DSP:
- Tape: TIME/speed powoduje płynny pitch glide; zależność wow/flutter, nasycenia, histerezy, zużycia i filtrowania w kolejnych obiegach.
- BBD: clock/time wpływa na pasmo, filtrację, szum, kompander i modulację; różne obwody zachowują charakter.
- Oil can: pamięć elektrostatycznego nośnika i zanik ładunku; powtórki mogą występować bez klasycznego feedbacku; spacing głowic.
- Digital: czyste, vintage oraz kreatywne algorytmy; nie nakładać bez potrzeby identycznych „analogowych” artefaktów na wszystkie.
- FEEDBACK: muzyczna, użyteczna rozdzielczość gałki, bez niekontrolowanego wzrostu poziomu; zamierzone samooscylacje tylko z ochroną bezpieczeństwa.
- Ruch gałek: bez klików, z odpowiednim smoothing/interpolacją, ale bez zabijania naturalnego pitch glide. Stabilność przy automatyzacji i zmianie BPM.

## Tor i funkcje
Dwa niezależne silniki Delay A/B z istniejącymi trybami routingowymi (sprawdzić Series/Parallel/Split L/R i kompatybilność). Zachować FX CHAIN (Filter, Drive, Mod, Lo-Fi i dotychczasowe routingi; rzeczywisty zakres zweryfikować w kodzie). Zbadać opcje INPUT/PRE oraz WET ONLY dla dodatkowych efektów. Nie usuwać istniejących parametrów i presetów.

## Nowy REVERB
Całkowicie odrębny, odłączalny moduł na **końcu łańcucha**, osobna karta GUI i własne presety/parametry. Musi działać z delayami A/B lub **bez nich**, przy obu wyłączonych; niezależny bypass i prawidłowe tails/spillover.
Rodziny do zaprojektowania: Room/Chamber, Hall, Plate, Spring, Shimmer, Cloud/Ambient, Nonlinear/Gated, Granular, Modal/Resonator, FDN/Diffusion, Convolution IR, Hybrid. To zakres badań i propozycji; nie deklarować „wszystkich możliwych” bez audytu kosztów/realizowalności.
Tryby: algorithmic, convolution IR, hybrid DSP+IR; własne IR WAV, resampling, trim, gain/normalizacja, bezpieczny crossfade przy zmianie IR; partitioned FFT dla długich IR; w hybrid możliwe IR early reflections + algorytmiczny late tail. Sterowanie: Decay, Size, Damping, Predelay, Modulation, Width, Ducking, Freeze, Mix, Tone; zależnie od silnika. Uwzględnić mono/stereo, latency, CPU/RAM i kompatybilność hosta.

## Wymagania jakościowe i weryfikacja
Windows x64 VST3, FL Studio, GUI widoczne i responsywne, poprawna automatyzacja, zapamiętywanie stanu, presety starszych wersji, zachowanie identyfikatora VST3 i stabilnych ID parametrów. Bez alokacji, mutexów i operacji plikowych w audio thread; ochrona NaN/Inf, denormals, runaway feedback, stop/bypass/tails; testy float/double, 44.1/48/96 kHz, zmiany block size, tempo, obciążenia, długich renderów i re-open.
Wdrożenie etapami na osobnej gałęzi; testy jednostkowe DSP, integracyjne, pluginval i rzeczywiste sprawdzenie w FL Studio (jeśli brak dostępu, jawnie oznaczyć jako nieprzetestowane). Nie deklarować gotowości na podstawie kompilacji.
Kolejność rekomendowana do rozpisania przez Codex: 0 audyt i plan zgodności; 1 GUI C szkielet; 2 integracja 3 silników; 3 poprawa zachowania gałek/DSP; 4 Reverb algorytmiczny; 5 IR/Hybrid; 6 testy i optymalizacja. To kolejność robocza, nie harmonogram.

## Materiały źródłowe
Repo docelowe: https://github.com/jurekbak71-dev/JerzyMXAnalogDelayVST3
Repo porównawcze: https://github.com/jurekbak71-dev/JerzyMultiFX
Repo silników: trzy powyższe.
Makieta C: wybrana z planszy 2×2 wygenerowanej w rozmowie 2026-11-04; **plik graficzny nie został jeszcze dołączony do repozytorium**. Dołączony dokument jest specyfikacją tekstową; obraz użytkownik może przekazać Codex jako załącznik.
