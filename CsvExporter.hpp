#ifndef KRAKENEXPORT_CSVEXPORTER_HPP
#define KRAKENEXPORT_CSVEXPORTER_HPP
#include "Exporter.hpp"

class CsvExporter : public Exporter{
public:
    void init() override {};    //Brauchen wir für diesen Exporter nicht
    std::string getLastLedgerID(std::string const& currency) override;
    void exportRewards(std::string const& currency, std::forward_list<Reward> const& rewardList) override;

private:
    static std::string createFilenameFromCurrency(const std::string &currency);
    static void createCsvFile(const std::filesystem::path &currencyPath);
};


#endif //KRAKENEXPORT_CSVEXPORTER_HPP
