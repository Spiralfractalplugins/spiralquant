#include <JuceHeader.h>
#include <cmath>
#include "PluginProcessor.h"
#include "gui/Palette.h"
#include "gui/LogoMark.h"

static void renderLogoImage (const juce::File& file, int size, bool withBackground)
{
    juce::Image img (juce::Image::ARGB, size, size, true);
    {
        juce::Graphics g (img);
        g.setColour (juce::Colours::transparentBlack);
        g.fillAll();
        SpiralQuantLogo::draw (g, juce::Rectangle<float> ((float) size, (float) size), withBackground);
    }
    if (auto stream = file.createOutputStream())
    {
        juce::PNGImageFormat png;
        png.writeImageToStream (img, *stream);
    }
}

static int runRenderLogo (const juce::String& outputDir)
{
    const juce::File dir (outputDir);
    dir.createDirectory();
    renderLogoImage (dir.getChildFile ("logo.png"), 1024, false);

    const int sizes[] = { 16, 32, 64, 128, 256, 512, 1024 };
    for (int s : sizes)
    {
        auto png = dir.getChildFile ("icon_" + juce::String (s) + ".png");
        renderLogoImage (png, s, true);
        renderLogoImage (dir.getChildFile ("icon_" + juce::String (s) + "@2x.png"), s * 2, true);
    }
    std::cout << "rendered logo + icons to " << dir.getFullPathName() << "\n";
    return 0;
}

/** Offscreen HUD renders, one per skin, for layout review and screenshot
    updates: --render-ui <dir> [w h] (default 1280x920). */
static int runRenderUi (const juce::String& outputDir, int w, int h)
{
    const juce::File dir (outputDir);
    dir.createDirectory();

    PluginProcessor proc;
    proc.prepareToPlay (48000.0, 512);
    const int numSkins = (int) Skins::all().size();
    for (int s = 0; s < numSkins; ++s)
    {
        if (auto* p = proc.apvts.getParameter ("uiSkin"))
            p->setValueNotifyingHost ((float) s / (float) (numSkins - 1));
        Skins::select (s);
        // A fresh editor picks the palette up in its constructor (the 20 Hz
        // skin timer has no message loop to run on here).
        std::unique_ptr<juce::AudioProcessorEditor> ed (proc.createEditor());
        if (ed == nullptr) { std::cerr << "no editor\n"; return 1; }
        ed->setSize (w, h);

        juce::Image img (juce::Image::ARGB, w, h, true);
        {
            juce::Graphics g (img);
            ed->paintEntireComponent (g, false);
        }
        ed.reset();
        auto png = dir.getChildFile ("ui-" + juce::String (w) + "x" + juce::String (h) + "-"
                                     + Skins::all()[(size_t) s].name + ".png");
        if (auto stream = png.createOutputStream())
        {
            juce::PNGImageFormat pngFmt;
            pngFmt.writeImageToStream (img, *stream);
        }
        std::cout << png.getFullPathName() << "\n";
    }
    return 0;
}

int main (int argc = 1, char** argv = nullptr)
{
    juce::ScopedJuceInitialiser_GUI gui;

    if (argc > 1 && juce::String (argv[1]) == "--lab-debug")
    {
        // Direct complex-FFT roundtrip sanity probe first.
        {
            juce::dsp::FFT probe { 8 };
            std::complex<float> a[256], b[256];
            double eIn = 0.0;
            for (int i = 0; i < 256; ++i)
            {
                a[i] = std::complex<float> (std::sin (juce::MathConstants<float>::twoPi * 3.0f * i / 256.0f), 0.0f);
                eIn += std::norm (a[i]);
            }
            probe.perform (a, b, false);
            double eFwd = 0.0;
            for (int i = 0; i < 256; ++i) eFwd += std::norm (b[i]);
            probe.perform (b, a, true);
            double eBack = 0.0;
            for (int i = 0; i < 256; ++i) eBack += std::norm (a[i]);
            std::cout << "fft probe: in=" << eIn << " fwd=" << eFwd << " back=" << eBack << "\n";
        }

        QuantumLab lab;
        lab.prepare (48000.0);
        lab.setParams (juce::String (argv[2]).getIntValue(), 0.7f, 0.7f, 0.6f, 0.6f);
        const int n = 512;
        juce::AudioBuffer<float> buf (2, n);
        double inRms = 0.0, outRms = 0.0;
        float outMax = 0.0f; bool nan = false;
        for (int blk = 0; blk < 100; ++blk)
        {
            for (int i = 0; i < n; ++i)
            {
                const float s = 0.4f * std::sin (juce::MathConstants<float>::twoPi * 261.6f
                                                 * (float) (blk * n + i) / 48000.0f);
                buf.setSample (0, i, s); buf.setSample (1, i, s);
                inRms += (double) s * s;
            }
            lab.process (buf);
            for (int i = 0; i < n; ++i)
            {
                const float o = buf.getSample (0, i);
                if (! std::isfinite (o)) { nan = true; break; }
                outRms += (double) o * o;
                outMax = juce::jmax (outMax, std::abs (o));
            }
            if (nan) break;
        }
        inRms = std::sqrt (inRms / (100.0 * n));
        outRms = std::sqrt (outRms / (100.0 * n));
        std::cout << "inRms=" << inRms << " outRms=" << outRms
                  << " outMax=" << outMax << " nan=" << (int) nan << "\n";
        return 0;
    }

    if (argc > 2 && juce::String (argv[1]) == "--render-logo")
        return runRenderLogo (argv[2]);

    if (argc > 2 && juce::String (argv[1]) == "--render-ui")
    {
        int w = 1280, h = 920;
        if (argc > 4) { w = juce::String (argv[3]).getIntValue(); h = juce::String (argv[4]).getIntValue(); }
        return runRenderUi (argv[2], w, h);
    }

    for (int wt = 0; wt < 6; ++wt)
    for (int pt = 0; pt < 6; ++pt)
    for (int sm = 0; sm < 5; ++sm)
    {
        PluginProcessor proc;
        proc.prepareToPlay (48000.0, 512);

        if (auto* p = proc.apvts.getParameter ("waveType")) p->setValueNotifyingHost ((float) wt / 5.0f);
        if (auto* p = proc.apvts.getParameter ("potentialType")) p->setValueNotifyingHost ((float) pt / 5.0f);
        if (auto* p = proc.apvts.getParameter ("sampleMode")) p->setValueNotifyingHost ((float) sm / 4.0f);
        if (auto* p = proc.apvts.getParameter ("samplesPerTimestep")) p->setValueNotifyingHost (0.2f);
        if (auto* p = proc.apvts.getParameter ("applyWavefunction")) p->setValueNotifyingHost (1.0f);

        juce::MidiBuffer midi;
        midi.addEvent (juce::MidiMessage::noteOn (1, 60, (juce::uint8) 100), 0);
        midi.addEvent (juce::MidiMessage::noteOn (1, 64, (juce::uint8) 90), 0);
        midi.addEvent (juce::MidiMessage::noteOn (1, 67, (juce::uint8) 90), 0);
        juce::AudioBuffer<float> buffer (2, 512);
        buffer.clear();
        proc.processBlock (buffer, midi);

        float rms = 0.0f;
        const float* d = buffer.getReadPointer (0);
        for (int i = 0; i < 512; ++i) { if (! std::isfinite (d[i])) { std::cerr << "NaN at wt=" << wt << " pt=" << pt << " sm=" << sm << "\n"; return 1; } rms += d[i]*d[i]; }
        rms = std::sqrt (rms / 512.0f);
        if (rms <= 0.0f) { std::cerr << "silent at wt=" << wt << " pt=" << pt << " sm=" << sm << "\n"; return 1; }
    }

    // state roundtrip
    {
        PluginProcessor proc;
        proc.prepareToPlay (48000.0, 512);
        juce::MemoryBlock mb;
        proc.getStateInformation (mb);
        PluginProcessor proc2;
        proc2.prepareToPlay (48000.0, 512);
        proc2.setStateInformation (mb.getData(), (int) mb.getSize());
    }

    // FX stress
    for (int fxt = 0; fxt < 5; ++fxt)
    {
        PluginProcessor proc;
        proc.prepareToPlay (48000.0, 512);

        if (auto* p = proc.apvts.getParameter ("waveType")) p->setValueNotifyingHost (0.0f);
        if (auto* p = proc.apvts.getParameter ("potentialType")) p->setValueNotifyingHost (1.0f / 5.0f);
        if (auto* p = proc.apvts.getParameter ("sampleMode")) p->setValueNotifyingHost (0.0f);
        if (auto* p = proc.apvts.getParameter ("samplesPerTimestep")) p->setValueNotifyingHost (0.2f);
        if (auto* p = proc.apvts.getParameter ("applyWavefunction")) p->setValueNotifyingHost (1.0f);
        if (auto* p = proc.apvts.getParameter ("fxType")) p->setValueNotifyingHost ((float) fxt / 4.0f);
        if (auto* p = proc.apvts.getParameter ("fxDepth")) p->setValueNotifyingHost (0.6f);
        if (auto* p = proc.apvts.getParameter ("fxRate")) p->setValueNotifyingHost (0.1f);
        if (auto* p = proc.apvts.getParameter ("fxFeedback")) p->setValueNotifyingHost (0.4f);
        if (auto* p = proc.apvts.getParameter ("fxMix")) p->setValueNotifyingHost (0.7f);
        if (auto* p = proc.apvts.getParameter ("fxBits")) p->setValueNotifyingHost (0.4f);
        if (auto* p = proc.apvts.getParameter ("fxEntropy")) p->setValueNotifyingHost (0.5f);

        juce::MidiBuffer midi;
        midi.addEvent (juce::MidiMessage::noteOn (1, 60, (juce::uint8) 100), 0);
        juce::AudioBuffer<float> buffer (2, 512);
        buffer.clear();
        proc.processBlock (buffer, midi);

        float rms = 0.0f;
        const float* d = buffer.getReadPointer (0);
        for (int i = 0; i < 512; ++i)
        {
            if (! std::isfinite (d[i])) { std::cerr << "FX NaN fxt=" << fxt << "\n"; return 1; }
            rms += d[i] * d[i];
        }
        rms = std::sqrt (rms / 512.0f);
        if (rms <= 0.0f) { std::cerr << "FX silent fxt=" << fxt << "\n"; return 1; }
    }

    // chord / pitch / tunnel matrix
    for (int chord = 0; chord < 2; ++chord)
    for (int ct = 0; ct < 8; ++ct)
    for (int tunnel = 0; tunnel < 2; ++tunnel)
    {
        PluginProcessor proc;
        proc.prepareToPlay (48000.0, 512);
        proc.apvts.getParameter ("samplesPerTimestep")->setValueNotifyingHost (0.2f);
        proc.apvts.getParameter ("chordMode")->setValueNotifyingHost ((float) chord);
        proc.apvts.getParameter ("chordType")->setValueNotifyingHost ((float) ct / 7.0f);
        proc.apvts.getParameter ("tunnel")->setValueNotifyingHost ((float) tunnel);
        proc.apvts.getParameter ("pitchShift")->setValueNotifyingHost (chord == 0 ? -1.0f : 1.0f);

        juce::MidiBuffer midi, idle;
        midi.addEvent (juce::MidiMessage::noteOn (1, 60, (juce::uint8) 100), 0);
        juce::AudioBuffer<float> buffer (2, 512);
        buffer.clear();
        for (int b = 0; b < 8; ++b)
            proc.processBlock (buffer, idle);
        proc.processBlock (buffer, midi);
        for (int b = 0; b < 8; ++b)
            proc.processBlock (buffer, idle);

        float rms = 0.0f;
        const float* d = buffer.getReadPointer (0);
        for (int i = 0; i < 512; ++i)
        {
            if (! std::isfinite (d[i])) { std::cerr << "NaN chord=" << chord << " ct=" << ct << " tunnel=" << tunnel << "\n"; return 1; }
            rms += d[i] * d[i];
        }
        rms = std::sqrt (rms / 512.0f);
        if (rms <= 0.0f) { std::cerr << "silent chord=" << chord << " ct=" << ct << " tunnel=" << tunnel << "\n"; return 1; }
    }

    // Wave 3: Zeno / squeeze / EPR matrix — finite and non-silent for every combo.
    for (int zn = 0; zn < 2; ++zn)
    for (int sq = 0; sq < 3; ++sq)
    for (int ent = 0; ent < 2; ++ent)
    {
        PluginProcessor proc;
        proc.prepareToPlay (48000.0, 512);
        proc.apvts.getParameter ("samplesPerTimestep")->setValueNotifyingHost (0.2f);
        proc.apvts.getParameter ("zeno")->setValueNotifyingHost ((float) zn);
        proc.apvts.getParameter ("squeeze")->setValueNotifyingHost ((float) sq / 2.0f);
        proc.apvts.getParameter ("entangle")->setValueNotifyingHost ((float) ent);
        if (ent != 0)
        {
            proc.apvts.getParameter ("jumpRate")->setValueNotifyingHost (0.5f);
            proc.apvts.getParameter ("chordMode")->setValueNotifyingHost (1.0f);
            proc.apvts.getParameter ("chordSuperpose")->setValueNotifyingHost (1.0f);
        }

        juce::MidiBuffer midi, idle;
        midi.addEvent (juce::MidiMessage::noteOn (1, 60, (juce::uint8) 100), 0);
        juce::AudioBuffer<float> buffer (2, 512);
        float rms = 0.0f;
        for (int b = 0; b < 16; ++b)
        {
            buffer.clear();
            proc.processBlock (buffer, b == 0 ? midi : idle);
            const float* d = buffer.getReadPointer (0);
            for (int i = 0; i < 512; ++i)
            {
                if (! std::isfinite (d[i])) { std::cerr << "NaN wave3 zn=" << zn << " sq=" << sq << " ent=" << ent << "\n"; return 1; }
                rms += d[i] * d[i];
            }
        }
        rms = std::sqrt (rms / (512.0f * 16.0f));
        if (rms <= 0.0f) { std::cerr << "silent wave3 zn=" << zn << " sq=" << sq << " ent=" << ent << "\n"; return 1; }
    }

    // Squeeze must physically reshape the packet: + squeezes position (variance
    // drops), - squeezes momentum (position variance grows). Flat potential and
    // slow evolution keep spreading out of the picture.
    {
        auto posVariance = [] (const float* pos, int n)
        {
            double tot = 0.0;
            for (int i = 0; i < n; ++i) tot += (double) pos[i] * pos[i];
            if (tot <= 0.0) return -1.0;
            double mean = 0.0;
            for (int i = 0; i < n; ++i) mean += i * ((double) pos[i] * pos[i]);
            mean /= tot;
            double var = 0.0;
            for (int i = 0; i < n; ++i) { const double d = i - mean; var += d * d * ((double) pos[i] * pos[i]); }
            return var / tot;
        };

        double vars [3] = { 0.0, 0.0, 0.0 };
        for (int sq = 0; sq < 3; ++sq)
        {
            PluginProcessor proc;
            proc.prepareToPlay (48000.0, 512);
            proc.apvts.getParameter ("waveType")->setValueNotifyingHost (0.0f);
            proc.apvts.getParameter ("potentialType")->setValueNotifyingHost (0.0f);
            proc.apvts.getParameter ("samplesPerTimestep")->setValueNotifyingHost (1.0f);
            proc.apvts.getParameter ("squeeze")->setValueNotifyingHost ((float) sq / 2.0f);

            juce::MidiBuffer midi, idle;
            midi.addEvent (juce::MidiMessage::noteOn (1, 60, (juce::uint8) 100), 0);
            juce::AudioBuffer<float> buffer (2, 512);
            for (int b = 0; b < 4; ++b)
            {
                buffer.clear();
                proc.processBlock (buffer, b == 0 ? midi : idle);
            }
            float pos [QuantumEngine::SCOPE_SIZE], mom [QuantumEngine::SCOPE_SIZE], pot [QuantumEngine::SCOPE_SIZE];
            proc.snapshotScope (pos, mom, pot);
            vars [sq] = posVariance (pos, QuantumEngine::SCOPE_SIZE);
            if (vars [sq] < 0.0) { std::cerr << "squeeze scope empty\n"; return 1; }
        }
        if (! (vars [2] < vars [1] && vars [0] > vars [1]))
        { std::cerr << "squeeze did not reshape variance: neg=" << vars [0]
                    << " zero=" << vars [1] << " pos=" << vars [2] << "\n"; return 1; }
    }

    // Scope snapshot: finiteness + Parseval. JUCE's forward FFT uses a fixed
    // scaling convention, so instead of pinning the absolute ratio we assert it
    // lands in a sane band and is identical for two different wavefunctions.
    {
        auto energy = [] (const float* v, int n)
        {
            double e = 0.0;
            for (int i = 0; i < n; ++i) { if (! std::isfinite (v[i])) return -1.0; e += (double) v[i] * v[i]; }
            return e;
        };

        float pos [QuantumEngine::SCOPE_SIZE], mom [QuantumEngine::SCOPE_SIZE], pot [QuantumEngine::SCOPE_SIZE];
        double ratios [2] = { 0.0, 0.0 };

        const int cfgs [2][2] = { { 0, 1 }, { 3, 4 } }; // waveType, potentialType
        for (int c = 0; c < 2; ++c)
        {
            PluginProcessor proc;
            proc.prepareToPlay (48000.0, 512);
            proc.apvts.getParameter ("waveType")->setValueNotifyingHost ((float) cfgs[c][0] / 5.0f);
            proc.apvts.getParameter ("potentialType")->setValueNotifyingHost ((float) cfgs[c][1] / 5.0f);
            proc.apvts.getParameter ("samplesPerTimestep")->setValueNotifyingHost (0.2f);

            juce::MidiBuffer midi, idle;
            midi.addEvent (juce::MidiMessage::noteOn (1, 60, (juce::uint8) 100), 0);
            juce::AudioBuffer<float> buffer (2, 512);
            proc.processBlock (buffer, midi);
            for (int b = 0; b < 4; ++b)
                proc.processBlock (buffer, idle);

            proc.snapshotScope (pos, mom, pot);
            for (int i = 0; i < QuantumEngine::SCOPE_SIZE; ++i)
                if (mom[i] < 0.0f) { std::cerr << "negative momentum bin\n"; return 1; }

            const double ep = energy (pos, QuantumEngine::SCOPE_SIZE);
            const double em = energy (mom, QuantumEngine::SCOPE_SIZE);
            const double et = energy (pot, QuantumEngine::SCOPE_SIZE);
            if (ep <= 0.0 || em < 0.0 || et < 0.0 || ep < 0.0 || em != em || et != et)
            { std::cerr << "scope energy NaN/negative\n"; return 1; }

            ratios[c] = em / ep;
            if (ratios[c] < 0.1 || ratios[c] > 10.0 * QuantumEngine::SCOPE_SIZE)
            { std::cerr << "Parseval ratio out of band: " << ratios[c] << "\n"; return 1; }
        }

        const double rel = std::abs (ratios[0] - ratios[1]) / ratios[0];
        if (rel > 0.01)
        { std::cerr << "Parseval ratio not state-independent: " << ratios[0] << " vs " << ratios[1] << "\n"; return 1; }
    }

    // Phonon update: collapse jumps, quadrature readout, superposed chords.
    {
        auto render = [] (const std::vector<std::pair<const char*, float>>& settings, bool noteOffLate)
        {
            PluginProcessor proc;
            proc.prepareToPlay (48000.0, 512);
            proc.apvts.getParameter ("samplesPerTimestep")->setValueNotifyingHost (0.2f);
            proc.apvts.getParameter ("release")->setValueNotifyingHost (0.0f); // 1 ms
            for (const auto& s : settings)
                if (auto* p = proc.apvts.getParameter (s.first))
                    p->setValueNotifyingHost (s.second);

            juce::MidiBuffer midi;
            midi.addEvent (juce::MidiMessage::noteOn (1, 60, (juce::uint8) 100), 0);
            juce::AudioBuffer<float> buffer (2, 512);
            proc.processBlock (buffer, midi);
            for (int b = 0; b < 16; ++b)
            {
                juce::MidiBuffer m;
                if (noteOffLate && b == 8)
                    m.addEvent (juce::MidiMessage::noteOff (1, 60, (juce::uint8) 64), 0);
                proc.processBlock (buffer, m);
            }
            return std::vector<float> (buffer.getReadPointer (0), buffer.getReadPointer (0) + 512);
        };

        auto stats = [] (const std::vector<float>& v) -> std::pair<double, int>
        {
            double rms = 0.0;
            for (float x : v)
            {
                if (! std::isfinite (x)) return { -1.0, 0 };
                rms += (double) x * x;
            }
            return { std::sqrt (rms / (double) v.size()), 0 };
        };

        const auto base = render ({}, false);
        if (stats (base).first <= 0.0) { std::cerr << "baseline silent\n"; return 1; }

        // (a) collapse: perturbs the wave, stays finite, reproducible across instances.
        const auto jump = render ({ { "jumpRate", 1.0f } }, false);
        const auto jump2 = render ({ { "jumpRate", 1.0f } }, false);
        if (stats (jump).first <= 0.0) { std::cerr << "collapse silent or NaN\n"; return 1; }
        if (jump != jump2) { std::cerr << "collapse not reproducible\n"; return 1; }
        if (jump == base) { std::cerr << "collapse had no effect\n"; return 1; }

        // (b) quadrature rotation and PhononQND staircase change the readout.
        const auto quad = render ({ { "frftAngle", 0.5f } }, false);
        if (quad == base || stats (quad).first <= 0.0) { std::cerr << "quadrature readout broken\n"; return 1; }
        const auto qnd = render ({ { "sampleMode", 1.0f } }, false); // normalised 1.0 = index 4
        if (qnd == base || stats (qnd).first <= 0.0) { std::cerr << "PhononQND readout broken\n"; return 1; }

        // (c) superposition: sounds while held, every voice stops after the collapse.
        const auto supOn = render ({ { "chordMode", 1.0f }, { "chordSuperpose", 1.0f } }, false);
        if (stats (supOn).first <= 0.0) { std::cerr << "superpose silent\n"; return 1; }
        const auto supOff = render ({ { "chordMode", 1.0f }, { "chordSuperpose", 1.0f } }, true);
        if (stats (supOff).first > 0.0) { std::cerr << "superpose left a voice sounding after note-off\n"; return 1; }
    }

    // Wave 2: resonator memory, cavity ringdown, quantum random walk melody.
    {
        auto run = [] (const std::vector<std::pair<const char*, float>>& settings, int blocks, bool hostNote)
        {
            PluginProcessor proc;
            proc.prepareToPlay (48000.0, 512);
            proc.apvts.getParameter ("samplesPerTimestep")->setValueNotifyingHost (0.2f);
            proc.apvts.getParameter ("release")->setValueNotifyingHost (0.0f); // 1 ms
            for (const auto& s : settings)
                if (auto* p = proc.apvts.getParameter (s.first))
                    p->setValueNotifyingHost (s.second);

            juce::AudioBuffer<float> buffer (2, 512);
            std::vector<float> out ((size_t) blocks * 512);
            for (int b = 0; b < blocks; ++b)
            {
                juce::MidiBuffer m;
                if (b == 0 && hostNote)
                    m.addEvent (juce::MidiMessage::noteOn (1, 60, (juce::uint8) 100), 0);
                proc.processBlock (buffer, m);
                std::copy (buffer.getReadPointer (0), buffer.getReadPointer (0) + 512,
                           out.begin() + (long) b * 512);
            }
            return out;
        };

        auto rmsBlock = [] (const std::vector<float>& v, int b) -> double
        {
            double e = 0.0;
            for (int i = 0; i < 512; ++i)
            {
                const float x = v[(size_t) b * 512 + (size_t) i];
                if (! std::isfinite (x)) return -1.0;
                e += (double) x * x;
            }
            return std::sqrt (e / 512.0);
        };

        const auto wBase = run ({}, 16, true);

        // (a) delayed self-coupling changes the evolution and stays finite.
        const auto wMem = run ({ { "memoryAmt", 1.0f }, { "memoryTime", 0.05f } }, 16, true);
        if (rmsBlock (wMem, 15) <= 0.0) { std::cerr << "memory silent or NaN\n"; return 1; }
        if (wMem == wBase) { std::cerr << "memory had no effect\n"; return 1; }

        // (b) ringdown: the readout fades while the note is held.
        const auto wRd = run ({ { "ringdown", 1.0f } }, 400, true);
        const double early = rmsBlock (wRd, 20), late = rmsBlock (wRd, 390);
        if (early <= 0.0 || late < 0.0 || late >= early)
        { std::cerr << "ringdown does not fade the readout: " << early << " -> " << late << "\n"; return 1; }

        // (c) the walk composes notes without host MIDI, deterministically.
        const auto w1 = run ({ { "walkOn", 1.0f } }, 200, false);
        const auto w2 = run ({ { "walkOn", 1.0f } }, 200, false);
        if (rmsBlock (w1, 199) <= 0.0) { std::cerr << "walk silent or NaN\n"; return 1; }
        if (w1 != w2) { std::cerr << "walk not reproducible\n"; return 1; }

        // Switching the walk off releases the held note: no stuck voices.
        {
            PluginProcessor proc;
            proc.prepareToPlay (48000.0, 512);
            proc.apvts.getParameter ("samplesPerTimestep")->setValueNotifyingHost (0.2f);
            proc.apvts.getParameter ("release")->setValueNotifyingHost (0.0f);
            proc.apvts.getParameter ("walkOn")->setValueNotifyingHost (1.0f);
            juce::MidiBuffer idle;
            juce::AudioBuffer<float> buffer (2, 512);
            for (int b = 0; b < 100; ++b)
                proc.processBlock (buffer, idle);
            proc.apvts.getParameter ("walkOn")->setValueNotifyingHost (0.0f);
            for (int b = 0; b < 60; ++b)
                proc.processBlock (buffer, idle);
            double e = 0.0;
            const float* d = buffer.getReadPointer (0);
            for (int i = 0; i < 512; ++i)
            {
                if (! std::isfinite (d[i])) { std::cerr << "walk-off NaN\n"; return 1; }
                e += (double) d[i] * d[i];
            }
            if (e > 0.0) { std::cerr << "walk left a voice sounding after switch-off\n"; return 1; }
        }
    }

    // Output limiter: on, a hot mix is bounded to full scale; off, it spikes past.
    {
        auto peakOf = [] (const std::vector<float>& v) -> float
        {
            float p = 0.0f;
            for (float x : v)
            {
                if (! std::isfinite (x)) return -1.0f;
                p = std::max (p, std::abs (x));
            }
            return p;
        };

        // Loud settings: max voice gain, full drive+saturation, chord of 3 voices.
        const std::vector<std::pair<const char*, float>> hot =
            { { "gain", 1.0f }, { "drive", 1.0f }, { "satMix", 1.0f },
              { "chordMode", 1.0f }, { "samplesPerTimestep", 0.2f } };

        auto hotPeak = [&hot, &peakOf] (bool limiterOn)
        {
            PluginProcessor proc;
            proc.prepareToPlay (48000.0, 512);
            proc.apvts.getParameter ("limiter")->setValueNotifyingHost (limiterOn ? 1.0f : 0.0f);
            for (const auto& s : hot)
                if (auto* p = proc.apvts.getParameter (s.first))
                    p->setValueNotifyingHost (s.second);
            juce::MidiBuffer midi, empty;
            midi.addEvent (juce::MidiMessage::noteOn (1, 60, (juce::uint8) 127), 0);
            juce::AudioBuffer<float> buffer (2, 512);
            std::vector<float> out;
            for (int b = 0; b < 32; ++b)
            {
                proc.processBlock (buffer, b == 0 ? midi : empty);
                out.insert (out.end(), buffer.getReadPointer (0), buffer.getReadPointer (0) + 512);
            }
            return peakOf (out);
        };

        const float off = hotPeak (false);
        const float on = hotPeak (true);
        if (off < 0.0f || on < 0.0f) { std::cerr << "limiter test NaN\n"; return 1; }
        if (off <= 0.891f) { std::cerr << "limiter test not hot enough (off peak " << off << ")\n"; return 1; }
        if (on > 1.0f) { std::cerr << "limiter failed to clamp (on peak " << on << ")\n"; return 1; }
        if (on >= off) { std::cerr << "limiter did not reduce the peak\n"; return 1; }
    }

    // Fractal FX must be a tail, not a booster: with the wet path wide open it
    // stays finite, audible and no louder than the dry signal it sits on. DEPTH
    // decides how far the comb may wander and RATE how often it re-tunes, so both
    // have to move the waveform, not just the loudness.
    {
        struct FractalRender { double peak = 0.0, level = 0.0; std::vector<float> data; };

        auto fractalRender = [] (int fxType, float depth, float rateNorm = 0.5f)
        {
            FractalRender r;
            PluginProcessor proc;
            proc.prepareToPlay (48000.0, 512);
            proc.apvts.getParameter ("samplesPerTimestep")->setValueNotifyingHost (0.2f);
            proc.apvts.getParameter ("limiter")->setValueNotifyingHost (0.0f);
            proc.apvts.getParameter ("fxType")->setValueNotifyingHost ((float) fxType / 4.0f);
            proc.apvts.getParameter ("fxMix")->setValueNotifyingHost (1.0f);
            proc.apvts.getParameter ("fxFeedback")->setValueNotifyingHost (1.0f);
            proc.apvts.getParameter ("fxEntropy")->setValueNotifyingHost (1.0f);
            proc.apvts.getParameter ("fxDepth")->setValueNotifyingHost (depth);
            proc.apvts.getParameter ("fxRate")->setValueNotifyingHost (rateNorm);

            juce::MidiBuffer midi, empty;
            midi.addEvent (juce::MidiMessage::noteOn (1, 57, (juce::uint8) 110), 0);
            juce::AudioBuffer<float> buffer (2, 512);
            for (int b = 0; b < 64; ++b)
            {
                proc.processBlock (buffer, b == 0 ? midi : empty);
                const float* d = buffer.getReadPointer (0);
                for (int i = 0; i < 512; ++i)
                {
                    if (! std::isfinite (d[i])) { r.peak = -1.0; return r; }
                    r.peak = std::max (r.peak, (double) std::abs (d[i]));
                    r.level += std::abs (d[i]);
                    r.data.push_back (d[i]);
                }
            }
            if (r.level <= 0.0) r.peak = 0.0;
            return r;
        };

        auto relativeDiff = [] (const FractalRender& a, const FractalRender& b)
        {
            double diff = 0.0;
            for (size_t i = 0; i < a.data.size(); ++i)
                diff += std::abs (a.data[i] - b.data[i]);
            return diff / a.level;
        };

        const auto dry = fractalRender (0, 1.0f);
        const auto wet = fractalRender (4, 1.0f);
        const auto shallow = fractalRender (4, 0.0f);
        const auto still = fractalRender (4, 1.0f, 0.0f);
        const auto hopping = fractalRender (4, 1.0f, 1.0f);
        if (dry.peak < 0.0 || wet.peak < 0.0 || shallow.peak < 0.0
            || still.peak < 0.0 || hopping.peak < 0.0) { std::cerr << "fractal FX NaN\n"; return 1; }
        if (wet.peak <= 0.0) { std::cerr << "fractal FX silent\n"; return 1; }
        if (wet.peak > dry.peak * 1.05)
        { std::cerr << "fractal FX louder than dry: " << wet.peak << " vs " << dry.peak << "\n"; return 1; }
        if (relativeDiff (wet, shallow) < 0.02)
        { std::cerr << "fractal DEPTH does nothing\n"; return 1; }
        if (relativeDiff (still, hopping) < 0.02)
        { std::cerr << "fractal RATE does nothing\n"; return 1; }
    }

    // Quantum Lab (wave 4): evolution-level FX, exchange symmetry and the
    // master lab matrix — finite, audible, state-changing, reproducible, and
    // bit-exact when the lab is switched off.
    {
        auto renderLab = [] (const std::vector<std::pair<const char*, float>>& settings, int blocks)
        {
            PluginProcessor proc;
            proc.prepareToPlay (48000.0, 512);
            proc.apvts.getParameter ("samplesPerTimestep")->setValueNotifyingHost (0.2f);
            proc.apvts.getParameter ("limiter")->setValueNotifyingHost (0.0f);
            for (const auto& s : settings)
                if (auto* p = proc.apvts.getParameter (s.first))
                    p->setValueNotifyingHost (s.second);

            juce::MidiBuffer midi, empty;
            midi.addEvent (juce::MidiMessage::noteOn (1, 60, (juce::uint8) 100), 0);
            midi.addEvent (juce::MidiMessage::noteOn (1, 64, (juce::uint8) 90), 0);
            juce::AudioBuffer<float> buffer (2, 512);
            std::vector<float> out;
            for (int b = 0; b < blocks; ++b)
            {
                proc.processBlock (buffer, b == 0 ? midi : empty);
                out.insert (out.end(), buffer.getReadPointer (0), buffer.getReadPointer (0) + 512);
            }
            return out;
        };
        auto labStats = [] (const std::vector<float>& v) -> std::pair<double, bool>
        {
            double rms = 0.0;
            bool ok = true;
            for (float x : v)
            {
                if (! std::isfinite (x)) { ok = false; break; }
                rms += (double) x * x;
            }
            return { std::sqrt (rms / (double) std::max<size_t> (1, v.size())), ok };
        };

        const auto labBase = renderLab ({}, 24);
        const auto baseStats = labStats (labBase);
        if (! baseStats.second || baseStats.first <= 0.0) { std::cerr << "lab baseline broken\n"; return 1; }

        // (a) evolution effects each change the signal and stay clean.
        for (const char* id : { "gpSat", "wkbFilter", "qPhaser", "phaseAccum" })
        {
            const auto v = renderLab ({ { id, 0.8f } }, 24);
            const auto st = labStats (v);
            if (! st.second || st.first <= 0.0) { std::cerr << "lab evolution NaN/silent: " << id << "\n"; return 1; }
            if (v == labBase) { std::cerr << "lab evolution had no effect: " << id << "\n"; return 1; }
        }

        // (b) exchange symmetry: Bose and Fermi differ from off and each other.
        const auto bose  = renderLab ({ { "interfere", 0.5f } }, 24);
        const auto fermi = renderLab ({ { "interfere", 1.0f } }, 24);
        for (const auto* v : { &bose, &fermi })
        {
            const auto st = labStats (*v);
            if (! st.second || st.first <= 0.0) { std::cerr << "symmetry NaN/silent\n"; return 1; }
            if (*v == labBase) { std::cerr << "symmetry had no effect\n"; return 1; }
        }
        if (bose == fermi) { std::cerr << "Bose and Fermi render identically\n"; return 1; }

        // (c) master lab matrix: every type renders 4.8 s of clean audio.
        for (int lt = 1; lt <= 5; ++lt)
        {
            const auto v = renderLab ({ { "labType", (float) lt / 5.0f },
                                        { "labAmt", 0.7f }, { "labChar", 0.85f },
                                        { "labSize", 0.6f }, { "labMix", 0.6f } }, 96);
            const auto st = labStats (v);
            if (! st.second || st.first <= 0.0) { std::cerr << "lab FX NaN/silent: type " << lt << "\n"; return 1; }
        }

        // (d) lab off is a bit-exact bypass: wet controls stay irrelevant.
        const auto bypass = renderLab ({ { "labAmt", 0.9f }, { "labChar", 1.0f },
                                         { "labSize", 1.0f }, { "labMix", 1.0f } }, 24);
        if (bypass != labBase) { std::cerr << "lab off is not a bit-exact bypass\n"; return 1; }

        // (e) reproducibility: the FDN and the event RNG are deterministic.
        const std::vector<std::pair<const char*, float>> anders =
            { { "labType", 0.2f }, { "labAmt", 0.7f }, { "labChar", 0.85f },
              { "labSize", 0.6f }, { "labMix", 0.6f } };
        if (renderLab (anders, 96) != renderLab (anders, 96))
        { std::cerr << "lab reverb not reproducible\n"; return 1; }

        // (f) Born events really duck: over 3 s at MIX=1 the gain envelope
        //     must swing between near-full and near-zero.
        const auto ducked = renderLab ({ { "labType", 0.6f }, { "labAmt", 1.0f },
                                         { "labChar", 1.0f }, { "labSize", 0.3f },
                                         { "labMix", 1.0f } }, 192);
        float duckMax = 0.0f, duckMin = 1.0e9f;
        for (float x : ducked) { duckMax = std::max (duckMax, std::abs (x)); duckMin = std::min (duckMin, std::abs (x)); }
        if (! std::isfinite (duckMin) || duckMax <= 0.0f || duckMin > duckMax * 0.25f)
        { std::cerr << "born events never duck: min " << duckMin << " max " << duckMax << "\n"; return 1; }
    }

    // Factory presets: every file must carry a params object whose keys are real
    // parameter IDs inside their ranges, and applying it must render cleanly.
    {
        int checked = 0;
        for (int i = 0; i < BinaryData::namedResourceListSize; ++i)
        {
            const juce::String resName { juce::CharPointer_UTF8 { BinaryData::namedResourceList[i] } };
            const juce::String orig { juce::CharPointer_UTF8 { BinaryData::getNamedResourceOriginalFilename (resName.toRawUTF8()) } };
            if (! orig.endsWith (".sp303preset"))
                continue;

            int size = 0;
            const char* data = BinaryData::getNamedResource (resName.toRawUTF8(), size);
            if (data == nullptr) { std::cerr << "preset resource unreadable: " << orig << "\n"; return 1; }
            const auto parsed = juce::JSON::parse (juce::String::fromUTF8 (data, size));
            if (! parsed.isObject()) { std::cerr << "preset is not a JSON object: " << orig << "\n"; return 1; }
            auto* root = parsed.getDynamicObject();
            auto* params = root->getProperty ("params").getDynamicObject();
            if (params == nullptr) { std::cerr << "preset has no params object: " << orig << "\n"; return 1; }

            PluginProcessor proc;
            proc.prepareToPlay (48000.0, 512);
            for (const auto& property : params->getProperties())
            {
                const juce::String key = property.name.toString();
                if (! property.value.isDouble() && ! property.value.isInt())
                { std::cerr << "preset value not numeric: " << orig.toRawUTF8() << " / " << key.toRawUTF8() << "\n"; return 1; }
                auto* p = proc.apvts.getParameter (key);
                if (p == nullptr)
                { std::cerr << "unknown parameter " << key.toRawUTF8() << " in " << orig.toRawUTF8() << "\n"; return 1; }
                const float raw = static_cast<float> (static_cast<double> (property.value));
                const auto& nr = p->getNormalisableRange();
                const float norm = nr.convertTo0to1 (raw);
                if (norm < -0.001f || norm > 1.001f)
                { std::cerr << "value " << raw << " out of range for " << key.toRawUTF8() << " in " << orig.toRawUTF8() << "\n"; return 1; }
                p->setValueNotifyingHost (norm);
            }

            juce::MidiBuffer midi, empty;
            midi.addEvent (juce::MidiMessage::noteOn (1, 60, (juce::uint8) 100), 0);
            juce::AudioBuffer<float> buffer (2, 512);
            double energy = 0.0;
            for (int b = 0; b < 16; ++b)
            {
                proc.processBlock (buffer, b == 0 ? midi : empty);
                const float* d = buffer.getReadPointer (0);
                for (int i = 0; i < 512; ++i)
                {
                    if (! std::isfinite (d[i])) { std::cerr << "preset NaN: " << orig << "\n"; return 1; }
                    energy += (double) d[i] * d[i];
                }
            }
            if (energy <= 0.0) { std::cerr << "preset silent: " << orig << "\n"; return 1; }
            ++checked;
        }
        if (checked < 18)
        { std::cerr << "expected at least 18 factory presets, found " << checked << "\n"; return 1; }
    }

    if (argc > 1 && juce::String (argv[1]) == "--bench")
    {
        // Deterministic CPU benchmark: 8 voices, evolution on, no FX.
        PluginProcessor proc;
        proc.prepareToPlay (48000.0, 512);
        proc.apvts.getParameter ("samplesPerTimestep")->setValueNotifyingHost (0.2f);
        proc.apvts.getParameter ("applyWavefunction")->setValueNotifyingHost (1.0f);

        {
            juce::MidiBuffer midi;
            for (int n : { 48, 55, 60, 62, 64, 67, 69, 72 })
                midi.addEvent (juce::MidiMessage::noteOn (1, (juce::uint8) n, (juce::uint8) 100), 0);
            juce::AudioBuffer<float> buffer (2, 512);
            proc.processBlock (buffer, midi);
        }

        const int blocks = 20000;
        juce::AudioBuffer<float> buffer (2, 512);
        juce::MidiBuffer empty;
        const double t0 = juce::Time::getMillisecondCounterHiRes();
        for (int i = 0; i < blocks; ++i)
            proc.processBlock (buffer, empty);
        const double us = (juce::Time::getMillisecondCounterHiRes() - t0) * 1000.0 / (double) blocks;
        const float rt = (float) (us * 48000.0 / 512.0 / 1.0e6);
        std::cout << "bench: " << juce::String (us, 1) << " us/block  ("
                  << juce::String (rt, 3) << " x realtime, 8 voices)\n";

        float rms = 0.0f;
        const float* d = buffer.getReadPointer (0);
        for (int i = 0; i < 512; ++i) { if (! std::isfinite (d[i])) { std::cerr << "bench NaN\n"; return 1; } rms += d[i] * d[i]; }
        if (rms <= 0.0f) { std::cerr << "bench silent\n"; return 1; }
    }

    if (argc > 1 && juce::String (argv[1]) == "--bench-extreme")
    {
        // Worst-case load probe: samplesPerTimestep = 0.1 demands ten evolution
        // steps per sample per voice. The per-block step budget caps this, so
        // the block time must stay inside the realtime envelope instead of
        // exploding by an order of magnitude.
        PluginProcessor proc;
        proc.prepareToPlay (48000.0, 512);
        proc.apvts.getParameter ("samplesPerTimestep")->setValueNotifyingHost (0.0f); // min = 0.1
        proc.apvts.getParameter ("applyWavefunction")->setValueNotifyingHost (1.0f);

        {
            juce::MidiBuffer midi;
            for (int n : { 48, 55, 60, 62, 64, 67, 69, 72 })
                midi.addEvent (juce::MidiMessage::noteOn (1, (juce::uint8) n, (juce::uint8) 100), 0);
            juce::AudioBuffer<float> buffer (2, 512);
            proc.processBlock (buffer, midi);
        }

        const int blocks = 400;
        juce::AudioBuffer<float> buffer (2, 512);
        juce::MidiBuffer empty;
        const double t0 = juce::Time::getMillisecondCounterHiRes();
        for (int i = 0; i < blocks; ++i)
            proc.processBlock (buffer, empty);
        const double us = (juce::Time::getMillisecondCounterHiRes() - t0) * 1000.0 / (double) blocks;
        const float rt = (float) (us * 48000.0 / 512.0 / 1.0e6);
        std::cout << "bench-extreme: " << juce::String (us, 1) << " us/block  ("
                  << juce::String (rt, 3) << " x realtime, 8 voices, STEP=0.1)\n";

        float rms = 0.0f;
        const float* d = buffer.getReadPointer (0);
        for (int i = 0; i < 512; ++i) { if (! std::isfinite (d[i])) { std::cerr << "bench-extreme NaN\n"; return 1; } rms += d[i] * d[i]; }
        if (rms <= 0.0f) { std::cerr << "bench-extreme silent\n"; return 1; }
    }

    if (argc > 1 && juce::String (argv[1]) == "--bench-lab")
    {
        // Quantum Lab cost probe: 8 voices with every evolution effect on,
        // benchmarked once per master lab effect at full mix.
        const char* names[] = { "Off", "Anderson Reverb", "Adiabatic Pitch",
                                "Born Events", "Mode Clip", "Loschmidt Delay" };
        for (int lt = 0; lt <= 5; ++lt)
        {
            PluginProcessor proc;
            proc.prepareToPlay (48000.0, 512);
            proc.apvts.getParameter ("samplesPerTimestep")->setValueNotifyingHost (0.2f);
            proc.apvts.getParameter ("applyWavefunction")->setValueNotifyingHost (1.0f);
            proc.apvts.getParameter ("gpSat")->setValueNotifyingHost (0.6f);
            proc.apvts.getParameter ("wkbFilter")->setValueNotifyingHost (0.6f);
            proc.apvts.getParameter ("qPhaser")->setValueNotifyingHost (0.6f);
            proc.apvts.getParameter ("phaseAccum")->setValueNotifyingHost (0.6f);
            proc.apvts.getParameter ("labType")->setValueNotifyingHost ((float) lt / 5.0f);
            proc.apvts.getParameter ("labAmt")->setValueNotifyingHost (0.7f);
            proc.apvts.getParameter ("labChar")->setValueNotifyingHost (0.5f);
            proc.apvts.getParameter ("labSize")->setValueNotifyingHost (0.5f);
            proc.apvts.getParameter ("labMix")->setValueNotifyingHost (0.5f);

            {
                juce::MidiBuffer midi;
                for (int n : { 48, 55, 60, 62, 64, 67, 69, 72 })
                    midi.addEvent (juce::MidiMessage::noteOn (1, (juce::uint8) n, (juce::uint8) 100), 0);
                juce::AudioBuffer<float> buffer (2, 512);
                proc.processBlock (buffer, midi);
            }

            const int blocks = lt == 1 ? 4000 : 20000;
            juce::AudioBuffer<float> buffer (2, 512);
            juce::MidiBuffer empty;
            const double t0 = juce::Time::getMillisecondCounterHiRes();
            for (int i = 0; i < blocks; ++i)
                proc.processBlock (buffer, empty);
            const double us = (juce::Time::getMillisecondCounterHiRes() - t0) * 1000.0 / (double) blocks;
            const float rt = (float) (us * 48000.0 / 512.0 / 1.0e6);

            float rms = 0.0f;
            const float* d = buffer.getReadPointer (0);
            for (int i = 0; i < 512; ++i)
            {
                if (! std::isfinite (d[i])) { std::cerr << "bench-lab NaN at " << lt << "\n"; return 1; }
                rms += d[i] * d[i];
            }
            if (rms <= 0.0f) { std::cerr << "bench-lab silent at " << lt << "\n"; return 1; }
            std::cout << "bench-lab [" << names[lt] << "]: " << juce::String (us, 1)
                      << " us/block  (" << juce::String (rt, 3) << " x realtime)\n";
        }
    }

    std::cout << "ALL TESTS PASSED" << std::endl;
    return 0;
}