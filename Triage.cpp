#include "Config.hpp"

#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>

#include <iostream>
#include <string>
#include <string_view>

namespace {

bool parse_line(std::string_view line, std::string_view& ticker, int& vol_delta, int& obi) {
    const std::size_t first = line.find(',');
    if (first == std::string_view::npos) return false;
    const std::size_t second = line.find(',', first + 1);
    if (second == std::string_view::npos) return false;

    ticker = line.substr(0, first);

    const std::string_view vol_sv = line.substr(first + 1, second - first - 1);
    const std::string_view obi_sv = line.substr(second + 1);

    std::string vol_str(vol_sv);
    std::string obi_str(obi_sv);

    char* end = nullptr;
    errno = 0;
    const long vol = std::strtol(vol_str.c_str(), &end, 10);
    if (errno != 0 || *end != '\0') return false;

    errno = 0;
    const long obi_l = std::strtol(obi_str.c_str(), &end, 10);
    if (errno != 0 || *end != '\0') return false;

    vol_delta = static_cast<int>(vol);
    obi = static_cast<int>(obi_l);
    return true;
}

}  // namespace

int main() {
    const int fd = open(config::kPipePath.data(), O_RDONLY);
    if (fd < 0) {
        std::cerr << "open(O_RDONLY) failed: " << std::strerror(errno) << '\n';
        return 1;
    }

    std::string buffer;
    buffer.reserve(4096);
    std::string pending;
    pending.reserve(4096);

    while (true) {
        char chunk[1024];
        const ssize_t n = read(fd, chunk, sizeof(chunk));
        if (n < 0) {
            std::cerr << "read failed: " << std::strerror(errno) << '\n';
            close(fd);
            return 1;
        }
        if (n == 0) {
            continue;
        }

        pending.append(chunk, static_cast<std::size_t>(n));
        std::size_t start = 0;
        while (true) {
            const std::size_t nl = pending.find('\n', start);
            if (nl == std::string::npos) {
                pending.erase(0, start);
                break;
            }

            std::string_view line(pending.data() + start, nl - start);
            start = nl + 1;

            std::string_view ticker;
            int vol_delta = 0;
            int obi = 0;
            if (!parse_line(line, ticker, vol_delta, obi)) {
                continue;
            }

            if (vol_delta > config::kSignalThresholdVolDelta && obi > config::kSignalThresholdObi) {
                std::cout << "\033[1;32m[EXECUTE LONG]\033[0m " << ticker
                          << " VolDelta=" << vol_delta
                          << " OBI=" << obi << '\n';
            } else {
                std::cout << "\033[90m[Monitoring]\033[0m " << ticker
                          << " VolDelta=" << vol_delta
                          << " OBI=" << obi << '\n';
            }
        }
    }
}
