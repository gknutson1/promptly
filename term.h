#pragma once

#include <string>

class term {
public:
    static constexpr std::string WHITE      = "\033[38;5;7m";
    static constexpr std::string LIGHT_BLUE = "\033[38;5;6m";
    static constexpr std::string DARK_BLUE  = "\033[38;5;4m";
    static constexpr std::string RED        = "\033[38;5;1m";
    static constexpr std::string YELLOW     = "\033[38;5;3m";
    static constexpr std::string RESET      = "\033[0m";
};
