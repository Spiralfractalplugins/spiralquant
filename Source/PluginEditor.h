#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "gui/DarkLookAndFeel.h"
#include "gui/ValueSlider.h"
#include "gui/QuantumCanvas.h"
#include "gui/AdsrCanvas.h"
#include "gui/PresetBrowser.h"
#include "gui/LogoMark.h"

class PluginEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit PluginEditor (PluginProcessor&);
    ~PluginEditor() override;
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    enum class Group { Wave, Potential, Env, Filter, Fx, Harmony, Lab };
    static juce::Colour accentFor (Group);

    struct Knob
    {
        juce::String id;
        std::unique_ptr<ValueSlider> slider;
        std::unique_ptr<juce::Label> nameLabel, valueLabel;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
        juce::Rectangle<int> baseCell;
        Group group;
        juce::String lastText;
    };

    void addKnob (const char* id, const juce::String& title, const juce::String& tooltip,
                  Group group, ValueSlider::FormatFn format);
    void layoutKnob (const Knob& k, juce::Rectangle<int> cell, int maxKnobD = 0);
    juce::Rectangle<int> scaled (juce::Rectangle<int> r) const;
    void applySkin();
    void setGroupAlpha();
    void timerCallback() override;

    PluginProcessor& processor;
    DarkLookAndFeel lnf;
    float uiScale = 1.0f;
    int originX = 0, originY = 0;

    static constexpr int baseW = 1280, baseH = 1142;

    juce::Label title, versionLabel, status;
    LogoMark logo;
    QuantumCanvas canvas;
    AdsrCanvas adsrCanvas;
    PresetBrowser presetBrowser;
    juce::ComboBox skinCombo;

    juce::TextButton waveTypeChip, potentialTypeChip;
    juce::ToggleButton showFFT, applyQm, chordChip, tunnelChip, superposeChip, walkChip, limiterChip, eprChip;
    juce::ComboBox sampleMode, fxType, fxSolo, chordType, labType, labSym;

    std::vector<Knob> knobs;
    std::vector<ValueSlider*> fxKnobs;
    std::vector<ValueSlider*> labKnobs;   // master lab row, dimmed when the lab is off
    juce::Slider* pitchKnob = nullptr;

    // Cached message-thread accessors: the 20 Hz timer no longer walks the
    // APVTS parameter list by name on every tick.
    std::atomic<float>* skinParam = nullptr;
    std::atomic<float>* jumpParam = nullptr;
    juce::RangedAudioParameter* waveTypeParam = nullptr;
    juce::RangedAudioParameter* potTypeParam = nullptr;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> sampleModeAtt, fxTypeAtt,
        fxSoloAtt, chordTypeAtt, uiSkinAtt, labTypeAtt, labSymAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> showFFTAtt, applyQmAtt,
        chordAtt, tunnelAtt, superposeAtt, walkAtt, limiterAtt, eprAtt;

    // Base-grid rectangles (design units; every bound goes through scaled()).
    // The grid is derived from the golden section: columns split 768:474 (=1.620),
    // the left column splits 520:190:118 with adsr/harmony at 1.610, and the right
    // column's hero panels (208) sit phi times above the utility panels (128).
    // Wave 4 adds the full-width Quantum Lab band (208, matching the hero
    // panel height) below both columns.
    static const juce::Rectangle<int> headerArea, canvasArea, adsrArea, harmonyArea,
        waveArea, potArea, quantArea, filterArea, fxArea, labArea;

    // Hero knobs (Wavefunction panel) cap at phi x the ~75 px utility knob.
    static constexpr int heroKnobD = 122;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginEditor)
};
