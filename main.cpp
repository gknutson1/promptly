// === Memory page information ===
// the name of the shared memory page, and it's mode. Also applies to the semaphore.
#define PAGE_NAME "/promptly"
#define PAGE_MODE 0666

// === Time information ===
// Format for the time display.

// "%T" is equivalent to "%H:%M:%S"
#define TIME_FORMAT "%T"
// How many characters to allocate for the time.
// This MUST be at least one larger than the output length of TIME_FORMAT,  as a '\n' will be added to the end.
#define TIME_LEN 9

// === Battery limits ===
// At what charge level to change the color of the battery indicator. The largest parameter that is larger or equal to
// the current battery level will be applied. BAT_HIGH must be 100.

// Indicator will be red and blinking. If battery is charging, this will not apply and will fall back to BAT_WARN.
#define BAT_ALARM 5
// Indicator will be red.
#define BAT_WARN 15
// Indicator will be yellow.
#define BAT_NORMAL 80
// Indicator will be green
#define BAT_HIGH 100


#include <cstring>
#include <filesystem>
#include <iostream>
#include <unistd.h>
#include <utmp.h>
#include <fstream>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <limits.h>
#include <semaphore.h>

#include "Segment/Segment.h"
#include "Element/Element.h"
#include "term.h"
#include "icons.h"
#include "Path/Path.h"

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

    // remote access status can be found in the utmp file, that we can search by
    // using ttyname() to get our tty and looking up that tty with getutline().
    // However utmp stores tty w/o /dev (e.g. pts/1 instead of /dev/pts/1) so
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
 * Add an element containing the username and hostname
 * @param seg Segment to add the element to
 */
void addUserHost(Segment &seg) {
    Element *element = seg.Append();

    // If we are root, make the username red
    if (getuid() == 0) { element->addForm(fore::RED); }
    else { element->addForm(fore::LIGHT_BLUE); }

    element->add(getlogin())->addForm(ctrl::RESET_FG)->add('@');

    char hostname[_SC_HOST_NAME_MAX];
    gethostname(hostname, _SC_HOST_NAME_MAX);

    // If we are connected over ssh, make the hostname yellow
    if (ShellRemote()) { element->addForm(fore::YELLOW); }
    else { element->addForm(fore::LIGHT_BLUE); }
    element->add(hostname);
}

/**
 * Add an element containing the current time
 * @param seg Segment to add the element to
 */
void addTime(Segment &seg) {
    char timestr[TIME_LEN] = {};
    const time_t cur_time = time(nullptr);
    strftime(timestr, TIME_LEN, TIME_FORMAT, localtime(&cur_time));

    seg.Append(timestr);
}

/**
 * Add an element containing the current battery level, if a battery is installed
 * @param seg Segment to add the element to
 * @return true if a battery was found and a element was added, false otherwise
 */
bool addBat(Segment &seg) {
    fs::path bat;

    // Iterate through /sys/class/power_supply to find a entry with a type of "Battery"
    for (auto const& dir_entry : std::filesystem::directory_iterator{"/sys/class/power_supply"}) {
        std::ifstream file (dir_entry.path() / "type");
        std::string type;
        file >> type;
        if (type == "Battery") { bat = dir_entry; break; }
    }

    if (bat.empty()) { return false; }

    std::string buf;

    // Get current battery capacity
    std::ifstream file (bat / "capacity");
    file >> buf;
    if (buf.empty()) { return false; }

    // Convert capacity to integer
    int pwr = std::stoi(buf);

    Element *element = seg.Append();

    if      (pwr <= BAT_ALARM)  element->addForm(ctrl::BLINK + fore::RED);
    else if (pwr <= BAT_WARN)   element->addForm(fore::RED);
    else if (pwr <= BAT_NORMAL) element->addForm(fore::YELLOW);
    else if (pwr <= BAT_HIGH)   element->addForm(fore::GREEN);

    element->add(buf + " ");

    // Battery icons are in steps of 10, so we need to round capacity to the tens place
    int pwr_increment =  pwr / 10 + (pwr % 10 >= 5);

    // Determine if we are plugged in and add the relevant icon
    file = bat / "status";
    file >> buf;

    if (buf == "Charging" || buf == "Full") {
        element->addIcon(bat_charge[pwr_increment]);
    } else {
        element->addIcon(bat_drain[pwr_increment]);
    }

    element->addForm(ctrl::RESET + back::DEFAULT);

    return true;
}

/**
 * Add an element containing the current cpu usage. This utilizes a shared memory page
 * to share the previous cpu counters with.
 * @param seg Segment to add the element to
 */
void addCPU(Segment& seg) {
    struct stat {
        unsigned long total = 0;
        unsigned long used = 0;

    };

    // Connect to the shared memory page, and catch the error if it doesn't exist
    int fd = shm_open(PAGE_NAME, O_RDWR, PAGE_MODE);
    const bool needs_init = fd == -1 && errno == ENOENT;

    // If our shared page is missing: reset errno, create the shared memory file, and set it's size
    if (needs_init) {
        errno = 0;
        fd = shm_open(PAGE_NAME, O_CREAT | O_RDWR, PAGE_MODE);
        ftruncate(fd, sizeof(stat));
    }

    // Map the shared page to our struct
    stat *prev = static_cast<stat*>(mmap(nullptr, sizeof(stat), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0));

    // Get the lock for the data - if it doesn't exist, create it in an unlocked state
    sem_t *lock = sem_open(PAGE_NAME, O_CREAT, PAGE_MODE, 1);

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

    Element *element = seg.Append(std::to_string(usage));
    element->add(" " + chars::CPU + " ", 3);
}

/**
 * Add an element containing information on the current python environment, if in a venv/virtualenv
 * @param seg Segment to add the element to
 * @return true if a python virtual environment was detected and a element was added, false otherwise
 */
bool addPythonEnv(Segment &seg) {
    // this needs to be a char* and not a string because if the environment variable
    // does not exists, std::getenv returns null which causes the string constructor to crash
    const char* cname = std::getenv("VIRTUAL_ENV_PROMPT");
    string name;

    if (cname == nullptr) {
        // If VIRTUAL_ENV_PROMPT is empty, try VIRTUAL_ENV
        cname = getenv("VIRTAUL_ENV");
        // If still empty, assume not using a virtual environment
        if (cname == nullptr) { return false; }
        // If using VIRTUAL_ENV, use only the last path segment
        name = cname;
        name = name.substr(name.find_last_of(fs::path::preferred_separator) + 1);
    } else {
        // If they exist, clear the parenthesis surrounding the prompt
        name = cname;
        if (name.starts_with("(")) { name.erase(0, 1); }
        if (name.ends_with(")")) { name.erase(name.size() - 1, 1); }
    }

    seg.Append()->add(name + " " + chars::PYTHON + " ", 3);
    return true;
}

/**
 * Add an element containing the nerd font icon for the current distro. Find the distro by reading /etc/os-release.
 * If we can't find a match, default to the linux "tux" icon.
 * @param seg Segment to add the element to
 */
void getIcon(Segment &seg) {
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
        else { str[i] = (char) tolower(c); }
        if (i <= 0) { break; }
    }

    const std::string& icon = icons.at(str);

    // Default to the linux penguin ("tux") if we don't know the icon
    if (icon.empty()) { seg.Append()->add(icons.at("tux"), 1); }
    else { seg.Append()->add(icon, 1); }
}

bool statusOK(Segment &seg, const int argc, char **argv) {
    if (argc <= 1) { return true; }

    bool ok = true;
    string err;

    // Loop through all arguments
    for (int arg = 1; arg < argc; arg++) {
        // Check if any character in the arguments is not 0 - this indicates an error
        for (char chr = argv[arg][0]; chr; ++chr) { if (chr != '0') ok = false; }

        // Add error code to the error string. Add '|' characters between every error.
        err += argv[arg];
        if (arg < argc - 1) err += '|';
    }

    if (!ok) seg.Append()->addForm(fore::RED)->add(err);
    return ok;
}

size_t getSize() {
    winsize size; // NOLINT(*-pro-type-member-init)
    ioctl(STDERR_FILENO, TIOCGWINSZ, &size);
    return size.ws_col;
}

int main(const int argc, char **argv) {
    Segment left{fore::DEFAULT + " " + chars::L_SEP + " ", chars::L_SEP_LEN + 2};
    Segment right{fore::DEFAULT + " " + chars::R_SEP + " ", chars::R_SEP_LEN + 2};

    const bool status = statusOK(right, argc, argv);

    addUserHost(right);
    addTime(right);

    addBat(right);
    addCPU(right);
    addPythonEnv(right);

    getIcon(left);

    const size_t term_size = getSize();
    size_t remain = term_size - left.getLen() - right.getLen();

    remain = Path::addPath(left, remain ? remain : INT_MAX);

    // We use fputs instead of puts to avoid a newline
    fputs(left.getContent().c_str(), stdout);



    if (term_size) {
        string sep;
        sep.resize_and_overwrite(remain * 2, [](char* str, const size_t size) {
            for (size_t i = 0; i < size; i += 2)
                strcpy(&str[i], chars::M_SEP.c_str());

            return size;
        });

        fputs(sep.c_str(), stdout);
    }

    fputs((right.getContent() + "\n").c_str(), stdout);

    fputs(((status ? fore::GREEN : fore::RED) + "❯" + ctrl::RESET).c_str(), stdout);
}
