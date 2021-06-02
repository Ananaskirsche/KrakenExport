//
// Created by fatih on 02.06.21.
//
#include <filesystem>
#include <fstream>
#include <regex>
#include "Configuration.hpp"



/**
 * Konstruktur mit Dateipfad zur Config Datei
 * @param filePath
 */
Configuration::Configuration(const std::filesystem::path &filePath) {
    this->filePath = filePath;
    this->krakenApiKey = "";
    this->krakenSecretKey = "";

    if(!std::filesystem::exists(this->filePath)){
        throw std::invalid_argument("File " + this->filePath.generic_string() + " not found!");
    }

    std::ifstream ifs(filePath, std::ios::in);
    std::string buf;
    std::string::size_type found;
    while(std::getline(ifs, buf)){
        found = buf.find('=');
        if(found == std::string::npos){
            continue;
        }

        std::string key = buf.substr(0, found);
        std::string value = buf.substr(found+1, buf.size());

        if(key == "KRAKEN_API_KEY"){
            this->krakenApiKey = value;
        }
        else if(key == "KRAKEN_SECRET_KEY"){
            this->krakenSecretKey = value;
        }
        else if(key == "CURRENCIES"){

            //Use Regex to split Input String by ,
            std::regex re("[\\w.]+");
            std::sregex_token_iterator first{value.begin(), value.end(), re}, last;

            this->currencies = std::forward_list<std::string> {first, last};
        }
    }
    ifs.close();


    //Prüfen, ob jetzt alle Variablen belegt sind
    if (this->krakenApiKey.empty()){
        throw std::invalid_argument("Please provide a KRAKEN_API_KEY!");
    }
    if (this->krakenSecretKey.empty()){
        throw std::invalid_argument("Please provide a KRAKEN_SECRET_KEY!");
    }
    if(this->currencies.empty()){
       throw  std::invalid_argument("Please enter at least one currency to export!");
    }



}

std::string Configuration::getKrakenApiKey() {
    return this->krakenApiKey;
}

std::string Configuration::getKrakenSecretKey() {
    return this->krakenSecretKey;
}

std::forward_list<std::string> Configuration::getCurrencies() {
    return this->currencies;
}




