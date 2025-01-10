#include "Segment.h"

using std::string;

size_t Segment::getLen() const {
    size_t size = base_len;

    for (const auto &element : elements) { size += sep_len + element.getLen(); }
    return size - sep_len;
}

string Segment::getContent() const {
    string s = L_DIV;
    auto iter = elements.begin();
    s += iter->getContent();
    ++iter;

    for (; iter != elements.end(); ++iter) {
        s += sep + iter->getContent();
    }

    return s + R_DIV;
}
