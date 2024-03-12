#include <iostream>
#include <fstream>
#include <string>
#include <unordered_map>
#include <thread>
#include <vector>
#include <sstream>
#include <algorithm>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <chrono> 

#define CHUNKS 12

struct cityInfo {
    int count;
    double sum, min, max;
};

void error(const std::string& message) {
    std::cerr << "Error: " << message << std::endl;
    std::exit(EXIT_FAILURE);
}

// Function to update cityInfos based on temperature and city
void updateCityInfos(std::string cityName, double temperature, std::unordered_map<std::string, cityInfo>& cityInfos) {
    auto it = cityInfos.find(cityName);
    if (it != cityInfos.end()) {
        it->second.count++;
        it->second.sum += temperature;
        it->second.min = std::min(it->second.min, temperature);
        it->second.max = std::max(it->second.max, temperature);
    } else {
        // Create a new entry if cityName hasn't been seen yet
        cityInfos.insert({cityName, {1, temperature, temperature, temperature}});
    }
}

// Comparator function to sort cityInfos by city name
bool compareCityInfos(const std::pair<std::string, cityInfo>& a, const std::pair<std::string, cityInfo>& b) {
    return a.first < b.first;
}

// Function to process a chunk of data
void Thread(const char* data, off_t chunk_start, off_t chunk_end, std::unordered_map<std::string, cityInfo>& cityInfos) {
    off_t pos = chunk_start;
    std::string line;

    // Process the chunk of data byte by byte
    while (pos < chunk_end) {
        std::string cityName;
        double temperature;
        char c = data[pos]; // Initialize c with data[pos]

        // Read characters until encountering ';' which separates city name and temperature
        while (c != ';' && pos < chunk_end) {
            c = data[pos]; // Read the next character
            cityName.push_back(c);
            ++pos;
        }
        ++pos; // skip ";"

        // Read characters until encountering '\n' which marks the end of temperature
        std::string tempStr;
        while (c != '\n' && pos < chunk_end) {
            c = data[pos]; // Read the next character
            tempStr.push_back(c);
            ++pos;
        }
        temperature = std::stod(tempStr);

        updateCityInfos(cityName, temperature, cityInfos);
    }
}



int main(int argc, char* argv[]) {
    // Check if the filename is provided as an argument
    auto total_time_start = std::chrono::steady_clock::now();
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <filename>" << std::endl;
        return 1;
    }

    std::string filename = argv[1];
    int numChunks = CHUNKS;
    auto map_file_start = std::chrono::steady_clock::now();
    // Open the file
    int fd = open(filename.c_str(), O_RDONLY);
    if (fd == -1)
        error("Failed to open file");

    // Obtain the size of the file
    off_t fileSize = lseek(fd, 0, SEEK_END);
    if (fileSize == -1)
        error("Failed to determine file size");

    // Memory map the file
    char* mappedFile = static_cast<char*>(mmap(nullptr, fileSize, PROT_READ, MAP_PRIVATE, fd, 0));
    if (mappedFile == MAP_FAILED)
        error("Failed to map file into memory");
    auto map_file_end = std::chrono::steady_clock::now();
    auto threads_start = std::chrono::steady_clock::now();
    // Create threads to process chunks
    std::vector<std::thread> threads;
    std::vector<std::unordered_map<std::string, cityInfo>> cityMaps(numChunks); // Vector of city info maps
    for (int i = 0; i < numChunks; ++i) {
        // std::cout << "Starting Thread: " << i << std::endl;
        off_t start = i * fileSize / numChunks;
        off_t end = (i + 1) * fileSize / numChunks; // C 
        threads.emplace_back(Thread, mappedFile, start, end, std::ref(cityMaps[i]));
    }

    // Wait for all threads to finish
    for (auto& thread : threads) {
        thread.join();
    }
    auto threads_end = std::chrono::steady_clock::now();

    // Unmap the file
    if (munmap(mappedFile, fileSize) == -1)
        error("Failed to unmap file");

    // Close the file descriptor
    if (close(fd) == -1)
        error("Failed to close file");

    auto combine_threads_start = std::chrono::steady_clock::now();
    // Combine the city information from all maps into a single map
    std::unordered_map<std::string, cityInfo> combinedCityInfo;
    for (const auto& cityMap : cityMaps) {
        for (const auto& pair : cityMap) {
            const std::string& cityName = pair.first;
            const cityInfo& info = pair.second;
            combinedCityInfo[cityName].sum += info.sum;
            combinedCityInfo[cityName].count += info.count;
            combinedCityInfo[cityName].min = std::min(combinedCityInfo[cityName].min, info.min);
            combinedCityInfo[cityName].max = std::max(combinedCityInfo[cityName].max, info.max);
        }
    }
    auto combine_threads_end = std::chrono::steady_clock::now();

    auto sort_print_start = std::chrono::steady_clock::now();
    // Convert the unordered map to a vector for sorting
    std::vector<std::pair<std::string, cityInfo>> sortedCityInfos(combinedCityInfo.begin(), combinedCityInfo.end());

    // Sort the cityInfos by city name
    std::sort(sortedCityInfos.begin(), sortedCityInfos.end(), compareCityInfos);

    // Output sorted cityInfos
    for (const auto& pair : sortedCityInfos) {
        const cityInfo& res = pair.second;
        double mean = res.sum / static_cast<double>(res.count);
        std::cout << "City: " << pair.first << ", Min: " << res.min
                  << ", Mean: " << mean << ", Max: " << res.max << std::endl;
    }
    auto sort_print_end = std::chrono::steady_clock::now();
    auto total_time_end = std::chrono::steady_clock::now();

    // Calculate and output the elapsed time
    std::chrono::duration<double> total_time = total_time_end - total_time_start;
    std::chrono::duration<double> sort_print_time = sort_print_end - sort_print_start;
    std::chrono::duration<double> combine_threads_time =  combine_threads_end -  combine_threads_start;
    std::chrono::duration<double> threads_time =  threads_end -  threads_start;
    std::chrono::duration<double> map_file_time =  map_file_end -  map_file_start;

    std::cout << "Sorting and printing time: " << sort_print_time.count() << " seconds" << std::endl;
    std::cout << "Combining threads time: " << combine_threads_time.count() << " seconds" << std::endl;
    std::cout << "Threads time: " << threads_time.count() << " seconds" << std::endl;
    std::cout << "Mapping file time: " << map_file_time.count() << " seconds" << std::endl;
    std::cout << "Total execution time: " << total_time.count() << " seconds" << std::endl;

    return 0;
}