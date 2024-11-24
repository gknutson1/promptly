#pragma once

#include <string>

#define genSEQ(str) (std::string("\033[") + str + "m")
#define genFG(str) (genSEQ("38;5;" + str))
#define genBG(str) (genSEQ("48;5;" + str))

class fore {
public:
    static constexpr std::string RESET      = "\033[0m";
    static constexpr std::string WHITE      = genFG("7");
    static constexpr std::string LIGHT_BLUE = genFG("6");
    static constexpr std::string DARK_BLUE  = genFG("4");
    static constexpr std::string RED        = genFG("1");
    static constexpr std::string YELLOW     = genFG(DEFAULT_FG);
};

class back {
    static constexpr std::string WHITE      = "\033[48;5;7m";
    static constexpr std::string DEFAULT    = "\033[48;5;236m";
};
