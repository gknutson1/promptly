#include <cstring>
#include <iostream>
#include <unistd.h>
#include <utmp.h>
#include <fstream>

#include "Segment/Segment.h"
#include "term.h"

/**
 * Check whether we are connected over ssh. Looks up the user based on the current tty/pty in the utmp file.
 * @return true if the user is connected via ssh, false otherwise.
 */
bool ShellRemote() {
    utmp udata {.ut_type = USER_PROCESS};

    // utmp stores tty w/o /dev (e.g pts/1 instead of /dev/pts/1) so
    // we need to offset ttyname() by the proper number of bytes to remove the leading /dev/
    constexpr int offset = sizeof "/dev/" - 1;
    // We need to use strcpy instead of directly assigning to udata.ut_line because
    // ut_line is a char[] and ttyname() returns a char*
    strcpy(udata.ut_line, ttyname(STDIN_FILENO) + offset);
    // Resets to the beginning of the utmp file
    setutent();
    // Get the correct utmp entry
    const utmp *data = getutline(&udata);

    // If getutline() can't find our pty, fall back on checking if the SSH_CONNECTION environment variable is set.
    // This is less reliable because environment variables are not necessarily preserved in some situations
    // (e.g. when in sudo or su). However, some situations (e.g. terminal emulators that don't correctly
    // register themselves to utmp) may result in the current pts not being in the utmp file, requiring the use
    // of the fallback method.
    if (data == nullptr) { return getenv("SSH_CONNECTION") != nullptr; }

    // If connected via ssh, the ip address will be stored in ut_addr_v6.
    // This is true even if the user is connected via ipv4 - the address is just
    // stored in the first element of the array.
    return data->ut_addr_v6[0] != 0;
}

/**
 * Add your username and the system hostname to the given segment
 * @param seg Segment to add data to
 */
void addUserHost(Segment &seg) {
    // If we are root, make the username red
    if (getuid() == 0) { seg.addForm(fore::RED); }
    else { seg.addForm(fore::LIGHT_BLUE); }

    seg.add(getlogin())->addForm(ctrl::RESET_FG)->add('@');

    char hostname[_SC_HOST_NAME_MAX];
    gethostname(hostname, _SC_HOST_NAME_MAX);

    // If we are connected over ssh, make the hostname yellow
    if (ShellRemote()) { seg.addForm(fore::YELLOW); }
    else { seg.addForm(fore::LIGHT_BLUE); }
    seg.add(hostname);
}

// How many characters to allocate for the time.
// Must be one more than the string length, as a '\n' will be added
// Currently set to 9 (HH:MM:SS\n)
#define TIME_LEN 9

/**
 * Add the current time to the given segment
 * @param seg Segment to add data to
 */
void addTime(Segment &seg) {
    char timestr[TIME_LEN] = {};
    const time_t cur_time = time(nullptr);

    // "%T" equivalent to "%H:%M:%S"
    strftime(timestr, TIME_LEN, "%T", localtime(&cur_time));
    seg.add(timestr);
}

int main() {
    Segment left;
    Segment right;

    left.add(getenv("PWD"));

    addUserHost(right);
    addTime(right);

    std::cout << left.getContent() << right.getContent() << std::endl;
}
