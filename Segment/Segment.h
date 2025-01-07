#pragma once

#define INT_R_DIV ( back::DEFAULT + " " + fore::BG_DEFAULT + ctrl::RESET_BG + "\ue0b0" + fore::DEFAULT )
#define INT_L_DIV ( ctrl::RESET_BG + fore::BG_DEFAULT + "\ue0b2" + fore::DEFAULT + back::DEFAULT + " " )

#include <forward_list>
#include <string>
#include <utility>

#include "../term.h"
#include "../Element/Element.h"

using std::string;

class Segment {
    // R_DIV and L_DIV can't be constexpr because GCC doesn't like constexpr strings w/ a length over 15 characters
    inline static const string R_DIV = INT_R_DIV;
    inline static const string L_DIV = INT_L_DIV;
    static constexpr size_t base_len = strlen(INT_R_DIV) + strlen(INT_R_DIV);

    const string sep;
    const size_t sep_len;

    std::forward_list<Element> elements;

public:
    Segment(string sep, const size_t len): sep(std::move(sep)), sep_len(len) {}

    std::forward_list<Element> *getList() { return &elements; };
    void add(const Element& element) { elements.push_front(element); }
    void add(const string& element) { elements.emplace_front(element); }

    [[nodiscard]] size_t getLen() const;
    [[nodiscard]] string getContent() const;
};
