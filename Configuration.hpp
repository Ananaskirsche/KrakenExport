//
// Created by fatih on 02.06.21.
//

#ifndef KRAKENEXPORT_CONFIGURATION_HPP
#define KRAKENEXPORT_CONFIGURATION_HPP

#include <filesystem>
#include <forward_list>

class Configuration {
private:
    std::filesystem::path filePath;
    std::string krakenApiKey;
    std::string krakenSecretKey;
    std::forward_list<std::string> currencies;

public:
    explicit Configuration(const std::filesystem::path& filePath);
    std::string getKrakenApiKey();
    std::string getKrakenSecretKey();
    std::forward_list<std::string> getCurrencies();
};


#endif //KRAKENEXPORT_CONFIGURATION_HPP
