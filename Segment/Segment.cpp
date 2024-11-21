#include "Segment.h"

#include <cstring>

using std::string;

Segment * Segment::add(const string &str, const size_t size) {
    content.append(str);
    len += size;

    return this;
}

Segment * Segment::add(const string &str) { return add(str, str.size()); }

Segment * Segment::addForm(const std::string &str) { return add(str, 0); }

std::string Segment::getContent() const { return content; }

std::string::size_type Segment::getLen() const { return len; }
