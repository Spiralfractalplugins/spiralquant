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
    params.push_back (choice ("sampleMode", "Sample Mode", { "Real","Imag","Magnitude","Phase" }, 0));
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

    return { params.begin(), params.end() };
}

PluginProcessor::PluginProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "Parameters", createLayout())
{
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

    auto* wt = (juce::AudioParameterChoice*) apvts.getParameter ("waveType");
    auto* pt = (juce::AudioParameterChoice*) apvts.getParameter ("potentialType");
    auto* sm = (juce::AudioParameterChoice*) apvts.getParameter ("sampleMode");

    engine.setParams (
        *apvts.getRawParameterValue ("attack"),
        *apvts.getRawParameterValue ("decay"),
        *apvts.getRawParameterValue ("sustain"),
        *apvts.getRawParameterValue ("release"),
        *apvts.getRawParameterValue ("filterFreq"),
        *apvts.getRawParameterValue ("filterRes"),
        *apvts.getRawParameterValue ("drive"),
        *apvts.getRawParameterValue ("satMix"),
        *apvts.getRawParameterValue ("gain"),
        wt->getIndex(),
        *apvts.getRawParameterValue ("waveShift"),
        *apvts.getRawParameterValue ("waveScale"),
        *apvts.getRawParameterValue ("waveImpulse"),
        pt->getIndex(),
        *apvts.getRawParameterValue ("potentialShift"),
        *apvts.getRawParameterValue ("potentialScale"),
        *apvts.getRawParameterValue ("potentialHeight"),
        apvts.getRawParameterValue ("showFFT")->load() ? 1 : 0,
        sm->getIndex(),
        *apvts.getRawParameterValue ("timestepDelta"),
        *apvts.getRawParameterValue ("samplesPerTimestep"),
        apvts.getRawParameterValue ("applyWavefunction")->load()
    );

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