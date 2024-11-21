#pragma once

#include <string>

using std::string;

class Segment {
    string content;
    size_t len = 0;

public:
    Segment *add(const string& str, const size_t size);
    Segment *add(const string& str);
    Segment *addForm(const std::string &str);

    [[nodiscard]] std::string getContent() const;
    [[nodiscard]] std::string::size_type getLen() const;
};
