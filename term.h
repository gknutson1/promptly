#pragma once

#include <map>
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
static constexpr size_t u_strlen(string str) {
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

struct fore {
    static constexpr string WHITE      = genFG("7");
    static constexpr string LIGHT_BLUE = genFG("6");
    static constexpr string DARK_BLUE  = genFG("4");
    static constexpr string RED        = genFG("1");
    static constexpr string YELLOW     = genFG("3");
    static constexpr string GREEN      = genFG("2");
    static constexpr string DEFAULT    = genFG(DEFAULT_FG);
    static constexpr string BG_DEFAULT = genFG(DEFAULT_BG);
};

struct back {
    static constexpr string WHITE      = "\033[48;5;7m";
    static constexpr string DEFAULT    = genBG(DEFAULT_BG);
};

struct chars {
    // These are strings, not chars, because they are mostly two-byte Unicode sequences,
    // And it is (probably) easier/more preformant to store them as a string,
    // instead of a char16_t that will need to be converted back into a 2-byte u8 char
    // every time it needs to be added to a segment.
    // M_SEP is an exception becuase we need to create a strip of chars with it.
    static constexpr string R_SEP = "\ue0b3";
    static constexpr size_t R_SEP_LEN = u_strlen(R_SEP);
    // ReSharper disable once CppMultiCharacterLiteral
    static constexpr char16_t M_SEP = '\u00b7';
    static constexpr string L_SEP = "\ue0b1";
    static constexpr size_t L_SEP_LEN = u_strlen(L_SEP);
    static constexpr string CPU = "\uf4bc";
    static constexpr string MEM = "\uefc5";
    static constexpr string CLOCK = "\uf017";
    static constexpr string PYTHON = "\ue73c";
    static constexpr string FOLDER = "\uf115";
    static constexpr string LOCK = "\uf023";
    static constexpr string HOME = "\uf015";
};

static const std::map<int, string> bat_drain = {
    {0, "󱃍"},
    {1, "󰁺"},
    {2, "󰁻"},
    {3, "󰁼"},
    {4, "󰁽"},
    {5, "󰁾"},
    {6, "󰁿"},
    {7, "󰂀"},
    {8, "󰂁"},
    {9, "󰂂"},
    {10, "󰁹"}
};

static const std::map<int, string> bat_charge = {
    {0, "󰢟"},
    {1, "󰢜"},
    {2, "󰂆"},
    {3, "󰂇"},
    {4, "󰂈"},
    {5, "󰢝"},
    {6, "󰂉"},
    {7, "󰢞"},
    {8, "󰂊"},
    {9, "󰂋"},
    {10, "󰂅"}
};

struct ctrl {
    static constexpr string RESET    = genSEQ("0");
    static constexpr string RESET_FG = genSEQ("39");
    static constexpr string RESET_BG = genSEQ("49");
    static constexpr string BLINK    = genSEQ("5");
};