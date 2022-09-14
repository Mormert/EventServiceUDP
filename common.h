#ifndef EVENTSERVICE_COMMON_H
#define EVENTSERVICE_COMMON_H

#include <string>
#include <sstream>
#include <vector>

std::vector<std::string> split(const std::string &str, char delimiter) {
    std::vector<std::string> internal;
    std::stringstream ss(str);
    std::string tok;

    while (getline(ss, tok, delimiter)) {
        internal.push_back(tok);
    }

    return internal;
}

#endif //EVENTSERVICE_COMMON_H
