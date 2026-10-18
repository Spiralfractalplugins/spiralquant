# How SpiralQuant Works

## 1. Общий signal path

```text
MIDI note
  -> QuantumVoice (per voice)
      -> QuantumOsc (complex wavefunction psi[x])
          -> phase readout -> wavetable sample
      -> ADSR envelope
      -> resonant low-pass
      -> drive / saturation
      -> voice gain
  -> QuantumEngine (sum up to 8 voices)
  -> MasterFx (optional Flanger / Phaser / Bitcrush / Fractal Entropy)
  -> stereo output
  -> scope: |psi(x)| + |psi~(k)| spectrum from first active voice
```

Плагин генерирует звук не из классической wavetable, а из численного решения
1D time-dependent Schrödinger equation на сетке из 128 точек.

## 2. `QuantumOsc`: волновая функция

`psi[x]` — комплексное значение на координатной сетке:

```text
x_i = 2 * i / (N - 1) - 1,   i = 0..127, N = 128
```

### Начальное состояние
В зависимости от `waveType` генерируется:
- Gaussian с центром `waveShift`, шириной от `waveScale`, фазой от `waveImpulse`;
- Sine/Cos с волновым числом от `waveScale`;
- Parabola, Saw, Square.

После генерации функция нормализуется:

```text
sum |psi[i]|^2 -> 1
```

### Потенциал
`potential[i]` строится по `potentialType`:
- Flat
- Harmonic
- InfiniteWell
- Barrier
- DoubleWell
- Linear

Параметры `potentialShift`, `potentialScale`, `potentialHeight` меняют положение,
жёсткость/ширину и масштаб потенциала.

## 3. Эволюция по времени: split-step Fourier

Каждый timestep:

1. Половина потенциального шага в координатном пространстве:
   `psi *= exp(-i * dt * V(x))`
2. FFT в импульсное пространство.
3. Кинетический шаг:
   `psi[k] *= exp(-i * (2*pi/N)^2 * dt * k^2)`
4. Обратное FFT в координатное пространство, если `showFFT == false`.
5. Нормировка `|psi|^2 -> 1`.
6. При барьерах/бесконечной яме маска обнуляет волновую функцию в запрещённых областях.
7. При `jumpRate > 0` — стохастический квантовый скачок (Monte-Carlo wavefunction):
   применяется оператор уничтожения фонона `a psi = (x/sigma) psi + sigma * dpsi/dx`,
   и с вероятностью `~ jumpRate * |a psi|^2 * dt` волна коллапсирует на `a psi` и
   заново нормируется. RNG сеидруется по номеру голоса — рендеры воспроизводимы.

`showFFT` включает работу в импульсном пространстве: wavetable читается из
моментальной representation.

## 4. Чтение wavetable
Фаза осциллятора:

```text
phase += (frequency / sampleRate) * N
```

Сэмпл:

```text
psiInterpolated = (1 - frac) * psi[i0] + frac * psi[i1]
```

`sampleMode` решает, что вернуть:
- Real
- Imag
- Magnitude
- Phase
- PhononQND (лестница из 8 ступеней по Re psi — квантовое nondemolition-считывание)

`frftAngle` поворачивает квадратуру считывания: смесь координатного сэмпла с
импульсным `psi~` (берётся из последнего FFT — бесплатно) под тем же индексом
фазы. 0 = psi(x), 1 = psi~(k). В режиме `showFFT` не действует (уже импульсное
пространство).

`samplesPerTimestep` определяет, через сколько аудиосэмплов делается один
квантовый timestep. `timestepDelta` — длительность квантового шага.

## 5. Голос
Каждый голос:
- хранит playing frequency и делает плавный glide к target frequency;
- при `noteOn` запускает осциллятор (`trigger()`), если голос был idle;
- envelope ADSR;
- IIR low-pass с динамическим пересчётом коэффициентов;
- опциональный drive/насыщение;
- gain.

При `noteOff` envelope переходит в Release, после чего голос становится idle.

## 6. Полифония
`QuantumEngine` хранит 8 голосов. При Note On:
- сначала ищет idle voice;
- если все заняты, крадёт самый старый активный голос.

Суммирование идёт в mono, затем копируется во все выходные каналы.

Аккордовый режим (`chordMode`) клонирует ноту в строй; `chordSuperpose` включает
квантовую суперпозицию: на note-on заводятся *два* строя с весами cos(phi) и
sin(phi), note-off — измерение, которое выбирает победителя с вероятностью cos^2(phi)
(он уходит в release), а проигравшего глушит `kill()` мгновенно. Запись суперпозиции
освобождается, так что застреваний голоса нет.

## 7. Master FX
После суммы голосов применяется выбранный эффект:
- Off
- Flanger (`juce::dsp::Chorus`)
- Phaser (`juce::dsp::Phaser`)
- Bitcrush (снижение разрядности и sample-rate)
- Fractal Entropy (хаотическая модуляция + entropy noise)

`fxMix`, `fxDepth`, `fxRate`, `fxFeedback`, `fxBits`, `fxEntropy` управляют
эффектами. Solo режимы: Full / Dry / Wet.

## 8. UI и scope
`QuantumEngine` пишет снапшоты `|psi|`, `|psi~|` и `V(x)` первого активного голоса
в двойной буфер (`scopePos/scopeMom/scopePot [2][128]`) с атомарной сменой слота и
счётчиком последовательности; аудио-поток никогда не блокируется. UI опрашивает
`snapshotScope()` на таймере 30 Гц и перерисовывает холст только когда счётчик
изменился. Верхняя половина холста — `|psi(x)|` с оверлеем потенциала, нижняя —
спектр `|psi~(k)|` с гармонической сеткой (бин n = n·f₀, подписи в Гц после первой
ноты). Интерфейс использует direct manipulation: перетаскивание положения/ширины
волны и потенциала на графике, draggable ADSR-кривая, колесо мыши 1%/5% и двойной
клик для точного ввода на каждой ручке. Цвета берутся из активного скина
(`Skins::current()`: Dark / SciFi Neon / Console, параметр `uiSkin`).

## 9. State save
Параметры хранятся в `AudioProcessorValueTreeState`, поэтому automation и
state save/restore поддерживаются через XML.

## 10. Примечания
- `applyWavefunction` (тумблер QM) реально останавливает эволюцию: волна замирает,
  голос работает как статичный wavetable.
- `samplesPerTimestep` в UI подаётся через normalized-значения APVTS.
- `jumpRate`, `frftAngle`, `chordSuperpose` по умолчанию выключены (0/false), поэтому
  дефолтный звук и бенчмарк не меняются; RNG коллапсов сеидруется по индексу голоса,
  так что тесты воспроизводимы между запусками.
