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
