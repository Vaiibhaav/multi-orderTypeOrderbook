#pragma once

#include "TradeInfo.h"

class Trade {
public:
    // We only declare the functions here, no {} brackets
    Trade(const TradeInfo& bidTrade, const TradeInfo& askTrade);

    const TradeInfo& GetBidTrade() const;  
    const TradeInfo& GetAskTrade() const;  

private:
    TradeInfo bidTrade_;
    TradeInfo askTrade_;
};
using Trades = std::vector<Trade>;
