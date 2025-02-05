#include "Path.h"

#include <csignal>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <pwd.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>

#include "../List/List.h"
#include "../term.h"
#include "../Segment/Segment.h"


#define SEP '/'

class Segment;
/**
 * Get the homedir of the user. Uses $HOME, and falls back on looking the user up in
 * the password database if $HOME is unset.
 * @return A string representing the current user's homedir
 */
string getHome() {
    char* buf = std::getenv("HOME");
    // If HOME is unset, fall back on our homedir in the password database
    if (buf == nullptr) { buf = getpwuid(getuid())->pw_dir; }

    // Convert from char* to string
    string home(buf);
    // If home has a trailing /, remove it
    if (home.ends_with(SEP)) { home.erase(home.size() - 1); }

    return home;
}

/**
 * Check if a given path can be read and written to by the current user
 * @param path Path to check
 * @return True if the current user has read/write access to path
 */
bool Path::canAccess(const char* path) {
    struct stat info; // NOLINT(*-pro-type-member-init)
    stat(path, &info);

    // If we own pwd, assume we have read/write access
    if (info.st_uid == getuid()) return true;
    // If we don't own pwd, check if others have read/write access
    if (info.st_mode & S_IWOTH && info.st_mode & S_IROTH) return true;
    // If others don't have read_write access, check if group members have read/write access
    // If this is true, we need to loop through current user's groups to see if we have access
    if (info.st_mode & S_IWGRP && info.st_mode & S_IROTH) {
        const int group_count = getgroups(0, nullptr); // Gets a count of the current user's groups
        // We need to be one larger than group_count because user's egid is not guaranteed to be in group_count
        auto *groups = new unsigned int[group_count + 1];
        getgroups(group_count, groups);
        groups[group_count] = getegid();

        // If any of our groups matches pwd's group, we have read/write access
        for (int i = 0; i < group_count + 1; i++ )
            if (groups[i] == info.st_gid) {
                free(groups);
                return true;
            }
        free(groups);
    }
    return false;
}

size_t Path::minimize(string &raw_path, string &path) {
    // Each path element must be at least 1 character long
    string min {path[0]};

    // Loop over every directory
    DIR *dp = opendir(raw_path.c_str());

    // Get the directory info for the current path element.
    // This is so we can get the inode.
    struct stat dir_info; // NOLINT(*-pro-type-member-init)
    fstatat(dirfd(dp), path.c_str(), &dir_info, 0);

    while (const dirent *ep = readdir(dp)) {

        // Since readdir will eventually find the target
        // path in raw_path, we need to skip it.
        if (ep->d_ino == dir_info.st_ino) continue;

        // If ep is smaller than path, we can ignore everything after the end of ep
        size_t max = std::min(path.size(), strlen(ep->d_name));

        // Loop through the string and look for any differences
        for (size_t i = 0; i < max; ++i) {
            if (ep->d_name[i] != path[i]) {
                if (i >= min.size()) min += path[i];
                break;
            }

            if (i >= min.size()) min += path[i];
        }
    }
    closedir(dp);

    raw_path += SEP + path;

    size_t diff = path.size() - min.size();
    path = min;
    return diff;
}

/**
 * Add the current working directory to element, minimized. If the current working directory is in the user's homedir,
 * the homedir will br replaced with a ~. This function will attempt to shrink the cwd to fit within max_len, by first
 * shrinking every path element to it's minimum unique length, and then further shrinking as needed. The last path
 * element will always be full-size.
 * @param segment Segment to add directory information to
 * @param max_len Length at which to stop minimizing.
 */
void Path::addPath(Segment &segment, size_t max_len) {
    Element *element = segment.Append();
    string pwd = get_current_dir_name();
    // If pwd does not have a trailing slash, add one
    if (! pwd.ends_with(SEP)) { pwd += SEP; }
    // -2 for the beginning and ending separators, +3 for the icon
    size_t len = pwd.length() - 1 + 3;

    const string home = getHome();
    const bool in_home = pwd.starts_with(home);
//    const bool in_home = false;

    if (canAccess(pwd.c_str())) { // Check if we have read/write access to pwd
        if (in_home) element->addIcon(chars::HOME); // If we are in our homedir, use the home icon
        else element->addIcon(chars::FOLDER); // If we are outside homedir, use the folder icon
    }
    else element->addIcon(chars::LOCK); // If we don't have read/write access, use the lock icon


    List<string> path; // The actual path, for iterating through
    List<string> display; // What will be displayed
    string raw_path; // Plain string, for passing to stat()

    if (in_home) { // If we are in home, use ~ as a replacement for our homedir
        len -= home.size() - 1; // Shrink len - the extra 2 is for the ~ and the etra seperator
        pwd.erase(0, home.size()); // Remove home from pwd

        raw_path = home;
        display.Append("~");
    }

    // Split pwd on SEP and fill up path
    for (size_t head = 1, i = 1; i < pwd.length(); ++i) {
        if (pwd[i] == SEP) {
            path.Append(pwd.substr(head, i - head));
            head = i + 1;
        }
    }

    // minimize the path to the smallest unique string, until we are within max_len. Skip the last element.
    for (auto i = path.begin(); i.peek() != nullptr; ++i) {
        if (len > max_len && i.peek()->next != nullptr) len -= minimize(raw_path, *i);
        display.Append(*i);
    }

    // If we still aren't small enough, shrink each path element to one character until we are
    // within max_size, starting from left to right and skipping the last element.
    for (auto i = display.begin(); i.peek()->next != nullptr; ++i) {
        auto &s = *i;
        if (s.length() <= 1 ) continue; // If path element is already only one character, skip it
        if (len <= max_len) break; // Exit loop when we are within max_len

        len -= s.length() - 1;
        s = s[0];
    }

    if (!in_home) element->add(SEP);

    element->add(display.toString(string{SEP}));
}