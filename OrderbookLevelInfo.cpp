#include "OrderbookLevelInfo.h" 

OrderbookLevelInfo::OrderbookLevelInfo(const LevelInfos& bids,const LevelInfos& asks):
    bids_{bids},
    asks_{asks}
{}
const LevelInfos& OrderbookLevelInfo::getBids() const{return bids_;}   
const LevelInfos& OrderbookLevelInfo::getAsks() const{return asks_;}