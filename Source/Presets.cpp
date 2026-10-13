#include "Presets.h"

#include <algorithm>
#include <cmath>

#include "PresetBankData.h"

using namespace juce;

namespace
{
    /** Preset files carry 6 decimals; keep that so a round-trip does not print noise. */
    double tidy (float v)
    {
        return std::round (static_cast<double> (v) * 1.0e6) / 1.0e6;
    }
}

namespace PresetFormat
{
    bool parse (const var& root, PresetDef& out, String& error)
    {
        auto fail = [&] (const String& why)
        {
            error = why;
            return false;
        };

        auto* obj = root.getDynamicObject();
        if (obj == nullptr)
            return fail ("expected a JSON object");

        const int format = static_cast<int> (static_cast<double> (obj->getProperty ("format")));
        if (format != version)
            return fail ("format " + String (format) + " is not readable by this build, which writes format "
                         + String (version));

        auto name = obj->getProperty ("name");
        if (! name.isString() || name.toString().trim().isEmpty())
            return fail ("missing a name");

        auto* params = obj->getProperty ("params").getDynamicObject();
        if (params == nullptr)
            return fail ("missing a params object");

        PresetDef preset;
        preset.name = name.toString().trim();
        for (const auto& property : params->getProperties())
        {
            // JSON writes 55 as an int and 0.15 as a double - both are numbers here.
            if (! property.value.isInt() && ! property.value.isDouble())
                return fail ("param " + property.name.toString() + " is not a number");
            preset.values.push_back ({ property.name.toString(),
                                       static_cast<float> (static_cast<double> (property.value)) });
        }
        if (preset.values.empty())
            return fail ("params holds no value");

        auto sequence = obj->getProperty ("sequence");
        if (sequence.isString())
            preset.sequence = sequence.toString().trim();

        auto gate = obj->getProperty ("gate");
        if (gate.isString())
            preset.gate = gate.toString().trim();

        auto credit = obj->getProperty ("credit");
        if (credit.isString())
            preset.credit = credit.toString().trim();

        out = std::move (preset);
        return true;
    }

    bool parseText (const String& jsonText, PresetDef& out, String& error)
    {
        var parsed;
        if (auto result = JSON::parse (jsonText, parsed); result.failed())
        {
            error = result.getErrorMessage();
            return false;
        }
        return parse (parsed, out, error);
    }

    String toText (const PresetDef& preset)
    {
        auto* obj = new DynamicObject;
        obj->setProperty ("format", version);
        obj->setProperty ("name", preset.name);

        auto* params = new DynamicObject;
        for (const auto& value : preset.values)
            params->setProperty (value.paramId, static_cast<double> (tidy (value.rawValue)));

        obj->setProperty ("params", var (params));
        if (preset.sequence.isNotEmpty())
            obj->setProperty ("sequence", preset.sequence);
        if (preset.gate.isNotEmpty())
            obj->setProperty ("gate", preset.gate);
        if (preset.credit.isNotEmpty())
            obj->setProperty ("credit", preset.credit);

        return JSON::toString (var (obj), false, 6);
    }
}

namespace
{
    struct Bank
    {
        std::vector<PresetDef> presets;
        std::vector<String> errors;
    };

    const Bank& bank()
    {
        static const Bank b = []
        {
            Bank result;
            std::vector<std::pair<String, PresetDef>> loaded;

            for (int i = 0; i < PresetBank::namedResourceListSize; ++i)
            {
                const char* resource = PresetBank::namedResourceList[i];
                int size = 0;
                const char* data = PresetBank::getNamedResource (resource, size);
                if (data == nullptr || size <= 0)
                {
                    result.errors.push_back (String (resource) + ": not found in the compiled-in bank");
                    continue;
                }

                const char* original = PresetBank::getNamedResourceOriginalFilename (resource);
                const String file = original != nullptr ? String (original) : String (resource);

                PresetDef def;
                String error;
                if (! PresetFormat::parseText (String::fromUTF8 (data, size), def, error))
                {
                    result.errors.push_back (file + ": " + error);
                    continue;
                }
                loaded.push_back ({ file, std::move (def) });
            }

            // the numeric prefix of each file name is what fixes the program order
            std::stable_sort (loaded.begin(), loaded.end(),
                              [] (const auto& x, const auto& y) { return x.first < y.first; });

            for (auto& entry : loaded)
                result.presets.push_back (std::move (entry.second));
            return result;
        }();

        return b;
    }
}

const std::vector<PresetDef>& factoryPresets()
{
    return bank().presets;
}

const std::vector<juce::String>& factoryPresetErrors()
{
    return bank().errors;
}
