#pragma once

#include <string>

#include "../term.h"

using std::string;

class Segment {
    string content = L_DIV;
    size_t len = 4;

public:
    Segment *add(const string &str, const size_t size);

    Segment *add(const string &str);

    Segment *add(const char chr);

    Segment *addForm(const std::string &str);

    [[nodiscard]] std::string getContent() const;
    [[nodiscard]] std::string::size_type getLen() const;
};
