#pragma once

#define INT_R_DIV ( " " + back::DEFAULT + fore::BG_DEFAULT + ctrl::RESET_BG + "\ue0b0" + fore::DEFAULT )
#define INT_L_DIV ( ctrl::RESET_BG + fore::BG_DEFAULT + "\ue0b2" + fore::DEFAULT + back::DEFAULT + " " )

#include <string>

#include "../term.h"
#include "../Element/Element.h"
#include "../List/List.h"

using std::string;

class Segment {
    // R_DIV and L_DIV can't be constexpr because GCC doesn't like constexpr strings w/ a length over 15 characters
    inline static const string R_DIV = INT_R_DIV;
    inline static const string L_DIV = INT_L_DIV;
    static constexpr size_t base_len = u_strlen(INT_R_DIV) + u_strlen(INT_L_DIV);

    const string sep;
    const size_t sep_len;

    List<Element> elements;
public:
    Segment(string sep, const size_t len): sep(std::move(sep)), sep_len(len) {}

    Element* Append() {
        return elements.Append(Element());
    }

    Element* Append(const string& data) {
        return elements.Append(Element(data));
    }

    [[nodiscard]] size_t getLen() const;
    [[nodiscard]] string getContent() const;
};
