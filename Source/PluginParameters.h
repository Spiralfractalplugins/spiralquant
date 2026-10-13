#pragma once

#include <JuceHeader.h>

namespace ParamIDs
{
    inline const juce::String cutoff    { "Cutoff" };
    inline const juce::String wave      { "Wave" };
    inline const juce::String res       { "Res" };
    inline const juce::String env       { "EnvAmt" };
    inline const juce::String contour   { "Contour" };
    inline const juce::String accent    { "Accent" };
    inline const juce::String glide     { "Glide" };
    inline const juce::String volume    { "Volume" };
    inline const juce::String drive     { "Drive" };
    inline const juce::String satMix    { "DistMix" };
    inline const juce::String tone      { "Slope" };
    inline const juce::String warmth    { "Warmth" };
    inline const juce::String clockDiv  { "ClockDiv" };
    inline const juce::String swing     { "Swing" };
    inline const juce::String bpm       { "BPM" };
    inline const juce::String syncMode  { "Sync" };
    inline const juce::String manualPlay{ "Play" };
    inline const juce::String gateOn    { "GateOn" };
    inline const juce::String gateDiv   { "GateDiv" };
    inline const juce::String gateAtk   { "GateAtk" };
    inline const juce::String gateRel   { "GateRel" };
    inline const juce::String masterGain{ "Gain" };
    // Appended last on purpose: AUv2 hands the host index-based addresses, so inserting
    // new parameters in the middle would renumber everything after them.
    inline const juce::String gateLevel   { "GateLvl" };
    inline const juce::String satType     { "SatType" };
    inline const juce::String analogOn    { "AnalogOn" };
    inline const juce::String analogMode  { "AnalogMode" };
}

namespace CustomKeys
{
    /** ValueTree property names for non-automatable custom state. */
    inline const juce::Identifier sequence  { "sequence" };
    inline const juce::Identifier gate      { "gate" };
}
