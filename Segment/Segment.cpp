#include "Segment.h"

using std::string;

size_t Segment::getLen() const {
    size_t size = base_len;
    for (const auto& item : elements) { size += item.getLen();; }
    return size;
}

string Segment::getContent() const {
    string s = L_DIV;
    for (const auto& item : elements) { s += item.getContent(); }
    return s + R_DIV;
}
