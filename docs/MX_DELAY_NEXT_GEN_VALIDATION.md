# Walidacja etapu 0 / GUI C stage 1

Data: 2026-11-04. Zakres kodu: szkielet konsoli i nawigacja, bez zmian DSP, ParamID, UID i serializacji.

## Wyniki lokalne

| Kontrola | Wynik |
|---|---|
| Portable Release CTest | PASS, 2/2: MXDelayDSP i MXDelayCompatibility; asercje DSP aktywne |
| Stan/presety | PASS: niezależne binarne fixture v1…v5, mapowanie algorytmów, head pan, migracja FX v4/v5, ponowny zapis identycznych bajtów, odrzucenie wszystkich skróceń i nie-finite/nieznanych wersji |
| Geometria | PASS: zoom 75/100/125/150% przy DPI 1/1.25/1.5/2; rounding i fallback |
| Native Linux VST3 Debug | PASS: bundle z zasobem GUI i moduleinfo; SDK `3cdf9ca5…`, GCC 14.2, CMake 3.31.6 |
| Steinberg validator na module Linux | PASS: 47 tests passed, 0 failed |
| ASan + UBSan + LeakSanitizer, istniejący MXDelayDSPTests | PASS; wykonano poza sandboxem, w którym LeakSanitizer nie działa pod ptrace |
| Kontrola zasobów względem main | PASS: zachowane wszystkie istniejące mapowania nazw→tag i instancje interaktywnych regulatorów; XML parsuje się |
| Memory, DSP-only Debug | PASS: 1/1 suite, 1.89 s |
| Reel, DSP-only Debug | PASS: 1/1 suite, 73.04 s |
| Oil, DSP-only Debug | PASS: 1/1 suite, 22.49 s |
| Natywny Linux GUI/Xvfb | FAIL podczas attach, przed interakcjami; patrz diagnoza poniżej |
| Powtarzalność setup | PASS: lokalny sysroot z podpisanych pakietów Debian, przypięte CMake/Ninja/SDK; ponowne wykonanie instalatora |

Źródłowe testy silników wykonano osobno, bez integracji z MX i bez JUCE oversampling Oil. Wynik nie potwierdza kompletności nowych silników w MX. Fixture stanów są syntetycznymi historycznymi layoutami, nie archiwum rzeczywistych projektów FL Studio.

## Diagnoza Linux GUI

Dodany test otwiera prawdziwy moduł VST3 w X11, ma sprawdzać dwa cykle edytora, trzy skale, wszystkie zakładki, hit-testing ukrytych kontrolek, brak automatyki UI-only oraz reset. Te kontrole interakcji **nie zaliczyły się jeszcze**.

GDB wskazuje błąd zależności w przypiętym VSTGUI `5db272256172557818b6158cf0bb2c4410bddb25`: `X11::Frame::Frame` w x11frame.cpp najpierw wywołuje `RunLoop::init()`, a dopiero później `LinuxFactory::setRunLoop(cfg->runLoop)`. W init x11platform.cpp:131 dereferencjonuje pusty `RunLoop::get()`. Backtrace: RunLoop::Impl::init → X11::Frame::Frame → LinuxFactory::createFrame → CFrame::open → VST3Editor::open → MXDelayEditor::open.

Nie zmodyfikowano źródeł SDK/VSTGUI ani nie zamieniono testu na pozornie przechodzący. Windows nie korzysta z tego backendu; wymaga osobnej walidacji. Nie ogłaszać środowiska w pełni gotowym do natywnego Linux GUI. Wcześniejsze problemy infrastrukturalne (socket X11 w sandboxie i niekanoniczna ścieżka bundle w nowym hoście testowym) zdiagnozowano osobno; naprawa ścieżki nie usuwa opisanego błędu upstream.

## Niepotwierdzone / poza zakresem

- Natywny Windows build i zmodyfikowany MXDelayGUI: wynik CI zostanie dopisany po wykonaniu.
- FL Studio: nie uruchomiono; brak potwierdzenia okna, interakcji, automatyki, DPI, zapisu i odtwarzania projektów w tym DAW.
- pluginval na nowym MX: nie uruchomiono.
- Nowe Memory/Reel/Oil w MX, Algorithmic Reverb, Convolution IR, Hybrid: niezaimplementowane.
- Neutralny reset wszystkich parametrów, pełny zakres 70–200%, mierniki audio, preset browser: niezaimplementowane w tym szkielecie. Reset nadal przywraca factory default.
- CPU/RAM: nie wykonano porównawczego benchmarku ani optymalizacji; nie zmieniono toru audio. Dostępne rachunki pamięci są w audycie.
- Brak pomiarów widma aliasów, odsłuchu porównawczego ze sprzętem i wielogodzinnej próby DAW.

## Powtórzenie

Aktywacja przygotowanej maszyny: `source /workspace/shared/mx-cloud-env.sh`.

```sh
cmake -S . -B build/portable -G Ninja -DJERZY_BUILD_PLUGIN=OFF -DCMAKE_BUILD_TYPE=Release
cmake --build build/portable --parallel 3
ctest --test-dir build/portable --output-on-failure
```

Natywny Windows: CMake/VS2022, build targety `JerzyMXAnalogDelay2 MXDelayDSPTests MXDelayCompatibilityTests MXDelayGUITests`, następnie `ctest --test-dir build -C Release --verbose --output-on-failure`. Pipeline repo buduje i przechowuje rzeczywiste rendery okna. Nie utożsamiać automatycznego hosta testowego z FL Studio.

Konfigurację instalacji i aktywacji zapisano w roboczych polach środowiska `install_script` / `start_skill`; zapis nie jest publikacją. Niczego nie scalono do main.
