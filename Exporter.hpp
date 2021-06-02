#ifndef KRAKENEXPORT_EXPORTER_H
#define KRAKENEXPORT_EXPORTER_H
#include <string>
#include <forward_list>
#include "Reward.hpp"
class Exporter {
public:
    virtual void init() = 0;
    virtual std::string getLastLedgerID(const std::string &currency) = 0;
    virtual void exportRewards(const std::string &currency, const std::forward_list<Reward> &newRewards) = 0;
    virtual ~Exporter() { };
};
#endif