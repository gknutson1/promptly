#include "Segment.h"

using std::string;

Segment *Segment::add(const string &str, const size_t size) {
    content.append(str);
    len += size;

    return this;
}

Segment *Segment::add(const string &str) { return add(str, str.size()); }

Segment *Segment::add(const char chr) { return add({chr}, 1); }

Segment *Segment::addForm(const std::string &str) { return add(str, 0); }

Segment * Segment::addSep() { return add(sep, sep_len); }

std::string Segment::getContent() const { return L_DIV + content + R_DIV; }

std::string::size_type Segment::getLen() const { return len; }
