#include "Segment.h"

using std::string;

size_t Segment::getLen() const {
    size_t size = base_len;

    for (const auto *i = start; i != nullptr; i = i->next)
        size += sep_len + i->element.getLen();

    // We hae added the length of sep for every element, but there are one fewer
    // separators that there are elements, so we need to subtract sep_len once
    return size - sep_len;
}

string Segment::getContent() const {
    string s = L_DIV;
    s += start->element.getContent();

    for (const auto *i = start->next; i != nullptr; i = i->next)
        s += sep + i->element.getContent();

    return s + R_DIV;
}
