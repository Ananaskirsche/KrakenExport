#include <chrono>
#include <string>
#include <vector>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <ctime>

#include "external/rapidjson/document.h"
#include "external/rapidjson/rapidjson.h"

#include "Reward.hpp"

extern "C"
{
    #include "external/libkraken/kraken_api.h"
}


/**
 * Prüft, ob die kraken.key und currencies.txt existieren
 * @return
 */
bool checkConfig(){
    if(!std::filesystem::exists("kraken.key"))
    {
        printf("kraken.key does not exist!\n");
        return false;
    }

    if(!std::filesystem::exists("currencies.txt"))
    {
        printf("currencies.txt does not exist!\n");
        return false;
    }

    return true;
}

/**
 * Schreibt alle Währungen aus der currencies.txt in eine forward_list
 * @return
 */
std::vector<std::string> getCurrencies()
{
    std::vector<std::string> currenciesList;
    std::ifstream ifs("currencies.txt", std::ios::in);

    if (!ifs.is_open())
    {
        std::cout << "Could not open currencies.txt" << std::endl;
        return currenciesList;
    }

    std::string line;

    while (std::getline(ifs, line))
        currenciesList.push_back(line);

    ifs.close();

    return currenciesList;
}

/**
 * Holt den API-Key aus der kraken.key Datei
 * @return API-Key
 */
std::string getKrakenApiKey()
{
    std::ifstream ifs("kraken.key", std::ios::in);
    std::string key;
    std::getline(ifs, key);
    ifs.close();
    return key;
}

/**
 * Holt den Secret-Key aus der kraken.key Datei
 * @return Secret Key
 */
std::string getKrakenSecretKey()
{
    std::ifstream ifs("kraken.key", std::ios::in);
    std::string key;
    std::getline(ifs, key);
    std::getline(ifs, key);
    ifs.close();
    return key;
}

std::filesystem::path createFileNameOutOfCurrency(std::string const& currency)
{
    std::string filename = "staking_rewards_";
    filename.append(currency);
    filename.append(".csv");
    return filename;
}

/**
 * Erstellt die CSV-Datei welche die Transaktionen enthalten soll
 * @return Fehlercode
 */
int createCSVFile(std::filesystem::path currencyPath)
{
    std::ofstream ofs(currencyPath, std::ios::out);

    if (!ofs.is_open())
    {
        std::cout << "Could not create file " << currencyPath.string() << std::endl;
        return -1;
    }

    ofs << "LedgerID; Timestamp; Asset; Amount; Balance" << '\n';
    ofs.close();
    return 0;
}

std::string getLastLedgerIDFromRecords(std::string const& currency)
{
    std::filesystem::path filename = createFileNameOutOfCurrency(currency);

    if(!exists(filename))
    {
        createCSVFile(filename);
        return {};
    }

    std::ifstream ifs(filename, std::ios::in);

    if (!ifs.is_open())
    {
        std::cout << "Could not open " << filename.string() << std::endl;
        return {};
    }

    std::string lastLedger;
    std::string ledgerId, timestamp, asset, amount, balance;

    while (ifs >> ledgerId >> timestamp >> asset >> amount >> balance)
        lastLedger = ledgerId;

    ifs.close();

    lastLedger.substr(0, lastLedger.size() - 1);
    return lastLedger;
}


/**
 * Holt die Rewards mittels Kraken API
 * @param lastLedgerID Die ID die als letztes erfasst worden ist
 * @param currency Die Währung für die neue Einträge geholt werden sollen
 * @param api_key Kraken API Key
 * @param secret_key Kraken Secret Key
 * @return Liste mit neuen Rewards
 */
std::vector<Reward> getRewards(std::string const& lastLedgerID, std::string const& currency, std::string const& api_key, std::string const& secret_key)
{
    struct kraken_api *kr_api = nullptr;

    kraken_init(&kr_api, api_key.data(), secret_key.data());

    //Parameter setzen
    kraken_set_opt(&kr_api, "asset", currency.data());
    kraken_set_opt(&kr_api, "type", "staking");

    if(lastLedgerID != "")
    {
        kraken_set_opt(&kr_api, "start", lastLedgerID.data());
    }

    //Abfrage an KrakenAPI stellen
    kr_api->priv_func->get_ledgers_info(&kr_api);

    //Ergebnis überprüfen
    if(kr_api->s_result == nullptr){
        std::cout << "Error when calling Kraken API" << std::endl;
        return std::vector<Reward>();
    }

    //Get Krakenresult and clean krakenapi
    std::string json = kr_api->s_result;
    free(kr_api->s_result);
    kr_api->s_result = nullptr;
    kraken_clean(&kr_api);

    rapidjson::Document doc;
    doc.Parse(json.data());

    //JSON überprüfen
    if (!doc.HasMember("result"))
    {
        std::cout  << "JSON has no member 'result'!" << std::endl;
        return std::vector<Reward>();
    }
    if (!doc.HasMember("error"))
    {
        std::cout << "JSON has no member 'error'!" << std::endl;
        return std::vector<Reward>();
    }
    if(!doc["error"].GetArray().Empty())
    {
        std::cout << "An error occured! " << '\n' << "JSON: " << json << std::endl;
        return std::vector<Reward>();
    }

    //JSON verarbeiten
    rapidjson::GenericObject result = doc["result"].GetObject();
    rapidjson::GenericObject ledger = result["ledger"].GetObject();

    std::vector<Reward> rewardList;

    for(auto const& entry : ledger){
        rapidjson::GenericObject body = entry.value.GetObject();

        std::string ledgerId = entry.name.GetString();

        Reward reward;
        reward.ledgerId = ledgerId;
        reward.asset = body["asset"].GetString();
        reward.amount = body["amount"].GetString();
        reward.balance = body["balance"].GetString();

        std::string timeString;
        const time_t time = body["time"].GetDouble();
        tm* tmo = gmtime(&time);
        strftime(timeString.data(), 20*sizeof(char), "%Y-%m-%dT%H:%M:%S", tmo);
        reward.time = timeString;

        rewardList.push_back(reward);
    }

    return rewardList;
}


int writeRewardsToFile(std::vector<Reward> const& rewardList, std::string const& currency)
{
    std::filesystem::path filename = createFileNameOutOfCurrency(currency);

    std::ofstream ofs(filename, std::ios::out);

    if (!ofs.is_open())
    {
        std::cout << "Could not open " << filename.string() << std::endl;
        return -1;
    }

    for(auto const& reward : rewardList)
        ofs << reward.ledgerId << ";" << reward.time << ";" << reward.asset << ";" << reward.amount << ";" << reward.balance << '\n';

    ofs.close();
    return 0;
}



int main()
{
    if(!checkConfig())
        return -1;

    //Get Resources
    std::string apiKey = getKrakenApiKey();
    std::string secretKey = getKrakenSecretKey();
    std::vector<std::string> currencyList = getCurrencies();

    for(std::string const& currency : currencyList)
    {
        std::string lastLedgerID = getLastLedgerIDFromRecords(currency);
        std::vector<Reward> newRewards = getRewards(lastLedgerID, currency, apiKey, secretKey);
        writeRewardsToFile(newRewards, currency);

        std::cout << "Got " << newRewards.size() << " new " << currency << " rewards!" << std::endl;
    }

    return 0;
}
