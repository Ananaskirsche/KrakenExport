#include "include/rapidjson/document.h"
#include "include/rapidjson/rapidjson.h"
#include <chrono>
#include <filesystem>
#include <forward_list>
#include "Reward.h"
extern "C" {
    #include "include/libkraken/kraken_api.h"
    #include <fcntl.h>
    #include <cstdio>
    #include <ctime>
}

using namespace rapidjson;
using namespace std;

/**
 * Holt den API-Key aus der kraken.key Datei
 * @return API-Key
 */
char* getKrakenApiKey(){
    FILE *fd = fopen("kraken.key", "r");
    if(fd == nullptr){
        printf("Could not open kraken.key!\n");
        return nullptr;
    }

    char* api_key = (char*)malloc(58 * sizeof(char));

    int i = 0;
    char c;
    while(true){
        c = (char)fgetc(fd);
        if(c == '\n' || c == EOF){
            api_key[i] = '\0';
            break;
        }
        api_key[i] = c;
        i++;
    }

    fclose(fd);

    return api_key;
}



/**
 * Holt den Secret-Key aus der kraken.key Datei
 * @return Secret Key
 */
char* getKrakenSecretKey(){
    FILE *fd = fopen("kraken.key", "r");
    if(fd == nullptr){
        printf("Could not open kraken.key!\n");
        return nullptr;
    }

    //Erste Zeile lesen
    char c = 'a';
    while(c != '\n'){
        c = (char) fgetc(fd);
    }

    //Zweite Zeile lesen und in secret_key schreiben
    char* secret_key = (char*)malloc(90 * sizeof(char));
    int i = 0;
    while(true){
        c = (char) fgetc(fd);
        if(c == '\n' || c == EOF){
            secret_key[i] = '\0';
            break;
        }
        secret_key[i] = c;
        i++;
    }

    fclose(fd);
    return secret_key;
}



/**
 * Erstellt die CSV-Datei welche die Transaktionen enthalten soll
 * @return Fehlercode
 */
int createCSVFile(const char* currency){
    char filename[30] = "staking_rewards_";
    strcat(filename, currency);
    strcat(filename, ".csv");

    mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
    int file = creat(filename, mode);

    if(file == -1){
        printf("Could not create file %s!", filename);
        return -1;
    }

    //Header schreiben
    FILE *fd = fopen(filename, "w");
    if(fd == nullptr){
        printf("Could not open file %s\n",filename);
        return -1;
    }
    fprintf(fd, "LedgerID; Timestamp; Asset; Amount; Balance\n");
    fclose(fd);

    return 0;
}


/**
 * Gibt den letzten eingetragenen Reward aus der CSV-Datei zurück
 * @return NULL wenn noch kein Eintrag vorhanden ist, oder das Datum des letzten Eintrages
 */
char* getLastLedgerIDFromRecords(const char* currency){
    char filename[30] = "staking_rewards_";
    strcat(filename, currency);
    strcat(filename, ".csv");

    if(!std::filesystem::exists(filename)){
        createCSVFile(currency);
        return nullptr;
    }

    FILE *fd = fopen(filename, "r");
    if(fd == nullptr){
        printf("Could not open %s", filename);
        return nullptr;
    }

    int offset = 2;
    char lastChar = 'a';

    //Schrittweise den offset erhöhen um vom Dateiende zum ersten \n zu gehen
    while(lastChar != EOF){
        fseek(fd, -offset, 2);
        lastChar = (char)fgetc(fd);
        if(lastChar == '\n'){
            break;
        }
        offset++;
    }

    //Nachdem der Offset gefunden worden ist, können wir bis zum ersten Semikolon die letzte LedgerID einlesen
    char* lastLedger = (char*)malloc(20 * sizeof(char));
    int i = 0;
    char c;
    while(true){
        c = (char)fgetc(fd);
        if(c == EOF || c == '\n' || c == ';') {
            lastLedger[i] = '\0';
            break;
        }
        lastLedger[i] = c;
        i++;
    }

    fclose(fd);

    return lastLedger;
}


/**
 * Holt die Rewards mittels Kraken API
 * @param lastLedgerID Die ID die als letztes erfasst worden ist
 * @return Liste mit neuen Rewards
 */
std::forward_list<Reward*> getRewards(char* lastLedgerID, const char* currency){
    struct kraken_api *kr_api = nullptr;
    const char* api_key = getKrakenApiKey();
    const char* secret_key = getKrakenSecretKey();

    kraken_init(&kr_api, api_key, secret_key);

    //Parameter setzen
    kraken_set_opt(&kr_api, "asset", currency);
    kraken_set_opt(&kr_api, "type", "staking");
    if(lastLedgerID != nullptr){
        kraken_set_opt(&kr_api, "start", lastLedgerID);
    }

    //Abfrage an KrakenAPI stellen
    kr_api->priv_func->get_ledgers_info(&kr_api);

    //Ergebnis überprüfen
    if(kr_api->s_result == nullptr){
        printf("Error when calling Kraken API\n");
        return std::forward_list<Reward*>();
    }

    Document doc;
    char* json = kr_api->s_result;
    doc.Parse(json);

    //JSON überprüfen
    if (!doc.HasMember("result")){
        printf("JSON has no member 'result'!\n");
        return std::forward_list<Reward*>();
    }
    if (!doc.HasMember("error")){
        printf("JSON has no member 'error'!\n");
        return std::forward_list<Reward*>();
    }
    if(!doc["error"].GetArray().Empty()){
        printf("An error occured!\n JSON: %s", json);
        return std::forward_list<Reward*>();
    }

    //JSON verarbeiten
    GenericObject result = doc["result"].GetObject();
    GenericObject ledger = result["ledger"].GetObject();

    forward_list<Reward*> rewardList;

    for(auto& entry : ledger){
        GenericObject body = entry.value.GetObject();

        char* ledgerId = (char*)malloc(19*sizeof(char));
        strcpy(ledgerId, entry.name.GetString());

        Reward *reward = new Reward();
        reward->ledgerId = ledgerId;
        reward->asset = body["asset"].GetString();
        reward->amount = body["amount"].GetString();
        reward->balance = body["balance"].GetString();

        char* timeString = (char*)malloc(20 * sizeof(char));
        const time_t time = body["time"].GetDouble();
        tm* tmo = gmtime(&time);
        strftime(timeString, 20*sizeof(char), "%Y-%m-%dT%H:%M:%S", tmo);
        reward->time = timeString;

        rewardList.push_front(reward);
    }

    //KrakenAPI deinitialisieren & Variablen freen
    free(kr_api->s_result);
    kr_api->s_result = nullptr;
    kraken_clean(&kr_api);
    free((void*)secret_key);
    free((void*)api_key);

    return rewardList;
}


int writeRewardsToFile(const forward_list<Reward*>& rewardList, const char* currency){
    char filename[30] = "staking_rewards_";
    strcat(filename, currency);
    strcat(filename, ".csv");

    FILE *fd = fopen(filename, "a");
    if(fd == nullptr){
        return -1;
    }

    for(Reward *reward : rewardList){
        fprintf(fd, "%s;%s;%s;%s;%s\n", reward->ledgerId, reward->time, reward->asset, reward->amount, reward->balance);
    }

    fclose(fd);
    return 0;
}



int main() {
    char* lastDotLedgerID = getLastLedgerIDFromRecords("DOT.S");
    std::forward_list<Reward*> newDotRewards = getRewards(lastDotLedgerID, "DOT.S");
    writeRewardsToFile(newDotRewards, "DOT.S");

    char* lastAtomLedgerID = getLastLedgerIDFromRecords("ATOM.S");
    std::forward_list<Reward*> newAtomRewards = getRewards(lastAtomLedgerID, "ATOM.S");
    writeRewardsToFile(newAtomRewards, "ATOM.S");

    printf("Got %ld new DOT.S rewards!\n", std::distance(newDotRewards.begin(), newDotRewards.end()));
    printf("Got %ld new ATOM.S rewards!\n", std::distance(newAtomRewards.begin(), newAtomRewards.end()));

    for(Reward* reward : newDotRewards){
        delete reward;
    }
    for(Reward* reward : newAtomRewards){
        delete reward;
    }

    free(lastDotLedgerID);
    free(lastAtomLedgerID);

    return 0;
}
