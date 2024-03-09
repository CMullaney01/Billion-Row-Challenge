#include <iostream>
#include <fstream>
#include <string>
#include <unordered_map>
#include <vector>
#include <algorithm>

struct cityInfo {
    int count;
    double sum, min, max;
};

// Function to update cityInfos based on temperature and city
void updatecityInfos(std::string cityName, double temperature, std::unordered_map<std::string, cityInfo>& cityInfos) {
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
bool comparecityInfos(const std::pair<std::string, cityInfo>& a, const std::pair<std::string, cityInfo>& b) {
    return a.first < b.first;
}


int main(int argc, char* argv[]) {
    // Create a map (ordered)
    std::unordered_map<std::string, cityInfo> cityInfos;
    // Check if the filename is provided as an argument
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <filename>" << std::endl;
        return 1;
    }

    // Open the file
    std::ifstream file(argv[1]);

    // Check if the file is opened successfully
    if (!file.is_open()) {
        std::cerr << "Error opening the file." << std::endl;
        return 1;
    }

    // Read and output each line from the file
    std::string line;
    while (std::getline(file, line)) {
        // Find the position of the first ';' character
        size_t delimiterPos = line.find(';');
        if (delimiterPos == std::string::npos)
            continue; // Not a valid line format

        // Extract the city name and temperature
        std::string cityName = line.substr(0, delimiterPos);
        double temperature = std::strtod(line.c_str() + delimiterPos + 1, nullptr);

        // Update cityInfos
        updatecityInfos(cityName, temperature, cityInfos);
    }

    // Close the file
    file.close();

    // Convert the unordered map to a vector for sorting
    std::vector<std::pair<std::string, cityInfo>> sortedcityInfos(cityInfos.begin(), cityInfos.end());
    
    // Sort the cityInfos by city name
    std::sort(sortedcityInfos.begin(), sortedcityInfos.end(), comparecityInfos);

    // Output sorted cityInfos
    for (const auto& pair : sortedcityInfos) {
        const cityInfo& res = pair.second;
        double mean = res.sum / static_cast<double>(res.count);
        std::cout << "City: " << pair.first << ", Min: " << res.min
                  << ", Mean: " << mean << ", Max: " << res.max << std::endl;
    }

    return 0;
}
