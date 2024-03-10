#include <iostream>
#include <fstream> // Added for file input
#include <string>
#include <unordered_map>
#include <vector>
#include <algorithm>
#include <thread>
#include "CircularBuffer.h"


struct cityInfo {
    int count;
    double sum, min, max;
};

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

// Function for the worker thread to parse data and enqueue it into the buffer
void workerThread(CircularBuffer<std::pair<std::string, double>>& buffer, std::string filename) {
    // Open the file
    std::ifstream file(filename);

    // Check if the file is opened successfully
    if (!file.is_open()) {
        std::cerr << "Error opening the file." << std::endl;
        return; // Exit the function if file opening fails
    }

    // Read and enqueue each line from the file into the circular buffer
    std::string line;
    while (std::getline(file, line)) {
        // Find the position of the first ';' character
        size_t delimiterPos = line.find(';');
        if (delimiterPos == std::string::npos)
            continue; // Not a valid line format

        // Extract the city name and temperature
        std::string cityName = line.substr(0, delimiterPos);
        double temperature = std::stod(line.substr(delimiterPos + 1));

        // Enqueue city data into the circular buffer
        buffer.enqueue({cityName, temperature});
    }

    // Close the file
    file.close();
}

// Function for the processing thread to dequeue data from the buffer and process it
void processThread(CircularBuffer<std::pair<std::string, double>>& buffer, std::unordered_map<std::string, cityInfo>& cityInfos) {
    // Process data from the circular buffer
    while (true) {
        auto data = buffer.dequeue();
        if (data.first.empty()) // Signal to exit
            break;
        updateCityInfos(data.first, data.second, cityInfos);
    }
}

// Comparator function to sort cityInfos by city name
bool compareCityInfos(const std::pair<std::string, cityInfo>& a, const std::pair<std::string, cityInfo>& b) {
    return a.first < b.first;
}

int main(int argc, char* argv[]) {
    // Create a map (unordered)
    std::unordered_map<std::string, cityInfo> cityInfos;

    // Check if the filename is provided as an argument
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <filename>" << std::endl;
        return 1;
    }

    // Create a circular buffer to store city data
    CircularBuffer<std::pair<std::string, double>> buffer(512); // Adjust size as needed

    // Start the worker thread
    std::thread worker(workerThread, std::ref(buffer), argv[1]);

    // Start the process thread
    std::thread processor(processThread, std::ref(buffer), std::ref(cityInfos));

    // Wait for the worker thread to finish
    worker.join();

    // Signal process thread to exit
    buffer.enqueue({ "", 0 });

    // Wait for the process thread to finish
    processor.join();

    // Convert the unordered map to a vector for sorting
    std::vector<std::pair<std::string, cityInfo>> sortedcityInfos(cityInfos.begin(), cityInfos.end());
    
    // Sort the cityInfos by city name
    std::sort(sortedcityInfos.begin(), sortedcityInfos.end(), compareCityInfos);

    // Output sorted cityInfos
    for (const auto& pair : sortedcityInfos) {
        const cityInfo& res = pair.second;
        double mean = res.sum / static_cast<double>(res.count);
        std::cout << "City: " << pair.first << ", Min: " << res.min
                  << ", Mean: " << mean << ", Max: " << res.max << std::endl;
    }

    return 0;
}
