#include "Segment.h"

using std::string;

size_t Segment::getLen() const {
    size_t size = base_len;

    auto iter = elements.begin();
    size += iter->getLen();
    ++iter;

    while (iter != elements.end()) {
        size += sep_len + iter->getLen();
        ++iter;
    }

    return size;
}

string Segment::getContent() const {
    auto iter = elements.begin();
    string s = L_DIV + iter->getContent();
    ++iter;

    while (iter != elements.end()) {
        s += sep + iter->getContent();
        ++iter;
    }

    s += R_DIV;

    return s;
}
