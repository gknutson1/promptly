#pragma once
#include "../Element/Element.h"

class Segment;

class Path {
    [[nodiscard]] static bool canAccess(const char* path);
    static size_t minimize(string& raw_path, string& path);
public:
    static size_t addPath(Segment &segment, size_t max_len);
};
