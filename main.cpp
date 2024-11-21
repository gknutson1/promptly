#include <iostream>
#include <unistd.h>

#include "Segment/Segment.h"

void addUserHost(Segment &seg) {
    char hostname[_SC_HOST_NAME_MAX];
    gethostname(hostname, _SC_HOST_NAME_MAX);

    seg.add(getlogin())
        ->add('@')
        ->add(hostname);
}

int main() {
    Segment left;
    Segment right;

    left.add(getenv("PWD"));

    addUserHost(right);

    std::cout << left.getContent() << " | " << right.getContent() << std::endl;
}
