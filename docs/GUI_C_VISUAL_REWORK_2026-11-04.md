# GUI C — korekta względem zaakceptowanej makiety (2026-11-04)

## Materiał porównawczy
Zatwierdzony jest wariant **C / Modular Console** (lewy dolny z planszy czterech koncepcji). Użytkownik przesłał zrzut obecnego GUI 2026-11-04 i odrzucił jego wygląd jako niezgodny z projektem. Makieta C pokazuje zintegrowaną metalową konsolę z dwoma sąsiadującymi modułami A (ciepły bursztyn) i B (chłodny turkus), większymi gałkami podstawowymi, mniejszymi drugorzędnymi, czytelnymi ekranami czasu i podziału, pionowymi miernikami oraz jednolitym pasem wejścia, FX, reverb i wyjścia.

## Błędy dotychczasowego etapu
- Ogromne czarne pola pod algorytmami, w tym pusta przestrzeń w BBD.
- Jednolite, zbyt małe gałki, nieczytelna hierarchia.
- Zbyt wiele ciężkich obramowań, słaba integracja sekcji.
- Brak spójnej metalowej powierzchni pod parametrami zaawansowanymi.
- Brak realnych wskaźników poziomu i wizualnego sygnału.
- Sekcja Reverb wyłącznie planowana; nie przedstawia działającego DSP.
- Tekst sugeruje reset do fabrycznej wartości, a zatwierdzono zero / neutralną wartość zależną od parametru.

## Wykonane w tym etapie
- Każdy z 18 szablonów parametrów algorytmicznych otrzymał powierzchnię HardwarePanel zamiast czarnej pustki; subtelne akcenty Copper/Blue.
- Wprowadzono akcenty barwne A/B i poprawiono etykiety nagłówków oraz skrócono opisy.
- Dolny pas wskazuje cztery sekcje FX zamiast ogólnego „Four slots”.
- Zachowano wszystkie istniejące kontrolki, control-tag, sloty A/B, cztery zakładki i istniejący routing.
- Dodano statyczny test XML i kluczowych tagów: `python tests/test_gui_c_layout.py`.

## Kolejne etapy (nie są jeszcze wykonane)
1. Zastąpić niejednorodne elementy HardwarePanel jednym spójnym „machined metal chassis” z lekkimi, nieprzesadnymi detalami.
2. Przeprojektować geometrię dwóch modułów A/B: duże TIME/FEEDBACK, średnie LEVEL/MIX, małe modulacje; zachować wszystkie rzeczywiste parametry.
3. Dodać prawdziwe (telemetria DSP) mierniki L/R, bez atrap; dostosować kontrolę CPU.
4. Czytelny, wiarygodny display ms / BPM division: powiązać z rzeczywistymi danymi DSP, nie statyczny tekst.
5. Dodać interfejs reverb dopiero wraz z odpowiednimi parametrami i silnikiem, nie pozorować działającej funkcji.
6. Zaimplementować neutralny reset per parametr (obecny kod używa CControl::getDefaultValue() i może nie spełniać wymagania); z testami automatyzacji.
7. Windows VST3, testy renderowania GUI w FL Studio przy różnych skalach i DPI, weryfikacja z oryginalną makietą.

**Stan:** zmiana wyglądu i test statyczny zapisane w repozytorium, bez uruchomionej kompilacji i bez potwierdzonego zrzutu nowego GUI. To pierwsza poprawka, NIE finalna realizacja makiety C.


## Audyt źródeł trzech silników i Reverb — 2026-11-05

Potwierdzono rzeczywiste źródła, nie tylko nazwy algorytmów:

- Memory Delay: `jurekbak71-dev/JerzyMemoryDelay/source/dsp.h`, klasa `Jerzy::Engine`, metoda `tick(const double*, double*, int, const Parameters&, double tempo)`; zależność `source/parameters.h`. Własne przygotowanie buforów i stan układu; wymagany adapter stereo, mapowanie parametrów i bezpieczne przełączanie modelu.
- Reel Echo: `jurekbak71-dev/JerzyReelEchoVST3/Source/TapeEngine.h`, klasa `jerzy::TapeEngine`, metody `prepare/reset/set/process`; zależności `Source/Parameters.h` oraz implementacja `Source/TapeEngine.cpp`. Nie należy zastępować algorytmu zwykłym delayem z modulacją.
- Oil Drum Echo: `jurekbak71-dev/JerzyOilDrumEcho/src/Engine.h`, klasa `jerzy::Engine`; zależności `src/Parameters.h` i `src/Spring.h`. Integracja wymaga zachowania wielogłowicowości, mechaniki i obsługi sprężyny.
- MX Delay 2: obecne `source/mxdelay_params.h` zawiera 9 algorytmów na silnik A/B, po 16 pól na algorytm; identyfikatory `algoParam(slot, algo, control)` i format `source/mxdelay_state.h` wymagają migracji bez zmiany dotychczasowych identyfikatorów.

### Kryteria ukończenia (nie są jeszcze spełnione)

1. Skopiować i dostosować **oryginalny kod DSP** trzech silników z podaniem pochodzenia i licencji; utrzymać niezależny stan A/B, obsługę float/double i sample rate.
2. Rozszerzyć enum algorytmów, parametry kontrolera, mapowanie MIDI, automatyzację, zapis/odczyt presetów i testy migracji; istniejące ID muszą pozostać stabilne.
3. Reverb: niezależny procesor **po torze delay/FX**, z ON/OFF, dry/wet, pre-delay, decay, damping, width, freeze, trybami algorytmicznym, konwolucyjnym IR i hybrydowym; rzeczywista obsługa IR, bez statycznej atrapy GUI.
4. Powiązać makietę Modular Console z faktycznymi parametrami i telemetrią, bez fikcyjnych mierników, wyświetlaczy lub przełączników.
5. Potwierdzić Windows x64 CI, DSP, GUI, kompatybilność presetów, pluginval i test odsłuchowy w FL Studio.

**Stan 2026-11-05:** CI Windows #98 na gałęzi GUI C zakończyło się sukcesem dla dotychczasowego kodu (https://github.com/jurekbak71-dev/JerzyMXAnalogDelayVST3/actions/runs/41980153603). To NIE oznacza implementacji trzech silników ani Reverb; te moduły pozostają do zakodowania i przetestowania. Obecny wygląd GUI użytkownik odrzucił jako niezgodny z zatwierdzoną makietą.
