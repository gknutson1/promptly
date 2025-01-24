#include "Segment.h"

using std::string;

size_t Segment::getLen() const {
    size_t size = base_len;

    for (const auto& i : elements)
        size += sep_len + i.getLen();

    // We hae added the length of sep for every element, but there are one fewer
    // separators that there are elements, so we need to subtract sep_len once
    return size - sep_len;
}

string Segment::getContent() const {
    string s = L_DIV;

    auto begin = elements.begin();
    const auto end = elements.end();

    s += (*begin).getContent();
    ++begin;

    for (auto i = *begin; begin != end; ++begin, i = *begin)
        s += sep + i.getContent();

    return s + R_DIV;
}
