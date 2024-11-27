#pragma once

#define INT_R_DIV ( back::DEFAULT + " " + fore::BG_DEFAULT + ctrl::RESET_BG + "\ue0b0" + fore::DEFAULT )
#define INT_L_DIV ( ctrl::RESET_BG + fore::BG_DEFAULT + "\ue0b2" + fore::DEFAULT + back::DEFAULT + " " )

#include <string>
#include <utility>

#include "../term.h"

using std::string;

class Segment {
    // R_DIV and L_DIV can't be constexpr because GCC doesn't like constexpr strings w/ a length over 15 characters
    inline static const string R_DIV = INT_R_DIV;
    inline static const string L_DIV = INT_L_DIV;
    static constexpr size_t base_len = strlen(INT_R_DIV) + strlen(INT_R_DIV);

    const string sep;
    const size_t sep_len;

    string content;
    size_t len = base_len;

public:
    // ReSharper disable once CppNonExplicitConvertingConstructor
    // NOLINTNEXTLINE(google-explicit-constructor)
    Segment(string sep, const size_t len): sep(std::move(sep)), sep_len(len) {};

    Segment *add(const string &str, size_t size);

    Segment *add(const string &str);

    Segment *add(char chr);

    Segment *addForm(const string &str);

    Segment *addSep();

    [[nodiscard]] string getContent() const;
    [[nodiscard]] string::size_type getLen() const;
};
