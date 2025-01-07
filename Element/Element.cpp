#include "Element.h"

#include <cstring>

Element::Element(const string &content): content(content), len(content.length()) {}

Element *Element::add(const string &str, const size_t size) {
    content += str;
    len += size;

    return this;
}

Element *Element::add(const string &str) { return add(str, str.length()); }

Element *Element::add(const char chr) { return add({chr}, 1); }

Element *Element::addForm(const string &str) { return add(str, 0); }

string Element::getContent() const { return content; }

string::size_type Element::getLen() const { return len; }
