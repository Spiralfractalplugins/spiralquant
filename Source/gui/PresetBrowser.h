#pragma once
#include <JuceHeader.h>

/** Factory + user preset browser wired to an APVTS.
    Factory presets are the .sp303preset JSON files embedded at build time;
    user presets live in ~/Library/Application Support/SpiralQuant/presets. */
class PresetBrowser : public juce::Component
{
public:
    explicit PresetBrowser (juce::AudioProcessorValueTreeState& s) : state (s)
    {
        addAndMakeVisible (combo);
        addAndMakeVisible (loadBtn);
        addAndMakeVisible (saveBtn);
        addAndMakeVisible (deleteBtn);
        loadBtn.setButtonText ("LOAD");
        saveBtn.setButtonText ("SAVE");
        deleteBtn.setButtonText ("DEL");
        combo.setTooltip ("Preset list: F = factory, U = user.");
        loadBtn.setTooltip ("Recall the selected preset.");
        saveBtn.setTooltip ("Save all current parameter values as a user preset.");
        deleteBtn.setTooltip ("Delete the selected user preset from disk.");
        loadBtn.onClick = [this] { applySelected(); };
        saveBtn.onClick = [this] { saveCurrent(); };
        deleteBtn.onClick = [this] { deleteSelected(); };
        refresh();
    }

    void resized() override
    {
        auto r = getLocalBounds().reduced (1, 0);
        deleteBtn.setBounds (r.removeFromRight (48).reduced (1));
        saveBtn.setBounds   (r.removeFromRight (52).reduced (1));
        loadBtn.setBounds   (r.removeFromRight (52).reduced (1));
        combo.setBounds (r);
    }

    void refresh()
    {
        entries.clear();
        for (int i = 0; i < BinaryData::namedResourceListSize; ++i)
        {
            const juce::String resName { juce::CharPointer_UTF8 { BinaryData::namedResourceList[i] } };
            const juce::String orig { juce::CharPointer_UTF8 { BinaryData::getNamedResourceOriginalFilename (resName.toRawUTF8()) } };
            if (! orig.endsWith (".sp303preset"))
                continue;
            int size = 0;
            const char* data = BinaryData::getNamedResource (resName.toRawUTF8(), size);
            if (data == nullptr)
                continue;
            const juce::String json = juce::String::fromUTF8 (data, size);
            PresetEntry e;
            e.isFactory = true;
            e.displayName = titleOf (json, orig.upToFirstOccurrenceOf (".sp303preset", false, false));
            e.json = json;
            entries.add (e);
        }
        std::sort (entries.begin(), entries.end(), [] (const PresetEntry& a, const PresetEntry& b)
                   { return a.displayName < b.displayName; });

        const int firstUser = entries.size();
        for (const auto& f : userDir().findChildFiles (juce::File::findFiles, false, "*.sp303preset"))
        {
            PresetEntry e;
            e.displayName = titleOf (f.loadFileAsString(), f.getFileNameWithoutExtension());
            e.json = f.loadFileAsString();
            e.file = f;
            entries.add (e);
        }
        std::sort (entries.begin() + firstUser, entries.end(), [] (const PresetEntry& a, const PresetEntry& b)
                   { return a.displayName < b.displayName; });

        combo.clear (juce::dontSendNotification);
        for (int i = 0; i < entries.size(); ++i)
            combo.addItem ((entries[i].isFactory ? "F  " : "U  ") + entries[i].displayName, i + 1);
        if (! entries.isEmpty())
            combo.setSelectedId (1, juce::dontSendNotification);
    }

private:
    struct PresetEntry
    {
        juce::String displayName;
        juce::String json;
        juce::File file;
        bool isFactory = false;
    };

    juce::AudioProcessorValueTreeState& state;
    juce::ComboBox combo;
    juce::TextButton loadBtn, saveBtn, deleteBtn;
    juce::Array<PresetEntry> entries;
    std::unique_ptr<juce::FileChooser> chooser;

    static juce::File userDir()
    {
        return juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
            .getChildFile ("SpiralQuant").getChildFile ("presets");
    }

    static juce::String titleOf (const juce::String& json, const juce::String& fallback)
    {
        if (auto parsed = juce::JSON::parse (json); parsed.isObject())
            if (auto name = parsed.getProperty ("name", {}).toString(); name.isNotEmpty())
                return name;
        return fallback;
    }

    void applySelected()
    {
        const int idx = combo.getSelectedId() - 1;
        if (idx < 0 || idx >= entries.size())
            return;
        applyJson (entries[idx].json);
    }

    void applyJson (const juce::String& json)
    {
        const auto parsed = juce::JSON::parse (json);
        if (! parsed.isObject())
            return;
        const auto params = parsed.getDynamicObject ();
        if (params == nullptr)
            return;
        for (const auto& key : params->getProperties())
        {
            if (auto* p = state.getParameter (key.name.toString()))
                p->setValueNotifyingHost (p->getNormalisableRange().convertTo0to1 ((float) (double) key.value));
        }
    }

    void saveCurrent()
    {
        chooser = std::make_unique<juce::FileChooser> ("Save Preset",
                    userDir().getChildFile ("New Preset.sp303preset"), "*.sp303preset");
        const int flags = juce::FileBrowserComponent::saveMode
                        | juce::FileBrowserComponent::canSelectFiles;
        chooser->launchAsync (flags, [this] (const juce::FileChooser& fc)
        {
            auto file = fc.getResult();
            if (file != juce::File())
                writePresetTo (file);
            chooser.reset();
        });
    }

    void writePresetTo (const juce::File& file)
    {
        if (! file.getParentDirectory().exists())
            file.getParentDirectory().createDirectory();

        auto* obj = new juce::DynamicObject();
        juce::var root (obj);
        obj->setProperty ("format", 1);
        auto name = file.getFileNameWithoutExtension();
        obj->setProperty ("name", name);
        obj->setProperty ("credit", "User");
        auto* params = new juce::DynamicObject();
        obj->setProperty ("params", juce::var (params));
        for (int i = 0; i < state.state.getNumProperties(); ++i)
            if (auto* p = state.getParameter (state.state.getPropertyName (i).toString()))
                params->setProperty (p->paramID, (double) p->getNormalisableRange().convertFrom0to1 (p->getValue()));
        file.replaceWithText (juce::JSON::toString (root));
        refresh();
        for (int i = 0; i < entries.size(); ++i)
            if (! entries[i].isFactory && entries[i].displayName == name)
                combo.setSelectedId (i + 1, juce::dontSendNotification);
    }

    void deleteSelected()
    {
        const int idx = combo.getSelectedId() - 1;
        if (idx < 0 || idx >= entries.size() || entries[idx].isFactory)
            return;
        entries[idx].file.deleteFile();
        refresh();
    }
};
