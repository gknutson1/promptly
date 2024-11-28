#include <cstring>
#include <filesystem>
#include <iostream>
#include <unistd.h>
#include <utmp.h>
#include <fstream>
#include <sys/mman.h>
#include <fcntl.h>
#include <semaphore.h>

#include "Segment/Segment.h"
#include "term.h"

namespace fs = std::filesystem;

/**
 * Check whether we are connected over ssh. Looks up the user based on the current tty/pty in the utmp file.
 * @return true if the user is connected via ssh, false otherwise.
 */
bool ShellRemote() {
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wmissing-field-initializers"
    utmp udata {.ut_type = USER_PROCESS};
    #pragma GCC diagnostic pop

    // utmp stores tty w/o /dev (e.g. pts/1 instead of /dev/pts/1) so
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

void addBat(Segment &seg, bool addSep = false) {
    fs::path bat;

    for (auto const& dir_entry : std::filesystem::directory_iterator{"/sys/class/power_supply"}) {
        std::ifstream file (dir_entry.path() / "type");
        std::string type;
        file >> type;
        if (type == "Battery") { bat = dir_entry; break; }
    }

    if (bat.empty()) { return; }
    if (addSep) { seg.addSep(); }

    std::string buf;
    std::ifstream file (bat / "capacity");
    file >> buf;
    seg.add(buf);

}

void addCPU(Segment &seg) {
    #define NAME "/promptly"
    #define MODE 0666

    struct stat {
        unsigned long total = 0;
        unsigned long idle = 0;

    };

    // Connect to the shared memory page, and catch the error if it doesn't exist
    int fd = shm_open(NAME, O_RDWR, MODE);
    const bool needs_init = fd == -1 && errno == ENOENT;

    // If our shared page is missing: reset errno, create the shared memory file, and set it's size
    if (needs_init) {
        errno = 0;
        fd = shm_open(NAME, O_CREAT | O_RDWR, MODE);
        ftruncate(fd, sizeof(stat));
    }

    // Map the shared page to our struct
    stat *prev = static_cast<stat*>(mmap(nullptr, sizeof(stat), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0));

    // Get the lock for the data
    sem_t *lock = sem_open(NAME, O_CREAT, MODE, 1);

    // If the shared memory is currently locked, wait for it to unlock, and then lock it ourselves.
    sem_wait(lock);
    sem_post(lock);

    // ReSharper disable once CppDFAMemoryLeak
    if (needs_init) { prev = new stat; }

    // Get the first line of /proc/stat
    string buf;
    std::ifstream file("/proc/stat");
    getline(file, buf);

    // Erase the "cpu" line at the start of buf
    buf.erase(0, 3);

    // Get a pointer to the first char in buf,
    // and make str_end a pointer to that pointer.
    // Since strtol will "wind" str_end forward whenever
    // it consumes a char, this allows us to automatically
    // move forward through buf every time we call strtol
    char *str = buf.data();
    char **str_end = &str;

    unsigned long total = 0;
    unsigned long idle = 0;

    // Read buf to get all the relevant cpu counters
    total += std::strtol(str, str_end, 10); // user
    total += std::strtol(str, str_end, 10); // nice
    total += std::strtol(str, str_end, 10); // system

    // Idle and iowait are the two proc counters that indicate idle cpu
    idle += std::strtol(str, str_end, 10); // idle
    idle += std::strtol(str, str_end, 10); // iowait
    total += idle;

    total += std::strtol(str, str_end, 10); // irq
    total += std::strtol(str, str_end, 10); // softirq
    total += std::strtol(str, str_end, 10); // steal
    total += std::strtol(str, str_end, 10); // guest
    total += std::strtol(str, str_end, 10); // guest_nice

    int usage = 100 * (idle - prev->idle) / (total - prev->total);

    prev->total = total;
    prev->idle = idle;

    // Release our lock on the shared memory
    sem_post(lock);

    seg.add(std::to_string(usage));

}

int main() {
    Segment left{fore::DEFAULT + " " + chars::L_SEP + " ", chars::L_SEP_LEN + 2};
    Segment right{fore::DEFAULT + " " + chars::R_SEP + " ", chars::R_SEP_LEN + 2};

    left.add(getenv("PWD"));

    addUserHost(right);
    right.addSep();
    addTime(right);

    addBat(right, true);
    addCPU(right);

    std::cout << left.getContent() << right.getContent() << std::endl;
}