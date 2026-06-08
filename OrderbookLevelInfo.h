#include "LevelInfo.h"
#include "Constants.h"

class OrderbookLevelInfo{
public:
    OrderbookLevelInfo(const LevelInfos& bids,const LevelInfos& asks);
    const LevelInfos& getBids() const;   
    const LevelInfos& getAsks() const;   
private:
    const LevelInfos bids_;
    const LevelInfos asks_;
};