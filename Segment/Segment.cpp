#include "Segment.h"

using std::string;

size_t Segment::getLen() const {
    size_t size = base_len;

    for (const auto &element : elements) { size += sep_len + element.getLen(); }

    // We hae added the length of sep for every element, but there are one fewer
    // separators that there are elements, so we need to subtract sep_len once
    return size - sep_len;
}

string Segment::getContent() const {
    string s = L_DIV;
    // Insert the first item without a seperator
    auto iter = elements.begin();
    s += iter->getContent();
    ++iter;

    // Insert all other items with a seperator
    for (; iter != elements.end(); ++iter) {
        s += sep + iter->getContent();
    }

    return s + R_DIV;
}
