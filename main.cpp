#include <iostream>
#include <unistd.h>

#include "Segment/Segment.h"
#include "term.h"

void addUserHost(Segment &seg) {

    if (getuid() == 0) { seg.addForm(term::RED); }
    else { seg.addForm(term::LIGHT_BLUE); }

    seg.add(getlogin())->addForm(term::RESET)->add('@');

    char hostname[_SC_HOST_NAME_MAX];
    gethostname(hostname, _SC_HOST_NAME_MAX);

    seg.add(hostname);
}

int main() {
    Segment left;
    Segment right;

    left.add(getenv("PWD"));

    addUserHost(right);

    std::cout << left.getContent() << " | " << right.getContent() << std::endl;
}
