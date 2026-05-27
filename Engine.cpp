#include "Config.hpp"

#include <cerrno>
#include <csignal>
#include <cstring>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <chrono>
#include <iostream>
#include <random>
#include <string>
#include <thread>

namespace {

volatile std::sig_atomic_t g_reader_gone = 0;

void handle_sigpipe(int) {
    g_reader_gone = 1;
}

bool ensure_fifo() {
    struct stat st {};
    const char* path = config::kPipePath.data();
    if (stat(path, &st) == 0) {
        if (!S_ISFIFO(st.st_mode)) {
            std::cerr << "Path exists but is not a FIFO: " << path << '\n';
            return false;
        }
        return true;
    }
    if (mkfifo(path, 0666) != 0) {
        std::cerr << "mkfifo failed: " << std::strerror(errno) << '\n';
        return false;
    }
    return true;
}

}  // namespace

int main() {
    std::signal(SIGPIPE, handle_sigpipe);

    if (!ensure_fifo()) {
        return 1;
    }

    const int fd = open(config::kPipePath.data(), O_WRONLY);
    if (fd < 0) {
        std::cerr << "open(O_WRONLY) failed: " << std::strerror(errno) << '\n';
        return 1;
    }

    std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<int> vol_dist(config::kVolDeltaMin, config::kVolDeltaMax);
    std::uniform_int_distribution<int> obi_dist(config::kObiMin, config::kObiMax);

    while (true) {
        for (const auto ticker : config::kTickerSet) {
            const int vol_delta = vol_dist(rng);
            const int obi = obi_dist(rng);

            std::string line;
            line.reserve(32);
            line.append(ticker);
            line.push_back(',');
            line.append(std::to_string(vol_delta));
            line.push_back(',');
            line.append(std::to_string(obi));
            line.push_back('\n');

            const ssize_t written = write(fd, line.data(), line.size());
            if (written < 0) {
                if (errno == EPIPE || g_reader_gone) {
                    std::cerr << "Reader disconnected.\n";
                    close(fd);
                    return 0;
                }
                std::cerr << "write failed: " << std::strerror(errno) << '\n';
                close(fd);
                return 1;
            }
        }
        std::this_thread::sleep_for(config::kIngestionInterval);
    }
}
