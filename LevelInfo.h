#pragma once
#include "Constants.h"

//levels -> wiil be used in some public api that we'll ues to get the state of the order book
struct LevelInfo{
    Price price_;
    Quantity quantity_;
};
using LevelInfos = std::vector<LevelInfo>;
