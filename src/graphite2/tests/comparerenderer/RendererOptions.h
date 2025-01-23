// SPDX-License-Identifier: MIT OR MPL-2.0 OR GPL-2.0-or-later-2.1-or-later
// Copyright 2010, SIL International, All rights reserved.
#pragma once

class Option
{
public:
    typedef enum {
        OPTION_BOOL,
        OPTION_STRING,
        OPTION_INT,
        OPTION_FLOAT
    } OptionType;
    Option(const char * shortOpt, const char * longOpt, const char * desc, OptionType type)
        : m_shortOption(shortOpt), m_longOption(longOpt), m_description(desc),
        m_type(type), m_position(-1)
    {

    }
    ~Option()
    {

    }
    void setPosition(int i) { m_position = i; }
    const char * shortOption() const { return m_shortOption; }
    const char * longOption() const { return m_longOption; }
    const char * description() const { return m_description; }
    bool exists() { return m_position > -1; }
    const char * get(char ** argv) const { return (m_position > -1)? argv[m_position] : NULL; }
    int getInt(char ** argv) const { return (m_position > -1)? atoi(argv[m_position]) : 0; }
    float getFloat(char ** argv) const { return (m_position > -1)? static_cast<float>(atof(argv[m_position])) : 0.0f; }
    OptionType type() const { return m_type; }
private:
    const char * m_shortOption;
    const char * m_longOption;
    const char * m_description;
    OptionType m_type;
    int m_position;
};

// enum must be in same order as rendererOptions
typedef enum {
    OptTextFile,
    OptFontFile,
    OptSize,
    OptScript,
    OptGraphite,
    OptGraphite2,
    OptGraphite2s,
    OptHarfbuzzNg,
    OptHarfbuzz,
    OptIcu,
    OptUniscribe,
    OptRtl,
    OptRepeat,
    OptTolerance,
    OptFractionalTolerance,
    OptCompare,
    OptLogFile,
    OptVerbose,
    OptAlternativeFont,
    OptIgnoreGlyphIdDifferences,
    OptTrace,
    OptDemand,
    OptFeatures,
    OptAltFeatures,
    OptQuiet
} OptionId;

static Option rendererOptions[] = {
    Option("-t", "--text", "Text file", Option::OPTION_STRING),
    Option("-f", "--font", "Font file", Option::OPTION_STRING),
    Option("-s", "--size", "Font size", Option::OPTION_INT),
    Option("-S", "--script", "Script tag", Option::OPTION_STRING),
    Option("-g", "--graphite", "Use Graphite renderer", Option::OPTION_BOOL),
    Option("-n", "--graphite2", "Use Graphite2 renderer", Option::OPTION_BOOL),
    Option("-N", "--graphite2uc", "Use Graphite2 uncached renderer", Option::OPTION_BOOL),
    Option("-h", "--harfbuzzng", "Use Harfbuzz NG renderer", Option::OPTION_BOOL),
    Option("-H", "--harfbuzz", "Use Harfbuzz renderer", Option::OPTION_BOOL),
    Option("-i", "--icu", "Use ICU renderer", Option::OPTION_BOOL),
    Option("-u", "--uniscribe", "Use Uniscribe renderer with specified usp10.dll", Option::OPTION_STRING),
    Option("-r", "--rtl", "Right to left", Option::OPTION_BOOL),
    Option("", "--repeat", "Number of times to rerun rendering", Option::OPTION_INT),
    Option("", "--tolerance", "Ignore differences in position smaller than this", Option::OPTION_FLOAT),
    Option("", "--fractional-tolerance", "Ignore differences in position smaller than this", Option::OPTION_FLOAT),
    Option("-c", "--compare", "Compare glyph output", Option::OPTION_BOOL),
    Option("-l", "--log", "Log file for results instead of stdout", Option::OPTION_STRING),
    Option("-v", "--verbose", "Output lots of info", Option::OPTION_BOOL),
    Option("-a", "--alt-font", "Alternative font file", Option::OPTION_STRING),
    Option("", "--ignore-gid", "Ignore Glyph IDs in comparison (use with -c -a alt.ttf)", Option::OPTION_BOOL),
    Option("", "--trace", "JSON trace log file", Option::OPTION_STRING),
    Option("", "--demand", "Load glyphs on demand", Option::OPTION_BOOL),
    Option("", "--features", "Feature list", Option::OPTION_STRING),
    Option("", "--alt-features", "Feature list for alternative font (if different)", Option::OPTION_STRING),
    Option("-q", "--quiet", "Supress all output, including error messages", Option::OPTION_BOOL)
};

const int NUM_RENDERER_OPTIONS = sizeof(rendererOptions) / sizeof(Option);

void showOptions()
{
    const char * optionTypeDesc[] = {
        "", "arg", "int", "float"
    };
    for (size_t i = 0; i < NUM_RENDERER_OPTIONS; i++)
    {
        fprintf(stderr, "%s %s\t%s\t%s\n", rendererOptions[i].shortOption(),
                rendererOptions[i].longOption(),
                optionTypeDesc[rendererOptions[i].type()],
                rendererOptions[i].description());
    }
}


bool parseOptions(int argc, char ** argv)
{
    for (int i = 1; i < argc; i++)
    {
        bool known = false;
        for (int j = 0; j < NUM_RENDERER_OPTIONS && (!known); j++)
        {
            if ((strcmp(argv[i], rendererOptions[j].longOption()) == 0) ||
                ((strlen(rendererOptions[j].shortOption()) > 0) &&
                 (strcmp(argv[i], rendererOptions[j].shortOption()) == 0)))
            {
                known = true;
                if (rendererOptions[j].type() > Option::OPTION_BOOL)
                {
                    if (argc > i + 1)
                    {
                        rendererOptions[j].setPosition(++i);
                    }
                    else
                    {
                        return false;
                    }
                }
                else
                {
                    rendererOptions[j].setPosition(i);
                }
            }
        }
        if (!known) return false;
    }
    return true;
}
