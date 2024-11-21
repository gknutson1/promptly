#include <iostream>

#include "Segment/Segment.h"

Segment prompt;

int main() {
    prompt.add(getenv("PWD"));
    std::cout << prompt.getContent() << std::endl;
}
