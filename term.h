#pragma once

#include <string>

using std::string;

#define genSEQ(str) ("\033[" str "m")
#define genFG(str) (genSEQ("38;5;" str))
#define genBG(str) (genSEQ("48;5;" str))
#define DEFAULT_FG "246"
#define DEFAULT_BG "236"

/**
 * Calculate the length of a string as number of characters displayed.
 * Handles Unicode multibyte chars as well as terminal escape sequences
 * @param str The string to count
 * @return The number of characters that would be displayed if the string were printed
 */
static constexpr size_t strlen(string str) {
    size_t result = 0;

    bool term_seq = false;

    for (const char chr : str) {
        // terminal sequences are exited when we reach an 'm' character
        if (term_seq) { if (chr == 'm') term_seq = false; }
        // terminal sequences start with the '\033' char
        else if (chr == '\033') {term_seq = true; }

        // Any byte that starts with the bits "10" is an extension
        // of a utf-s character (the first bit in a multibyte utf-8
        // char starts w/ "11", and a non multibyte char starts w/ "0")
        // so we skip everything that starts w/ "10"
        else if ( (chr & 0xc0) != 0x80) result++;
    }

    return result;
}

class fore {
public:
    static constexpr std::string WHITE      = genFG("7");
    static constexpr std::string LIGHT_BLUE = genFG("6");
    static constexpr std::string DARK_BLUE  = genFG("4");
    static constexpr std::string RED        = genFG("1");
    static constexpr std::string YELLOW     = genFG("3");
    static constexpr std::string DEFAULT    = genFG(DEFAULT_FG);
    static constexpr std::string BG_DEFAULT = genFG(DEFAULT_BG);
};

class back {
public:
    static constexpr std::string WHITE      = "\033[48;5;7m";
    static constexpr std::string DEFAULT    = genBG(DEFAULT_BG);
};

class chars {
public:
    // These are strings, not chars, because they are mostly two-byte Unicode sequences,
    // And it is (probably) easier/more preformant to store them as a string,
    // instead of a char16_t that will need to be converted back into a 2-byte u8 char
    // every time it needs to be added to a segment.
    static constexpr std::string L_SEP = "\ue0b3";
    static constexpr std::string M_SEP = "\u00b7";
    static constexpr std::string R_SEP = "\ue0b1";
};

class ctrl {
public:
    static constexpr std::string RESET = genSEQ("0");
    static constexpr std::string RESET_FG = genSEQ("39");
    static constexpr std::string RESET_BG = genSEQ("49");
};

static std::string R_DIV = back::DEFAULT + " " + fore::BG_DEFAULT + ctrl::RESET_BG + "\ue0b0" + fore::DEFAULT;
static std::string L_DIV = ctrl::RESET_BG + fore::BG_DEFAULT + "\ue0b2" + fore::DEFAULT + back::DEFAULT + " ";