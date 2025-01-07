#pragma once

#include <string>

using std::string;

class Element {
    string content;
    size_t len = 0;

public:
    Element() = default;
    // ReSharper disable once CppNonExplicitConvertingConstructor
    Element(const string& content); // NOLINT(*-explicit-constructor)

    Element *add(const string &str, size_t size);

    Element *add(const string &str);

    Element *add(char chr);

    Element *addForm(const string &str);

    [[nodiscard]] string getContent() const;
    [[nodiscard]] string::size_type getLen() const;
};
