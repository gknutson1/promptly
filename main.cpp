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
#include "Element/Element.h"
#include "term.h"
#include "icons.h"

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
 * Create an Element containing the username and the system hostname
 */
Element addUserHost() {
    Element element;

    // If we are root, make the username red
    if (getuid() == 0) { element.addForm(fore::RED); }
    else { element.addForm(fore::LIGHT_BLUE); }

    element.add(getlogin())->addForm(ctrl::RESET_FG)->add('@');

    char hostname[_SC_HOST_NAME_MAX];
    gethostname(hostname, _SC_HOST_NAME_MAX);

    // If we are connected over ssh, make the hostname yellow
    if (ShellRemote()) { element.addForm(fore::YELLOW); }
    else { element.addForm(fore::LIGHT_BLUE); }
    element.add(hostname);

    return element;
}

// How many characters to allocate for the time.
// Must be one more than the string length, as a '\n' will be added
// Currently set to 9 (HH:MM:SS\n)
#define TIME_LEN 9

/**
 * Create an element containing the current time
 */
Element addTime() {

    char timestr[TIME_LEN] = {};
    const time_t cur_time = time(nullptr);

    // "%T" equivalent to "%H:%M:%S"
    strftime(timestr, TIME_LEN, "%T", localtime(&cur_time));

    return {timestr};
}

void addBat(Segment &seg) {
    fs::path bat;

    // Iterate through /sys/class/power_supply to find a entry with a type of "Battery"
    for (auto const& dir_entry : std::filesystem::directory_iterator{"/sys/class/power_supply"}) {
        std::ifstream file (dir_entry.path() / "type");
        std::string type;
        file >> type;
        if (type == "Battery") { bat = dir_entry; break; }
    }

    if (bat.empty()) { return; }

    std::string buf;

    // Get current battery capacity
    std::ifstream file (bat / "capacity");
    file >> buf;
    Element element(buf + " ");

    // Convert capacity to integer
    int pwr = std::stoi(buf);
    // Battery icons are in steps of 10, so we need to round capacity to the tens place
    int pwr_increment =  pwr / 10 + (pwr % 10 >= 5);

    // Determine if we are plugged in and add the relevant icon
    file = bat / "status";
    file >> buf;

    if (buf == "Charging" || buf == "Full") {
        element.add(bat_charge.at(pwr_increment), 1);
    } else {
        element.add(bat_drain.at(pwr_increment), 1);
    }

    seg.getList()->emplace_front(element);
}

Element addCPU() {
    #define NAME "/promptly"
    #define MODE 0666

    struct stat {
        unsigned long total = 0;
        unsigned long used = 0;

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

    // Get the lock for the data - if it doesn't exist, create it in an unlocked state
    sem_t *lock = sem_open(NAME, O_CREAT, MODE, 1);

    // If the shared memory is currently locked, wait for it to unlock, and then lock it ourselves.
    sem_wait(lock);

    // ReSharper disable once CppDFAMemoryLeak
    // Initialize the data in the shared memory page, if needed
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

    unsigned long used = 0;
    unsigned long total = 0;

    // Read buf to get all the relevant cpu counters
    used += std::strtol(str, str_end, 10); // user
    used += std::strtol(str, str_end, 10); // nice
    used += std::strtol(str, str_end, 10); // system

    // Idle and iowait are the two proc counters that indicate idle cpu
    total += std::strtol(str, str_end, 10); // idle
    total += std::strtol(str, str_end, 10); // iowait

    used += std::strtol(str, str_end, 10); // irq
    used += std::strtol(str, str_end, 10); // softirq
    used += std::strtol(str, str_end, 10); // steal
    used += std::strtol(str, str_end, 10); // guest
    used += std::strtol(str, str_end, 10); // guest_nice

    total += used;

    unsigned int usage = static_cast<int>(100l * (used - prev->used) / (total - prev->total));

    prev->total = total;
    prev->used = used;

    // Release our lock on the shared memory
    sem_post(lock);

    Element element(std::to_string(usage));
    element.add(" " + chars::CPU + " ", 3);

    return element;
}

void addPythonEnv(Segment &seg) {
    const char* cname = std::getenv("VIRTUAL_ENV_PROMPT");
    string name;

    if (cname == nullptr) {
        // If VIRTUAL_ENV_PROMPT is empty, try VIRTUAL_ENV
        cname = getenv("VIRTAUL_ENV");
        // If still empty, assume not using a virtual environment
        if (cname == nullptr) { return; }
        // If using VIRTUAL_ENV, use only the last path segment
        name = cname;
        name = name.substr(name.find_last_of(fs::path::preferred_separator) + 1);
    } else {
        // If they exist, clear the parenthesis surrounding the prompt
        name = cname;
        if (name.starts_with("(")) { name.erase(0, 1); }
        if (name.ends_with(")")) { name.erase(name.size() - 1, 1); }
    }

    seg.add(name + " " + chars::PYTHON + " ");
}


/**
 * Get the nerd font icon for the current distro. Find the distro by reading /etc/os-release. If we can't find
 * a match, default to the linux "tux" icon.
 * @return A string containg the two-byte utf8 sequence that corresponds to the current distro
 */
Element getIcon() {
    auto file = std::ifstream("/etc/os-release");
    std::string str;

    // Find the /etc/os-release line that holds the distro name ("NAME=")
    while (!file.eof()) {
        std::getline(file, str);
        if (str.starts_with("NAME=")) { break; }
    }

    // Get the os name (stored between the double quotes)
    size_t start = str.find_first_of('"') + 1;
    size_t length = str.find_last_of('"') - start;
    str = str.substr(start, length);

    // Loop through to delete all " ", "_", and "-" characters from the name, to improve icon detection
    for (size_t i = str.length() - 1;; i--) {
        char c = str[i];
        if (c == ' ' || c == '-' || c == '_') { str.erase(i, 1); }
        else { str[i] = tolower(c); }
        if (i <= 0) { break; }
    }

    const std::string& icon = icons.at(str);

    // Default to the linux penguin ("tux") if we don't know the icon
    if (icon.empty()) { return icons.at("tux"); }
    return {icon};
}

int main() {
    Segment left{fore::DEFAULT + " " + chars::L_SEP + " ", chars::L_SEP_LEN + 2};
    Segment right{fore::DEFAULT + " " + chars::R_SEP + " ", chars::R_SEP_LEN + 2};

    left.add(getenv("PWD"));
    left.add(getIcon());

    right.add(addUserHost());
    right.add(addTime());

    addBat(right);
    right.add(addCPU());
    addPythonEnv(right);

    std::cout << left.getContent() << right.getContent() << std::endl;
}
