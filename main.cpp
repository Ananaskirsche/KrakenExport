#include <chrono>
#include <string>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <ctime>
#include <forward_list>

#include "external/rapidjson/document.h"
#include "external/rapidjson/rapidjson.h"

#include "Reward.hpp"
#include "Exporter.hpp"
#include "CsvExporter.hpp"
#include "Configuration.hpp"

extern "C"
{
    #include "external/libkraken/kraken_api.h"
}



/**
 * Holt die Rewards mittels Kraken API
 * @param lastLedgerID Die ID die als letztes erfasst worden ist
 * @param currency Die Währung für die neue Einträge geholt werden sollen
 * @param api_key Kraken API Key
 * @param secret_key Kraken Secret Key
 * @return Liste mit neuen Rewards
 */
std::forward_list<Reward> getRewards(std::string const& lastLedgerID, std::string const& currency, std::string const& api_key, std::string const& secret_key)
{
    struct kraken_api *kr_api = nullptr;

    kraken_init(&kr_api, api_key.data(), secret_key.data());

    //Parameter setzen
    kraken_set_opt(&kr_api, "asset", currency.data());
    kraken_set_opt(&kr_api, "type", "staking");

    if(!lastLedgerID.empty())
    {
        kraken_set_opt(&kr_api, "start", lastLedgerID.data());
    }

    //Abfrage an KrakenAPI stellen
    kr_api->priv_func->get_ledgers_info(&kr_api);

    //Ergebnis überprüfen
    if(kr_api->s_result == nullptr){
        std::cout << "Error when calling Kraken API" << std::endl;
        return std::forward_list<Reward>();
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
        // Wir brauchen das nicht ausgeben, da 'result' fehlt, wenn es keine neuen Stakes gibt
        //std::cout  << "JSON has no member 'result'!" << std::endl;
        return std::forward_list<Reward>();
    }
    if (!doc.HasMember("error"))
    {
        std::cout << "JSON has no member 'error'!" << std::endl;
        return std::forward_list<Reward>();
    }
    if(!doc["error"].GetArray().Empty())
    {
        std::cout << "An error occured! " << '\n' << "JSON: " << json << std::endl;
        return std::forward_list<Reward>();
    }

    //JSON verarbeiten
    rapidjson::GenericObject result = doc["result"].GetObject();
    rapidjson::GenericObject ledger = result["ledger"].GetObject();

    std::forward_list<Reward> rewardList;

    for(auto const& entry : ledger){
        rapidjson::GenericObject body = entry.value.GetObject();

        std::string ledgerId = entry.name.GetString();

        Reward reward;
        reward.ledgerId = ledgerId;
        reward.asset = body["asset"].GetString();
        reward.amount = body["amount"].GetString();
        reward.balance = body["balance"].GetString();

        std::string timeString;
        timeString.resize(20);
        const time_t time = body["time"].GetDouble();
        tm* tmo = gmtime(&time);
        strftime(timeString.data(), 20*sizeof(char), "%Y-%m-%dT%H:%M:%S", tmo);

        //Wir müssen den C-String-Terminator abhacken
        timeString.resize(19);
        reward.time = timeString;

        rewardList.push_front(reward);
    }

    return rewardList;
}



int main()
{
    try{
        Configuration c{"config.txt"};

        std::unique_ptr<Exporter> exp = std::make_unique<CsvExporter>();

        for(std::string const& currency : c.getCurrencies())
        {
            std::string lastLedgerID = exp->getLastLedgerID(currency);
            std::forward_list<Reward> newRewards = getRewards(lastLedgerID, currency, c.getKrakenApiKey(), c.getKrakenSecretKey());
            exp->exportRewards(currency, newRewards);

            std::cout << "Got " << std::distance(newRewards.begin(), newRewards.end()) << " new " << currency << " rewards!" << std::endl;
        }
    }
    catch(const std::invalid_argument& e){
        std::cerr << "ERROR: " << e.what() << std::endl;
    }

    return 0;
}
