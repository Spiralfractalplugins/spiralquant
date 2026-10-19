#include "PluginProcessor.h"

juce::AudioProcessorValueTreeState::ParameterLayout createLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    auto choice = [] (const char* id, const char* name, juce::StringArray opts, int def)
    {
        return std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { id, 1 }, name, opts, def);
    };
    auto flt = [] (const char* id, const char* name, float min, float max, float def)
    {
        return std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { id, 1 }, name,
            juce::NormalisableRange<float> (min, max), def);
    };
    auto bl = [] (const char* id, const char* name, bool def)
    {
        return std::make_unique<juce::AudioParameterBool> (juce::ParameterID { id, 1 }, name, def);
    };

    params.push_back (choice ("waveType", "Wave Type", { "Gaussian","Sine","Cos","Parabola","Saw","Square" }, 0));
    params.push_back (flt ("waveShift", "Wave Shift", 0.0f, 1.0f, 0.5f));
    params.push_back (flt ("waveScale", "Wave Scale", 0.0f, 1.0f, 0.5f));
    params.push_back (flt ("waveImpulse", "Wave Impulse", 0.0f, 1.0f, 0.2f));

    params.push_back (choice ("potentialType", "Potential", { "Flat","Harmonic","InfiniteWell","Barrier","DoubleWell","Linear" }, 1));
    params.push_back (flt ("potentialShift", "Pot Shift", 0.0f, 1.0f, 0.5f));
    params.push_back (flt ("potentialScale", "Pot Scale", 0.0f, 1.0f, 0.5f));
    params.push_back (flt ("potentialHeight", "Pot Height", 0.0f, 1.0f, 0.5f));

    params.push_back (bl ("showFFT", "Show FFT", false));
    params.push_back (choice ("sampleMode", "Sample Mode", { "Real","Imag","Magnitude","Phase","PhononQND" }, 0));
    params.push_back (flt ("timestepDelta", "Timestep", 0.001f, 0.2f, 0.01f));
    params.push_back (flt ("samplesPerTimestep", "Evol Rate", 0.1f, 64.0f, 8.0f));
    params.push_back (bl ("applyWavefunction", "Apply QM", true));

    params.push_back (flt ("attack", "Attack", 1.0f, 2000.0f, 10.0f));
    params.push_back (flt ("decay", "Decay", 1.0f, 2000.0f, 100.0f));
    params.push_back (flt ("sustain", "Sustain", 0.0f, 1.0f, 0.8f));
    params.push_back (flt ("release", "Release", 1.0f, 5000.0f, 200.0f));

    params.push_back (flt ("filterFreq", "Filter Freq", 20.0f, 20000.0f, 4000.0f));
    params.push_back (flt ("filterRes", "Filter Res", 0.0f, 1.0f, 0.3f));
    params.push_back (flt ("drive", "Drive", 0.0f, 1.0f, 0.0f));
    params.push_back (flt ("satMix", "Sat Mix", 0.0f, 1.0f, 0.0f));
    params.push_back (flt ("gain", "Gain", 0.0f, 2.0f, 0.8f));

    // Master FX (appended last: host index stability)
    params.push_back (choice ("fxType", "FX Type", { "Off","Flanger","Phaser","Bitcrush","Fractal" }, 0));
    params.push_back (flt ("fxDepth", "FX Depth", 0.0f, 1.0f, 0.5f));
    params.push_back (flt ("fxRate", "FX Rate", 0.01f, 20.0f, 1.0f));
    params.push_back (flt ("fxFeedback", "FX Feedback", 0.0f, 0.95f, 0.3f));
    params.push_back (flt ("fxMix", "FX Mix", 0.0f, 1.0f, 0.5f));
    params.push_back (flt ("fxBits", "FX Bits", 2.0f, 16.0f, 8.0f));
    params.push_back (flt ("fxEntropy", "FX Entropy", 0.0f, 1.0f, 0.5f));
    params.push_back (choice ("fxSolo", "FX Solo", { "Full","Dry","Wet" }, 0));

    // Harmony / pitch / tunneling (appended last: host index stability)
    params.push_back (bl ("chordMode", "Chord Mode", false));
    params.push_back (choice ("chordType", "Chord Type",
        { "Power","Major","Minor","Sus4","Major7","Tritone","Cluster","Pythagorean" }, 1));
    params.push_back (flt ("pitchShift", "Pitch Shift", -4800.0f, 4800.0f, 0.0f));
    params.push_back (bl ("tunnel", "Tunneling", false));

    // UI-only (appended last: host index stability). Not read on the audio thread.
    params.push_back (choice ("uiSkin", "UI Skin", { "Dark","SciFi Neon","Console" }, 0));

    // Phonon update (appended at the very end: host index stability).
    params.push_back (bl ("chordSuperpose", "Superpose Chords", false));
    params.push_back (flt ("jumpRate", "Phonon Collapse", 0.0f, 1.0f, 0.0f));
    params.push_back (flt ("frftAngle", "Quadrature Rotation", 0.0f, 1.0f, 0.0f));

    // Wave 2: resonator memory, cavity ringdown, quantum walk (appended at the
    // very end: host index stability).
    params.push_back (flt ("memoryAmt", "Resonator Memory", 0.0f, 1.0f, 0.0f));
    params.push_back (flt ("memoryTime", "Memory Delay", 5.0f, 2000.0f, 80.0f));
    params.push_back (flt ("ringdown", "Cavity Ringdown", 0.0f, 1.0f, 0.0f));
    params.push_back (bl ("walkOn", "Quantum Walk", false));
    params.push_back (flt ("walkRate", "Walk Rate", 0.25f, 8.0f, 2.0f));

    // Output limiter (appended at the very end: host index stability).
    params.push_back (bl ("limiter", "Output Limiter", true));

    // Wave 3: Zeno measurement, EPR entanglement, squeezed states (appended at
    // the very end: host index stability).
    params.push_back (flt ("zeno", "Zeno Measurement", 0.0f, 1.0f, 0.0f));
    params.push_back (bl ("entangle", "Entangle Voices", false));
    params.push_back (flt ("squeeze", "Squeeze", -1.0f, 1.0f, 0.0f));

    return { params.begin(), params.end() };
}

PluginProcessor::PluginProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "Parameters", createLayout()),
      params (makeParamCache())
{
}

PluginProcessor::ParamCache PluginProcessor::makeParamCache() const
{
    auto r = [this] (const char* id) { return apvts.getRawParameterValue (id); };
    return { r ("waveType"), r ("waveShift"), r ("waveScale"), r ("waveImpulse"),
             r ("potentialType"), r ("potentialShift"), r ("potentialScale"), r ("potentialHeight"),
             r ("showFFT"), r ("sampleMode"), r ("timestepDelta"), r ("samplesPerTimestep"),
             r ("applyWavefunction"),
             r ("attack"), r ("decay"), r ("sustain"), r ("release"),
             r ("filterFreq"), r ("filterRes"), r ("drive"), r ("satMix"), r ("gain"),
             r ("fxType"), r ("fxDepth"), r ("fxRate"), r ("fxFeedback"), r ("fxMix"),
             r ("fxBits"), r ("fxEntropy"), r ("fxSolo"),
             r ("chordMode"), r ("chordType"), r ("pitchShift"), r ("tunnel"),
             r ("chordSuperpose"), r ("jumpRate"), r ("frftAngle"),
             r ("memoryAmt"), r ("memoryTime"), r ("ringdown"), r ("walkOn"), r ("walkRate"),
             r ("limiter"), r ("zeno"), r ("entangle"), r ("squeeze") };
}

void PluginProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    (void) samplesPerBlock;
    currentSampleRate = sampleRate;
    engine.prepare (sampleRate);
}

void PluginProcessor::releaseResources() {}

void PluginProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();

    // Raw pointers resolved once at construction: string lookups here would cost
    // a linear scan with String comparisons per parameter, per audio block.
    engine.setParams (
        params.attack->load(), params.decay->load(), params.sustain->load(), params.release->load(),
        params.filterFreq->load(), params.filterRes->load(), params.drive->load(),
        params.satMix->load(), params.gain->load(),
        static_cast<int> (params.waveType->load()),
        params.waveShift->load(), params.waveScale->load(), params.waveImpulse->load(),
        static_cast<int> (params.potentialType->load()),
        params.potentialShift->load(), params.potentialScale->load(), params.potentialHeight->load(),
        params.showFFT->load() >= 0.5f ? 1 : 0,
        static_cast<int> (params.sampleMode->load()),
        params.timestepDelta->load(), params.samplesPerTimestep->load(),
        params.applyWavefunction->load() >= 0.5f,
        params.chordMode->load() >= 0.5f ? (params.chordSuperpose->load() >= 0.5f ? 2 : 1) : 0,
        static_cast<int> (params.chordType->load()),
        params.pitchShift->load(),
        params.tunnel->load() >= 0.5f,
        params.jumpRate->load(), params.frftAngle->load(),
        params.memoryAmt->load(), params.memoryTime->load(), params.ringdown->load(),
        params.walkOn->load() >= 0.5f, params.walkRate->load(),
        params.limiter->load() >= 0.5f,
        params.zeno->load(), params.entangle->load() >= 0.5f, params.squeeze->load());

    engine.setFxParams (
        static_cast<int> (params.fxType->load()),
        params.fxDepth->load(), params.fxRate->load(), params.fxFeedback->load(),
        params.fxMix->load(), params.fxBits->load(), params.fxEntropy->load(),
        static_cast<int> (params.fxSolo->load()));

    bool running = true;
    engine.process (buffer, midi, running);
}

bool PluginProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
        && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;
    return true;
}

void PluginProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void PluginProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}