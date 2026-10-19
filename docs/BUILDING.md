# Сборка SpiralQuant

Требования (любая платформа):
- CMake ≥ 3.24
- Git
- C++17 компилятор (для нативных сборок)
- JUCE 8.0.14 в `vendor/JUCE`:

```bash
git clone --depth 1 --branch 8.0.14 https://github.com/juce-framework/JUCE.git vendor/JUCE
```

В JUCE 8 цель `SpiralQuant` — только общий код; форматы собираются отдельными целями
`SpiralQuant_VST3`, `SpiralQuant_Standalone`, плюс `SpiralQuantTests` (оффлайн-рендер,
аудио устройство не нужно).

## macOS (нативно)

```bash
cmake -S . -B build-release -DCMAKE_BUILD_TYPE=Release
cmake --build build-release --config Release --parallel
```

Собирается universal (`arm64;x86_64`), плагины копируются рядом с артефактами
(`JUCE_COPY_PLUGIN_AFTER_BUILD`). Артефакты — в
`build-release/SpiralQuant_artefacts/Release/`: `VST3/SpiralQuant.vst3`,
`AU/SpiralQuant.component`, `Standalone/SpiralQuant.app`.

## Windows и Linux с машины macOS/Linux (контейнер, без Windows)

`tools/windows-cross-build/cross-build.sh` собирает Windows VST3 через MSVC под
Wine ([image msvc-wine](https://github.com/mstorsjo/msvc-wine)) и нативный Linux
в одном amd64-контейнере Docker/OrbStack:

```bash
./tools/windows-cross-build/cross-build.sh          # обе платформы
./tools/windows-cross-build/cross-build.sh windows  # только Windows
./tools/windows-cross-build/cross-build.sh linux    # только Linux
```

Результат копируется в `build/windows/` и `build/linux/` (каталог `build/` в
`.gitignore`). Образ `spiralquant-msvc:1`, контейнер `spiralquant-msvc` и volume
`spiralquant-msvc-work` остаются между запусками; первая сборка на Apple Silicon
идёт через Rosetta и скачивает MSVC + Windows SDK (лицензия принимается
явно в `Dockerfile`: `vsdownload.py --accept-license`). Пересобрать контейнер
с нуля: `docker rm -f spiralquant-msvc && docker volume rm spiralquant-msvc-work`
(нужно, если репозиторий переехал в другую папку — привязка `/src` фиксируется
при создании контейнера).

Сборка заканчивается запуском `wine64 SpiralQuantTests.exe` — это и тесты, и
smoke-проверка того, что PE-бинарники вообще исполняются.

Что учтено в скриптах (без этого кросс-сборка не работает):

- **MinGW не поддерживается** — JUCE 8 проверяет это на уровне
  `juce_TargetPlatform.h` (`#error "MinGW is not supported"`), поэтому используется
  настоящий MSVC.
- Wine-префикс: `wine64` в Ubuntu 24.04 лежит в `/usr/lib/wine` (в образе
  симлинк в `/usr/local/bin`), нужен пакет `winbind`, иначе `cl` падает с
  `C1902 Program database manager mismatch`. В `system32` копируются
  `msdia140.dll` (с `regsvr32`), release-CRT, debug-CRT и `ucrtbased.dll` —
  последние нужны, потому что helper JUCE собирается в Debug и потом запускается
  под Wine.
- `patch_juce_vst3_helper.py` правит `vendor/JUCE/extras/Build/CMake/JUCEUtils.cmake`
  **только в копии внутри контейнера** (`/work/spiralquant`): вложенный configure
  VST3-manifest helper не получает `CMAKE_TOOLCHAIN_FILE` и запускает готовый
  .exe на сборочной машине. Патч привязан к разметке JUCE 8.0.14 — при смене
  версии якоря могут не найтись (скрипт тогда падает, а не молча портит файл).
  `vendor/JUCE` в репозитории остаётся нетронутым.
- Первый проход сборки может споткнуться на helper'е (глубокие каталоги `.obj`
  создаются по ходу), поэтому `build_target()` повторяет попытку один раз.
- `/utf-8` для MSVC включён в `CMakeLists.txt` — иначе символы `▾`, `→` и `ψ̃`
  в подписях редактора уходят как `?` (предупреждение C4566).
- Иконки для Windows в `resources/icons/` нет (только `.icns`), поэтому бандл
  собирается со стандартной иконкой JUCE.

## Windows нативно (Visual Studio 2022)

Требования: Windows 10/11 x64, VS 2022 с нагрузкой «Desktop development with C++»,
CMake ≥ 3.24, Git.

```bat
cmake -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release --parallel
```

Результат:
- `build\SpiralQuant_artefacts\Release\VST3\SpiralQuant.vst3` — скопируйте в
  `C:\Program Files\Common Files\VST3` (или укажите папку хосту).
- `build\SpiralQuant_artefacts\Release\Standalone\SpiralQuant.exe` — проверка
  звуком без хоста.
- Тесты: `cmake --build build --config Release --target SpiralQuantTests`, затем
  `build\SpiralQuantTests_artefacts\Release\SpiralQuantTests.exe`.

## Linux (Ubuntu 24.04 x64) нативно

```bash
sudo apt install build-essential cmake ninja-build pkg-config \
  libasound2-dev libjack-jackd2-dev libdbus-1-dev libfreetype-dev \
  libfontconfig-dev libx11-dev libxcomposite-dev libxcursor-dev libxdamage-dev \
  libxext-dev libxfixes-dev libxi-dev libxinerama-dev libxrandr-dev libxss-dev \
  mesa-common-dev

cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build -j --target SpiralQuant_VST3 SpiralQuant_Standalone SpiralQuantTests
build/SpiralQuantTests_artefacts/Release/SpiralQuantTests
```

Результат:
- `build/SpiralQuant_artefacts/Release/VST3/SpiralQuant.vst3/Contents/x86_64-linux/SpiralQuant.so`
  — в `~/.vst3/` или `/usr/local/lib/vst3/`.
- `build/SpiralQuant_artefacts/Release/Standalone/SpiralQuant`.

Для релиза бинарники стоит урезать (`strip SpiralQuant.so SpiralQuant`).

## Примечания
- Формат **Audio Unit существует только на macOS/iOS** — под Windows и Linux сборка
  ограничена VST3 (и Standalone). Это ограничение платформы, а не проекта: JUCE
  автоматически пропускает AU на других целевых платформах.
- Для x86 (32 бита) вместо `-A x64` используйте `-A Win32`, но современные хосты
  требуют x64.
- Бинарники из контейнера не подписаны; перед распространением Windows-сборку
  нужно проверить в реальном DAW — тесты проверяют только DSP-путь.
- Тесты (`tests/TestRender.cpp`) собираются и проходят на всех платформах; под
  Windows они идут через `/fp:fast` (MSVC) вместо `-O3 -fno-math-errno` (clang/GCC),
  так что сравнивать бит-в-бит с macOS-рендером некорректно — проверяются
  конечность, тишина и физические инварианты.
