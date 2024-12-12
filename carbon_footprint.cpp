#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <cstdlib>
#include <iomanip>
#include <cmath>
#include <curl/curl.h>

struct EmissionData {
    std::string year;
    double emissions;
};

std::string dataset_url = "https://raw.githubusercontent.com/owid/co2-data/refs/heads/master/owid-co2-data.csv";
const std::string dataset_file = "owid-co2-data.csv";
const std::string report_file = "report.txt";

size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    std::ofstream* outFile = static_cast<std::ofstream*>(userp);
    outFile->write(static_cast<char*>(contents), size * nmemb);
    return size * nmemb;
}

bool downloadDataset(const std::string& url, const std::string& filename) {
    CURL* curl;
    CURLcode res;
    std::ofstream outFile(filename, std::ios::binary);

    if (!outFile.is_open()) {
        std::cerr << "Failed to open file for writing: " << filename << "\n";
        return false;
    }

    curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &outFile);
        res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);

        if (res != CURLE_OK) {
            std::cerr << "cURL error: " << curl_easy_strerror(res) << "\n";
            return false;
        }
    } else {
        std::cerr << "Failed to initialize cURL.\n";
        return false;
    }

    return true;
}

void parseCSV(const std::string& filename, std::map<std::string, std::vector<EmissionData>>& data) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: Unable to open file: " << filename << "\n";
        return;
    }

    std::string line, header;
    getline(file, header); // Skip the header line

    while (getline(file, line)) {
        std::istringstream ss(line);
        std::string country, code, year, co2;

        getline(ss, country, ',');
        getline(ss, code, ',');
        getline(ss, year, ',');

        for (int i = 0; i < 3; ++i) // Skip to CO2 column
            getline(ss, co2, ',');

        if (!country.empty() && !year.empty() && !co2.empty()) {
            try {
                double emissions = std::stod(co2);
                data[country].push_back({year, emissions});
            } catch (const std::invalid_argument& e) {
                continue;
            }
        }
    }

    file.close();
}

void displayCountries(const std::set<std::string>& countries) {
    std::cout << "\nAvailable Countries:\n";
    for (const auto& country : countries) {
        std::cout << "- " << country << "\n";
    }
    std::cout << "\n";
}

void generateReport(const std::string& country, const std::vector<EmissionData>& emissions, double average, double stddev, double max_emission, const std::string& max_year, double min_emission, const std::string& min_year) {
    std::ofstream report(report_file);
    if (!report.is_open()) {
        std::cerr << "Error: Unable to create report file.\n";
        return;
    }

    report << "CO2 Emissions Report for " << country << "\n";
    report << std::string(40, '-') << "\n";
    report << std::setw(10) << "Year" << std::setw(15) << "Emissions (Mt)" << "\n";

    for (const auto& record : emissions) {
        report << std::setw(10) << record.year << std::setw(15) << record.emissions << "\n";
    }

    report << "\nStatistics for " << country << ":\n";
    report << "- Average Emissions: " << average << " Mt\n";
    report << "- Standard Deviation: " << stddev << " Mt\n";
    report << "- Highest Emissions: " << max_emission << " Mt in " << max_year << "\n";
    report << "- Lowest Emissions: " << min_emission << " Mt in " << min_year << "\n";

    report.close();
    std::cout << "Report saved to " << report_file << "\n";
}

void analyzeCountry(const std::string& country, const std::map<std::string, std::vector<EmissionData>>& data) {
    auto it = data.find(country);
    if (it == data.end()) {
        std::cerr << "Error: Country not found in dataset.\n";
        return;
    }

    const auto& emissions = it->second;
    double total = 0, max_emission = -1, min_emission = 1e9;
    std::string max_year, min_year;

    std::cout << "\nCO2 Emissions for " << country << ":\n";
    std::cout << std::setw(10) << "Year" << std::setw(15) << "Emissions (Mt)" << "\n";
    for (const auto& record : emissions) {
        std::cout << std::setw(10) << record.year << std::setw(15) << record.emissions << "\n";
        total += record.emissions;
        if (record.emissions > max_emission) {
            max_emission = record.emissions;
            max_year = record.year;
        }
        if (record.emissions < min_emission) {
            min_emission = record.emissions;
            min_year = record.year;
        }
    }

    double average = total / emissions.size();
    double variance = 0;
    for (const auto& record : emissions) {
        variance += std::pow(record.emissions - average, 2);
    }
    double stddev = std::sqrt(variance / emissions.size());

    std::cout << "\nStatistics for " << country << ":\n";
    std::cout << "- Average Emissions: " << average << " Mt\n";
    std::cout << "- Standard Deviation: " << stddev << " Mt\n";
    std::cout << "- Highest Emissions: " << max_emission << " Mt in " << max_year << "\n";
    std::cout << "- Lowest Emissions: " << min_emission << " Mt in " << min_year << "\n";

    generateReport(country, emissions, average, stddev, max_emission, max_year, min_emission, min_year);
}

int main() {
    std::cout << "Welcome to the Carbon Footprint Monitoring Tool!\n\n";

    if (!downloadDataset(dataset_url, dataset_file)) {
        std::cerr << "Error: Failed to download dataset.\n";
        return 1;
    }

    std::map<std::string, std::vector<EmissionData>> data;
    parseCSV(dataset_file, data);

    if (data.empty()) {
        std::cerr << "Error: No data available after parsing.\n";
        return 1;
    }

    std::set<std::string> countries;
    for (const auto& entry : data) {
        countries.insert(entry.first);
    }

    displayCountries(countries);

    std::string country;
    std::cout << "Enter the name of the country you want to analyze: ";
    getline(std::cin, country);

    analyzeCountry(country, data);

    std::cout << "\nThank you for using the Carbon Footprint Monitoring Tool!\n";
    return 0;
}
