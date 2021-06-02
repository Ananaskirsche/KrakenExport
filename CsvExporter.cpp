#include <iostream>
#include <filesystem>
#include <fstream>
#include "CsvExporter.hpp"


/**
 *
 * @param currency
 * @param newRewards
 */
void CsvExporter::exportRewards(const std::string &currency, const std::forward_list<Reward> &rewardList) {
    std::filesystem::path filename = createFilenameFromCurrency(currency);

    std::ofstream ofs(filename, std::ios::app);

    if (!ofs.is_open())
    {
        std::cout << "Could not open " << filename.string() << std::endl;
        return;
    }

    for(auto const& reward : rewardList){
        ofs << reward.ledgerId << ";" << reward.time << ";" << reward.asset << ";" << reward.amount << ";" << reward.balance << '\n';
    }

    ofs.close();
}



/**
 * Holt die zuletzt geschriebene LedgerID aus der CSV-Datei. Wenn die Datei noch nicht existiert, wird sie on-the-fly erstellt
 * @param currency Die Währung für die wir die LedgerID brauchen
 * @return LedgerID oder "" falls es keine letzte LedgerID gibt
 */
std::string CsvExporter::getLastLedgerID(const std::string &currency) {
    std::filesystem::path filename = createFilenameFromCurrency(currency);

    if(!exists(filename))
    {
        createCsvFile(filename);
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

    while (ifs >> ledgerId >> timestamp >> asset >> amount >> balance){
        lastLedger = ledgerId;
    }

    ifs.close();

    lastLedger.substr(0, lastLedger.size() - 1);
    return lastLedger;
}



/**
 * Erstellt den Dateinamen und erhält als Parameter die Währung für die die Datei erstellt werden soll
 * @param currency
 * @return
 */
std::string CsvExporter::createFilenameFromCurrency(const std::string &currency) {
    std::string filename = "staking_rewards_";
    filename.append(currency);
    filename.append(".csv");
    return filename;
}



/**
 * Erstellt die CSV-Datei und schreibt den CSV-Header
 * @param currencyPath
 */
void CsvExporter::createCsvFile(const std::filesystem::path &currencyPath) {
    std::ofstream ofs(currencyPath, std::ios::out);

    if (!ofs.is_open())
    {
        std::cout << "Could not create file " << currencyPath.string() << std::endl;
        return;
    }

    ofs << "LedgerID; Timestamp; Asset; Amount; Balance" << '\n';
    ofs.close();
}



