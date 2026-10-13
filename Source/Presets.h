#pragma once

#include <vector>

#include "PluginParameters.h"
#include "dsp/Sequence.h"

struct PresetValue
{
    juce::String paramId;
    float rawValue; // in the parameter's own units
};

struct PresetDef
{
    juce::String name;
    std::vector<PresetValue> values;
    juce::String sequence; // Sequence::encode() text; empty keeps the current pattern
    juce::String gate;     // GatePattern::encode() text; empty keeps the current gate
    juce::String credit;   // attribution line shown in the preset menu
};

/** The .sp303preset JSON format - see presets/README.md. Shared by the factory presets
    compiled in from presets/ and the user presets saved next to the settings file. */
namespace PresetFormat
{
    constexpr int version = 1;

    /** Reads one preset object. Returns false and a message naming what is wrong. */
    bool parse (const juce::var& root, PresetDef& out, juce::String& error);

    /** Parses a whole file, as saved on disk or embedded at build time. */
    bool parseText (const juce::String& jsonText, PresetDef& out, juce::String& error);

    /** Serialises a preset. */
    juce::String toText (const PresetDef&);
}

/** The presets compiled in from the presets/ folder, in file-name order. */
const std::vector<PresetDef>& factoryPresets();

/** Files from presets/ that did not load, as "name: reason". Empty in a good build. */
const std::vector<juce::String>& factoryPresetErrors();
