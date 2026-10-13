#include "PluginProcessor.h"
#include "PluginEditor.h"

juce::AudioProcessorEditor* PluginProcessor::createEditor()
{
    return new PluginEditor (*this);
}

juce::AudioProcessor* createPluginFilter()
{
    return new PluginProcessor();
}
