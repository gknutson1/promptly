#pragma once

#include <string>

#define genSEQ(str) (std::string("\033[") + str + "m")
#define genFG(str) (genSEQ("38;5;" + str))
#define genBG(str) (genSEQ("48;5;" + str))
#define DEFAULT_FG "246"
#define DEFAULT_BG "236"

class fore {
public:
    static constexpr std::string WHITE      = genFG("7");
    static constexpr std::string LIGHT_BLUE = genFG("6");
    static constexpr std::string DARK_BLUE  = genFG("4");
    static constexpr std::string RED        = genFG("1");
    static constexpr std::string YELLOW     = genFG("3");
    static constexpr std::string DEFAULT    = genFG(DEFAULT_FG);
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
