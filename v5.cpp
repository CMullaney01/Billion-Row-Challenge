#include <iostream>
#include <string>
#include <unordered_map>
#include <thread>
#include <vector>
#include <algorithm>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <chrono>
#include <iomanip>
#include <cstring>

struct CityInfo {
    int count = 0;
    long sum = 0;
    int min = 9999;
    int max = -9999;
};

void error(const std::string& message) {
    std::cerr << "Error: " << message << std::endl;
    std::exit(EXIT_FAILURE);
}

// Parse temperature directly from raw bytes into int × 10
// e.g. "-12.3" → -123,  "4.5" → 45
static inline int parseTemp(const char* data, off_t& pos) {
    int sign = 1;
    if (data[pos] == '-') {
        sign = -1;
        ++pos;
    }
    int val = 0;
    while (data[pos] != '\n') {
        char c = data[pos];
        if (c != '.')
            val = val * 10 + (c - '0');
        ++pos;
    }
    return sign * val;
}

// Per-thread chunk processor
void processChunk(const char* data, off_t chunk_start, off_t chunk_end,
                  std::unordered_map<std::string, CityInfo>& cityInfos) {

    cityInfos.reserve(512);
    off_t pos = chunk_start;

    // Align to start of a full line (except for the very first chunk)
    if (chunk_start != 0) {
        while (pos < chunk_end && data[pos] != '\n')
            ++pos;
        ++pos;
    }

    while (pos < chunk_end) {
        // City name: find ';' then construct string from pointer range
        const char* nameStart = data + pos;
        while (data[pos] != ';')
            ++pos;
        std::string cityName(nameStart, data + pos);
        ++pos; // skip ';'

        // Temperature: parse directly from raw bytes
        int temperature = parseTemp(data, pos);
        ++pos; // skip '\n'

        // Update stats
        auto& info = cityInfos[cityName];
        info.count++;
        info.sum += temperature;
        if (temperature < info.min) info.min = temperature;
        if (temperature > info.max) info.max = temperature;
    }
}

int main(int argc, char* argv[]) {
    auto total_time_start = std::chrono::steady_clock::now();

    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <filename>" << std::endl;
        return 1;
    }

    // Memory-map the file
    auto map_file_start = std::chrono::steady_clock::now();

    int fd = open(argv[1], O_RDONLY);
    if (fd == -1) error("Failed to open file");

    off_t fileSize = lseek(fd, 0, SEEK_END);
    if (fileSize == -1) error("Failed to determine file size");

    char* mappedFile = static_cast<char*>(
        mmap(nullptr, fileSize, PROT_READ, MAP_SHARED, fd, 0));
    if (mappedFile == MAP_FAILED) error("Failed to map file into memory");

    madvise(mappedFile, fileSize, MADV_SEQUENTIAL);

    auto map_file_end = std::chrono::steady_clock::now();

    // Spawn threads matching hardware concurrency
    auto threads_start = std::chrono::steady_clock::now();

    unsigned int numThreads = std::thread::hardware_concurrency();
    if (numThreads == 0) numThreads = 8;

    std::vector<std::thread> threads;
    std::vector<std::unordered_map<std::string, CityInfo>> cityMaps(numThreads);

    for (unsigned int i = 0; i < numThreads; ++i) {
        off_t start = static_cast<off_t>(i) * fileSize / numThreads;
        off_t end   = static_cast<off_t>(i + 1) * fileSize / numThreads;
        threads.emplace_back(processChunk, mappedFile, start, end,
                             std::ref(cityMaps[i]));
    }

    for (auto& t : threads) t.join();

    auto threads_end = std::chrono::steady_clock::now();

    // Combine per-thread maps
    auto combine_start = std::chrono::steady_clock::now();

    std::unordered_map<std::string, CityInfo> combined;
    combined.reserve(512);

    for (const auto& cityMap : cityMaps) {
        for (const auto& [name, info] : cityMap) {
            auto& c = combined[name];
            c.count += info.count;
            c.sum   += info.sum;
            if (info.min < c.min) c.min = info.min;
            if (info.max > c.max) c.max = info.max;
        }
    }

    auto combine_end = std::chrono::steady_clock::now();

    // Sort & print
    auto sort_start = std::chrono::steady_clock::now();

    std::vector<std::pair<std::string, CityInfo>> sorted(
        combined.begin(), combined.end());
    std::sort(sorted.begin(), sorted.end(),
              [](const auto& a, const auto& b) { return a.first < b.first; });

    for (const auto& [name, res] : sorted) {
        double mean = (res.sum / static_cast<double>(res.count)) / 10.0;
        double mn   = static_cast<double>(res.min) / 10.0;
        double mx   = static_cast<double>(res.max) / 10.0;
        std::cout << "City: " << name
                  << ", Min: " << std::fixed << std::setprecision(1) << mn
                  << ", Mean: " << mean
                  << ", Max: " << mx << '\n';
    }

    auto sort_end     = std::chrono::steady_clock::now();
    auto total_time_end = std::chrono::steady_clock::now();

    // Clean up
    munmap(mappedFile, fileSize);
    close(fd);

    // Timing report
    auto sec = [](auto d) {
        return std::chrono::duration<double>(d).count();
    };
    std::cerr << "Mapping file:       " << sec(map_file_end - map_file_start)  << "s\n"
              << "Threads (" << numThreads << "):       "
              << sec(threads_end - threads_start) << "s\n"
              << "Combining maps:     " << sec(combine_end - combine_start)    << "s\n"
              << "Sort & print:       " << sec(sort_end - sort_start)          << "s\n"
              << "Total:              " << sec(total_time_end - total_time_start) << "s\n";

    return 0;
}
