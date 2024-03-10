#include <iostream>
#include <fstream>
#include <string>
#include <unordered_map>
#include <vector>
#include <algorithm>
#include <thread>
#include "buffer.h"


struct cityInfo {
    int count;
    double sum, min, max;
};

// Global variable to indicate whether the worker thread has finished
bool workerFinished = false;

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
void workerThread(Buffer& buffer, std::string filename) {
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
        buffer.enqueue(std::make_tuple(std::move(cityName), temperature));
    }

    // Close the file
    file.close();

    // Signal that the worker thread has finished
    workerFinished = true;
    std::cout << "Worker thread finished." << std::endl;
}

// Function for the processing thread to dequeue data from the buffer and process it
void processThread(Buffer& buffer, std::unordered_map<std::string, cityInfo>& cityInfos) {
    // Process data from the circular buffer
    while (true) {
        auto data = buffer.dequeue();
        const std::string& city = std::get<0>(data);
        double temperature = std::get<1>(data);

        // Print received data
        // std::cout << "Received: City: " << city << ", Temperature: " << temperature << std::endl;

        // Break the loop if the buffer is empty (signaling to exit)
        if (buffer.isEmpty() && workerFinished)
            break;
            
        updateCityInfos(city, temperature, cityInfos);
    }
    std::cout << "Processor thread finished." << std::endl;
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
    Buffer buffer(131072); // Adjust size as needed

    std::cout << "Starting worker thread..." << std::endl;
    std::thread worker(workerThread, std::ref(buffer), argv[1]);

    std::cout << "Starting processor thread..." << std::endl;
    std::thread processor(processThread, std::ref(buffer), std::ref(cityInfos));

    // Wait for the worker thread to finish
    worker.join();

    // Wait for the process thread to finish
    processor.join();

    // print at the end
    // // Convert the unordered map to a vector for sorting
    // std::vector<std::pair<std::string, cityInfo>> sortedcityInfos(cityInfos.begin(), cityInfos.end());
    
    // // Sort the cityInfos by city name
    // std::sort(sortedcityInfos.begin(), sortedcityInfos.end(), compareCityInfos);

    // // Output sorted cityInfos
    // for (const auto& pair : sortedcityInfos) {
    //     const cityInfo& res = pair.second;
    //     double mean = res.sum / static_cast<double>(res.count);
    //     std::cout << "City: " << pair.first << ", Min: " << res.min
    //               << ", Mean: " << mean << ", Max: " << res.max << std::endl;
    // }

    // return 0;
}