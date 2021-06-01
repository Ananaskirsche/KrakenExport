#ifndef KRAKENEXPORT_REWARD_H
#define KRAKENEXPORT_REWARD_H

struct Reward {
    std::string ledgerId{};
    std::string time{};
    std::string asset{};
    std::string amount{};
    std::string balance{};
};

#endif
