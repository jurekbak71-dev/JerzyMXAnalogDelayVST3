# MX Delay Next Gen — etap 0 i zakres etapu 1

Data: 2026-11-04. Zlecenie: [Issue #11](https://github.com/jurekbak71-dev/JerzyMXAnalogDelayVST3/issues/11).
Kierunek: **GUI C — Modular Console**, zgodnie z [zatwierdzoną specyfikacją](MX_DELAY_NEXT_GEN_GUI_C_SPEC.md) i obrazem przekazanym w rozmowie.
Status: audyt źródeł i pierwszy szkielet edytora. Nie jest to ukończenie Next Gen ani integracja nowych silników/Reverb.

## 1. Źródła i granice audytu

| Projekt | Odczytana rewizja | Rola |
|---|---|---|
| MX Delay main | `aed456883fce4448afa9767852ff9eb82d147dca` | Punkt odniesienia 2.1.2 |
| Specyfikacja | `4586a4f3d948a315e704b1c1acf532fdb2c3705e` | Gałąź `docs/mx-delay-next-gen-gui-c-20261104` |
| JerzyMemoryDelay | `3a3c4238463688cc6f5499b30abb6a09e4c1f68f` | Pełny silnik BBD/SAD |
| JerzyReelEchoVST3 | `b6d03eccfaadb931df81cb342f8f2a4af739203b` | Pełny transport taśmy |
| JerzyOilDrumEcho | `f50340031f73afe01378adcbc82611a52645bc31` | Pełny silnik oil-can |
| Steinberg SDK do lokalnej walidacji | `3cdf9ca5d1f5b1b21e0a86832aa4abe55607bd96` | Rekurencyjny checkout, MIT |

Przejrzano specyfikację, treść issue i komentarze, źródła procesora, kontrolera, DSP, mechaniki, stanu, GUI i testów MX; dokumentację, opisy parametrów, źródła DSP/wrapperów, CMake i informacje licencyjne trzech projektów. Historyczne raporty walidacji tych projektów są deklaracjami dla ich rewizji, nie wynikami integracji w MX. Link Notion w issue nie jest dodatkowym zweryfikowanym źródłem; źródłem wykonawczym jest wskazana specyfikacja Git.

## 2. Obecna architektura i faktyczny tor

MX używa C++17, natywnego Steinberg VST3 i VSTGUI. Nie używa JUCE. `MXDelayProcessor` ma niezależne instancje `MXDelayDSP<float>` i `MXDelayDSP<double>`; każdy DSP ma dwa `SlotDSP` z osobnymi buforami, filtrowaniem, fazami i ziarnami PRNG. Wspólny jest BPM, stan parametrów i globalna kondycja maszyny. GUI nie jest właścicielem DSP; zamknięcie edytora nie usuwa pamięci audio.

Dziewięć zachowanych algorytmów (`mxdelay_params.h`, `mxdelay_dsp.h`):

| Indeks | Enum historyczny | Nazwa hosta | Aktywne kontrolki algorytmu na slot |
|---|---|---|---:|
| 0 | Volante | MultiHead Reel | 14 + 4 osobne panoramy głowic |
| 1 | ElCapistan | Tape Echo | 8 |
| 2 | Olivera | Oil Can | 6 |
| 3 | EC1 | Tube Echo | 6 |
| 4 | Brig | BBD | 6 |
| 5 | Deco | Doubletrack | 6 |
| 6 | DIG | Dual Digital | 6 |
| 7 | SpaceEcho202 | Space Echo 202 | 12 |
| 8 | DM101 | BBD Machine 101 | 8 |

Pola wspólne każdego slotu: Algorithm, Enable, Sync, Division, Time, Feedback, Level, Pan, Duck i cztery panoramy głowic. Wewnętrzne tablice przechowują również nieeksponowane/nieużywane pola — nie wolno ich usuwać z layoutu serializacji.

Tor: Input Trim → sloty FX z Route=INPUT/PRE → routing A/B → sloty FX z Route=WET ONLY → globalny dry/wet i Output Trim. PRE zmienia także gałąź dry w zwykłym przetwarzaniu. Są **cztery szeregowe sloty**, każdy OFF/Chorus/Flanger/LoFi/Overdrive/Granular; nie ma samodzielnego typu Filter. Nie podpisywać istniejącego racka fikcyjnymi czterema stałymi efektami z makiety. Granular to obecnie losowo zmieniany krótki tap, nie pełny silnik granularny z oknami ziaren.

- SERIES: A dostaje wejście; B dostaje `input + .72*A`; wet=`.72*(.48*A+B)`. To nie czysto szeregowe wet A→B.
- PARALLEL: oba dostają wejście; wet=`(A+B)/sqrt(2)`.
- SPLIT L/R: A dostaje dual-mono lewy, B dual-mono prawy; na wyjście trafiają A.left i B.right.
- Wyłączenie slotu czyści jego bufory i ucina ogon. Globalny bypass + spill podaje dry bez zmian oraz ogon wet, odcinając nowe wejście. Bypass bez spill resetuje oba sloty.
- Spring/Reverb w Tape Echo i Space Echo 202 to krótkie dodatkowe tapy w istniejącym delayu. Nie są nowym niezależnym Reverb.
- Mono/stereo i float/double są obsługiwane; inne układy kanałów są odrzucane. Host dostaje nieskończony tail. Nie raportuje się nowej latencji.

## 3. Kompatybilność VST3, automatyzacja i stan

Niezmienne kontrakty etapu 1:

- Processor UID `2F6C1A92-7BD84F0E-B4A3C911-E5D27A63` i Controller UID `8D3E5B21-C9404A7F-A26B19D4-5F70E3C8`.
- ParamID: globalne `1000…1031` (1006 to read-only Host Tempo); A od 1100, B od 1400, stride 300; algorytm `base+20+16*index+control`; head pan `base+10…13`.
- 202 rejestrowane parametry, w tym 201 automatyzowalnych i jeden miernik BPM, wynikające z aktualnych pętli rejestracji. Nie zamieniać nazw/skal tylko dlatego, że nowa gałka ma podobny opis.
- Zachować nazwy klas/fabryki, mapowania MIDI, kolejność 9 algorytmów oraz ich normalizację przez 8. CC mapping nie jest pełnym MIDI Learn.
- UI-only tags 9101…9104 (zoom) i 9200…9203 (zakładki) nie stają się parametrami audio.

Stan v5 (`mxdelay_state.h`) ma 1384 bajty: wersja int32 LE, 7 globalnych floatów, 4×6 FX, następnie dla A i B po 9 wspólnych, 9×16 algorytmicznych i 4 panoramy. Nie zapisuje audio, faz ani historii ogonów. Odczyt v1…v5 odrzuca NaN/Inf i przycina wartości do 0…1:

| Wersja | Migracja |
|---|---|
| 1 | 7 algorytmów; domyślne panoramy i nowe pola |
| 2 | Dodatkowe panoramy głowic |
| 3 | Machine Condition |
| 4 | 9 algorytmów i jeden FX; odwrócona historyczna polaryzacja route przy migracji |
| 5 | Cztery sloty FX |

v1…v3 migrują indeks algorytmu z normalizacji przez 6 na normalizację przez 8. Nowe algorytmy muszą otrzymać odrębny stabilny selektor rodziny/engine, bez zmiany znaczenia istniejącego parametru Algorithm: dodanie pozycji do starej listy zmieni stare automatyzacje. Nowe bloki ID dopiero po sprawdzeniu kolizji i zamrożeniu mapy. Nowa wersja stanu musi zachować jawne czytniki v1…v5 i mieć długości sekcji, limity oraz transakcyjny odczyt do tymczasowej struktury.

**Ryzyka już obecne:** `readMXState` zmienia docelowy obiekt przed końcem odczytu; uszkodzony preset może pozostawić częściowy stan. Parametry fabryczne nie są wszędzie spójne: slot time `.20` w DSP vs 500 ms w kontrolerze; division `.50` vs indeks 4 w liście kontrolera. Zmiana tych wartości może zmienić nowe instancje/brzmienie — nie naprawiono po cichu w PR GUI.

Nie ma własnej przeglądarki banków presetów ani banku fabrycznego MX. Presety hosta opierają się na stanie VST3. Migracja presetów Memory/Reel/Oil wymaga jawnego importera mapującego fizyczne jednostki, nie kopiowania ich blobów XML ani identyfikatorów VST3. Nowa przeglądarka powinna oddzielić kompletny preset MX od presetów silnika, FX i Reverb, z raportem pominiętych pól. Nie wprowadzać fikcyjnego działającego menu presetów w etapie 1.

## 4. DSP, bezpieczeństwo real-time i koszty

Dobre podstawy: alokacja linii w `prepare`, cubic interpolation delayów, 35 ms wygładzenia czasu oraz 8 ms feedback/level; osobna mechanika A/B; nasycenie zapisu, ograniczanie feedbacku i ochrona wewnętrznego wet przed nie-finite. Feedback `.965*pow(n,2.55)` pozostaje poniżej jedności na poziomie bazowego gain, ale cały tor nieliniowy i sumy tapów wymagają testów ekstremów.

| Priorytet | Znalezisko w kodzie | Dalsze działanie |
|---|---|---|
| Wysoki | `process()` używa thread_local vector, reserve/push_back i `stable_sort`; brak limitu liczby punktów automatyki | Prealokowany per-instance bufor w setupProcessing; deterministyczny merge kolejek z zachowaniem offsetów, bez zgubienia zdarzeń; instrumentation alokacji całego procesora |
| Wysoki | Bypass bez spill resetuje duże bufory w każdym segmencie process; wyłączenie silnika czyści go w pojedynczym ticku | Reset logiczny/generacje lub rozłożony reset poza gorącą ścieżką; test kosztu najgorszego bloku |
| Wysoki | Mix, trims, routing, zmiana algorytmu i część przełączników nie mają crossfade | Nie obiecywać resetu bez trzasków; osobny etap smoothing i testów skoków |
| Wysoki | Nie-finite wejście może przejść przez dry; ochrona ring/wet nie chroni całego wyjścia | Sanitizacja granic wejścia/wyjścia, float/double, wszystkie routingi |
| Średni | Brak jawnego scope FTZ/DAZ w wrapperze MX | Profil denormals i kontrolowana polityka na wspieranych platformach |
| Średni | Powtarzane exp/pow/sin i współczynniki filtrów w tickach | Cache niezmiennych współczynników, aktualizacja po zdarzeniu; null testy oraz profil przed/po |
| Średni | Sync ma 10 nut; długie nuty przy wolnym BPM przekraczają ring i są clampowane | Wyświetlać requested/actual time; ustalić kompatybilne rozszerzenie bufora/zakresu |
| Średni | DIG także ma bazową modulację i record saturation | Audyt trybu clean; nie usuwać charakteru istniejącego presetu bez wersjonowania |

Bufory MX przy danym typie próbki to około `26*sampleRate*sizeof(T)` bajtów (dwa stereo delaye po 6 s + cztery stereo FX po .25 s). Przy 48 kHz: około 4.76 MiB float albo 9.52 MiB double; po przełączeniu precision oba zestawy mogą pozostać zaalokowane (~14.28 MiB). Przy 96 kHz wartości podwajają się. To rachunek buforów, nie pomiar RSS ani kosztu GUI. Nie alokować wszystkich przyszłych modeli obu rodzin bez budżetu.

W tym PR nie zmieniono DSP i nie deklaruje się optymalizacji CPU/RAM. Najbezpieczniej najpierw zmierzyć p50/p99/max czasu bloku i allocations przy 44.1/48/96 kHz, blokach 16/64/128/257/1024, automatyce i zmianach presetów; potem optymalizować bez zmiany krzywych/brzmienia.

## 5. Trzy niezależne silniki — rzeczywiste różnice i integracja

| Silnik | Zweryfikowana architektura | Parametry / stan / ograniczenia |
|---|---|---|
| Memory | `Jerzy::Engine`, SDK-independent C++17, per-sample double tick; dwa niezależne kanały, wspólny zmienny zegar charge-cell | 14 parametrów + overload; 6 modeli EH7500/SAD, Issue A/SAD, Squelch/SAD, Deluxe/MN, Boy/BL, Toy/BL; 21 nut, boost/squelch/mod enable. Stałe 2×8192 double (~128 KiB). Blend i Level są częścią silnika. Bypass ukrywa ogon; zmiana modelu fade-through-dry i synchroniczny clear. Stan wrappera v1…v3 nie jest stanem MX |
| Reel | `jerzy::TapeEngine`, niezależny od JUCE C++20, float inplace; przestrzenny ring, wspólny inercyjny motor i dwie głowice | 39 parametrów, 16 presetów, APVTS XML schema1 + seed/UI/program. ORIGINAL/STUDIO/EXPERIMENTAL, cassette, record bias/ADAA, splices/dropouts, short/long levels/pans, stop/dub/SOS/freeze, wetOnly/tails. Około 16*sr bajtów. ORIGINAL ogranicza część funkcji i czas do 215–1000 ms. Brak natywnego double API |
| Oil | `jerzy::Engine`, niezależny od JUCE C++17, float stereo inplace; charge ring do 8 s, 4 pickupy, mechanika i osobny spring | 80 parametrów (enum `key::count`), 21 presetów, APVTS XML. Head On/Level/Pan/Send, dwa kompresory, modulacja, MASTER ONLY, dry mechanics, space. Wrapper VST3 ma 2× JUCE IIR oversampling i raportowaną latencję; samo Engine::process nie odtwarza tej jakości/latencji. Stereo-only wrapper. MASTER ONLY zamraża charge ring |

Podobieństwo do obecnych BBD/Oil Can/MultiHead nie oznacza równoważności. Obecne modele pozostają. Nowe trzy pełne DSP będą wybierane **niezależnie w A i B**, nie staną się trzecim równoczesnym slotem delay.

Plan adapterów (jeszcze niezaimplementowany):

1. Jeden host MX zachowuje UIDs, event slicing, buses, state i MIDI. Każdy adapter ma `prepare/reset/update/processWet`, własny stan, scratch i telemetry. Nie przenosić fabryk, APVTS ani wrapperów VST3.
2. Oddzielić dry/mix/gain silników od globalnego miksu i routingu MX. Memory Blend=wet nie może niejawnie zmieniać smoothing/dry; Reel wetOnly/ORIGINAL i Oil MASTER ONLY wymagają osobnego kontraktu. Zapisać golden audio źródłowych pełnych torów przed refaktorem.
3. Wspólne DTO zdarzeń z ParamID→physical mapping. Zachować różnice zakresów czasu, feedbacku i nut; nie przeskalowywać 21 nut Memory do 10 nut MX stratnie. Porównać standalone vs adapter przy identycznym sample rate, ustawieniach, seed i wet-only.
4. Reel wymaga C++20 dla adaptera albo audytu ograniczonego portu, bez globalnego przepisywania GUI. Dla double hosta: świadoma decyzja o konwersji przez prealokowany float scratch vs pełny double port, z testami błędu i poziomu szumu; nie deklarować 64-bit wewnętrznego DSP przez sam wrapper.
5. Oil potrzebuje zachowania oversamplingu 2× i kompensacji opóźnienia także dla dry i drugiego slotu w Parallel/Split. Ustalić stały worst-case latency i PDC albo przygotowane poza audio zmiany latency z restartComponent(kLatencyChanged). Nie włączać natywnego 1× jako rzekomo równoważnego VST3 źródła.
6. Zmiany engine przygotować poza audio, oddać gotowy obiekt w granicy bloku; krótki crossfade, zdefiniowany ogon, bounded retirement bez destrukcji dużych buforów na audio. Zachować UI-open niezależne od audio.

## 6. GUI C i reset

Docelowo: szara metalowa konsola, symetryczne A/B z amber/cyan, czytelne wyświetlacze, pionowe mierniki z rzeczywistych pomiarów, dolny pas Input/FX/Reverb/Output. Żadnych udawanych poziomów ani nieaktywnych gałek sugerujących działające DSP.

Etap 1 zachowuje pełne panele dziewięciu algorytmów A/B, przenosi istniejący czteroslotowy rack na kartę FX i globalny routing/bypass/spill/condition na ROUTING. Input/Output/Mix pozostają stale dostępne w dolnej konsoli. REVERB jest wyłącznie stroną informującą o braku implementacji. Zakładki są stanem lokalnym edytora, nie automatyzacją audio. Nowy edytor startuje od A/B; nawigacja nie czyści ogonów.

Zachowano wyłączenie DirectComposition przed utworzeniem okna Windows oraz obserwację wyłącznie plugin-owned child HWND. Nie subclassować okna FL Studio. Zachowano obecny design grid 1280×790, hit testing i geometrię zoom/DPI. Obecny zakres to 60–160% przez constraint, przyciski 75/100/125/150%. Docelowe 70–200% wymaga testów małych ekranów i Windows DPI, nie tylko zmiany maxSize. Mierniki sygnału i preset browser pozostają do kolejnego etapu.

**Reset:** istniejący prawy klik ustawia `getDefaultValue()`, nie neutralne. Pozostawiono tę semantykę, jawnie opisaną w UI etapu 1; nie deklaruje się spełnienia docelowego kryterium neutralnego resetu. Proponowany katalog polityk do wdrożenia wraz ze smoothing:

| Funkcja | Neutralne / 0 do zatwierdzenia testem zachowania |
|---|---|
| Input/Output trim | 0 dB → norm .5 / .6 (Output default .5 daje -3 dB) |
| PAN, head PAN | środek → .5, bez fabrycznego spreadu |
| Feedback, Duck, Drive, Depth, Amount, noise/wear/fault | 0%, jeśli nie zmienia znaczenia trybu |
| Slot Level | unity → .8 w obecnym `level*1.25`, nie bezrefleksyjnie 0 |
| Mix | dry=0; wybór musi być jawny, nie factory .35 |
| Bias / neutral tone / ratio | środek lub unity właściwe dla danego modelu; nie jednolita wartość .5 dla wszystkich |
| Time / rate / cutoff | poprawna minimalna lub referencyjna fizyczna wartość, bo 0 ms/Hz może być nielegalne |
| Head play/send, enable, sync, routing, model | jawna polityka dyskretna, nie reset wszystkich do OFF |

Katalog musi objąć każdy aktywny ParamID, mieć testy jednostek i gestów beginEdit/performEdit/endEdit oraz nie naruszyć fabrycznych wartości `ParameterInfo`. Reset staje się automatyką istniejącego parametru; dopiero smoothing audio pozwoli potwierdzić brak klików. Shift jest dziedziczoną funkcją CKnob; sprawdzić drag na Windows, zamiast deklarować na podstawie kodu bazowego. Kontrast, kolejność focus/klawiatura i opisy dostępności wymagają osobnego sprawdzenia.

## 7. Niezależny Reverb — projekt, nie implementacja

Umieścić osobny moduł **po istniejącym torze delay/FX i globalnym miksie**, przed finalnym wyjściem/gain staging zgodnie z zamrożonym nowym diagramem. Legacy Reverb bypassed musi dawać bit-identyczny wynik starego toru, bez dodatkowego gain/delay. Osobna decyzja o przeniesieniu Output Trim, by nie zmienić presetów. Przy obu slotach delay OFF doprowadzić do nowego reverb sygnał wejściowy/dry niezależnie od legacy MX Mix; obecne wet-only przy obu OFF daje ciszę, więc nie wystarczy dopiąć reverb tylko do legacy wet.

- Algorithmic: na początek jeden dobrze przetestowany FDN/diffusion Room/Hall, Decay/Size/Damping/Predelay/Width/Mix, niezależne Enable/Tails. Spring z Oil nie jest automatycznie gotowym uniwersalnym Reverb MX. Plate, Chamber, Shimmer, Cloud, Gated, Granular i Modal wymagają odrębnych modeli i kosztów.
- IR: WAV decode, metadane i rozmiary z limitami, resampling, trim i normalizacja w workerze; przygotowane partitioned FFT, pierwsza krótka partycja dla małej latencji; pamięć zależna od długości, kanałów i wielkości partycji. Żadnych plików, FFT planów ani zwalniania IR w callbacku. Obsługa braku pliku, ścieżek względnych/hash, mono/stereo i true-stereo jako osobne świadome zakresy.
- Hybrid: IR early reflections + algorytmiczny late tail, time alignment, gain staging, dwa przygotowane stany do crossfade przy zmianie IR. Freeze/ducking/predelay nie są darmowymi dodatkami — testować zmiany i bezpieczeństwo pętli.
- Latency i PDC wspólne dla A/B/Reverb; dry/wet wyrównane. Presety muszą przechowywać ustawienia/identyfikator IR, a politykę przenośności embedded vs referenced ustalić z limitem danych.
- Test minimum: Reverb-only przy A/B OFF dla każdego routing/mix; independent bypass/tails; reset/transport; impulse/RT60/stereo; IR brak/uszkodzenie/samplerate; automation/crossfade; latency i CPU/RAM; state v1…v5 daje Reverb OFF.

## 8. Licencje i dystrybucja

| Składnik | Dowód w checkout | Wniosek przed integracją/dystrybucją |
|---|---|---|
| MX własne źródła | Brak głównego LICENSE na audytowanym main | Nie zakładać MIT ani prawa do zmiany licencji; ustalić prawa i politykę dystrybucji |
| Memory własne źródła | Brak głównego LICENSE; docs/licenses zawiera tylko SDK/VSTGUI | Wymaga wyjaśnienia licencji własnego DSP przed kopiowaniem do MX |
| Reel | README i THIRD_PARTY_NOTICES: AGPL-3.0-only, LICENSE | Samo niezależne DSP także objęte licencją projektu; usunięcie JUCE wrappera nie znosi copyleft |
| Oil | README: AGPL-3.0-or-later, LICENSE | Analogicznie; zachować notices i corresponding source |
| JUCE 8.0.6 w Reel/Oil | AGPLv3 lub commercial; projekty deklarują AGPL | Commercial JUCE nie przelicencjonuje kodu Reel/Oil; integracja ich DSP bez JUCE usuwa framework dependency, nie licencję źródeł |
| Pobrany Steinberg SDK | LICENSE.txt MIT na podanej rewizji | Zachować notice; main MX pobierał ruchome master; ten PR przypina audytowany commit, a VST3_SDK_ROOT nadal pozwala użyć własnego SDK |
| VSTGUI | BSD-like 3-clause notice w źródłach / Memory docs | Zachować copyright, disclaimer i warunki użycia nazwy |
| SDK w starszym JUCE | THIRD_PARTY_NOTICES Reel wskazuje GPLv3/proprietary | Nie mieszać tej deklaracji z aktualnym osobnym MIT SDK; sprawdzić dokładny vendored checkout przy budowaniu |
| IR i assety | Jeszcze nie wybrane | Osobny rejestr pochodzenia, praw do redystrybucji, notices; import użytkownika nie daje praw do dołączania IR do paczki |

Nie skopiowano DSP ani wrapperów z trzech projektów do MX w tym etapie. Integracja AGPL do zamkniętej dystrybucji wymaga praw od uprawnionych autorów albo zgodnej strategii open source z pełnym corresponding source i notices. Ustalenia licencyjne są warunkiem etapu integracji; nie blokują audytu i własnego szkieletu GUI.

## 9. Kolejność i bramki akceptacji

1. **Ten PR:** audyt, zachowanie kontraktów, działająca nawigacja i przeniesienie obecnych kontrolek; testy stanu v1…v5, istniejącego DSP oraz GUI. Brak nowych silników/Reverb.
2. Neutralny reset i smoothing, prawdziwe telemetry/mierniki, poprawne wartości czasu w sync, zakres 70–200%, focus/kontrast; testy DPI i FL Studio.
3. Po licencjach: adapter jednego pełnego silnika na raz, złote rendery, zachowanie wszystkich jego funkcji; nowy state/ParamID namespace, importer presetów, double/oversampling/PDC; potem dwa sloty i routing każdego zestawienia.
4. Jeden niezależny Algorithmic Reverb, ze szczególnym testem Reverb-only.
5. IR/Hybrid i dopiero kolejne rodziny efektów po budżecie CPU/RAM.
6. pluginval, natywny Windows build/test GUI, rzeczywiste FL Studio project save/reopen/automation/offline/resize/monitor DPI, długie sesje, null/alias/listening i profil p99. Żadnego merge do main bez zgody użytkownika.

Wyniki bieżących poleceń, ograniczenia i zakres gotowości: [raport walidacji](MX_DELAY_NEXT_GEN_VALIDATION.md).
