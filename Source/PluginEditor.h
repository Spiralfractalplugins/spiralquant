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
    enum class Group { Wave, Potential, Env, Filter, Fx, Harmony };
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
    void layoutKnob (const Knob& k, juce::Rectangle<int> cell);
    juce::Rectangle<int> scaled (juce::Rectangle<int> r) const;
    void applySkin();
    void setGroupAlpha();
    void timerCallback() override;

    PluginProcessor& processor;
    DarkLookAndFeel lnf;
    float uiScale = 1.0f;
    int originX = 0, originY = 0;

    static constexpr int baseW = 1280, baseH = 920;

    juce::Label title, versionLabel, status;
    LogoMark logo;
    QuantumCanvas canvas;
    AdsrCanvas adsrCanvas;
    PresetBrowser presetBrowser;
    juce::ComboBox skinCombo;

    juce::TextButton waveTypeChip, potentialTypeChip;
    juce::ToggleButton showFFT, applyQm, chordChip, tunnelChip;
    juce::ComboBox sampleMode, fxType, fxSolo, chordType;

    std::vector<Knob> knobs;
    std::vector<ValueSlider*> fxKnobs;
    juce::Slider* pitchKnob = nullptr;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> sampleModeAtt, fxTypeAtt,
        fxSoloAtt, chordTypeAtt, uiSkinAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> showFFTAtt, applyQmAtt,
        chordAtt, tunnelAtt;

    // Base-grid rectangles (design units; every bound goes through scaled()).
    static const juce::Rectangle<int> headerArea, canvasArea, adsrArea, harmonyArea,
        waveArea, potArea, envArea, filterArea, fxArea;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginEditor)
};
