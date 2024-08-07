/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU GPL v2 license, you may redistribute it
 * and/or modify it under version 2 of the License, or (at your option), any later version.
 */

#ifndef _PLAYERBOT_GEARSCORECALCULATOR_H
#define _PLAYERBOT_GEARSCORECALCULATOR_H


#include "Player.h"
#include "StatsCollector.h"

#define ITEM_SUBCLASS_MASK_SINGLE_HAND                                                                        \
    ((1 << ITEM_SUBCLASS_WEAPON_AXE) | (1 << ITEM_SUBCLASS_WEAPON_MACE) | (1 << ITEM_SUBCLASS_WEAPON_SWORD) | \
     (1 << ITEM_SUBCLASS_WEAPON_DAGGER) | (1 << ITEM_SUBCLASS_WEAPON_FIST))
     
class StatsWeightCalculator
{

public:
    StatsWeightCalculator(Player* player);
    float CalculateItem(uint32 itemId);

private:
    void GenerateWeights(Player* player);
    void GenerateBasicWeights(Player* player);
    void GenerateAdditionalWeights(Player* player);

    void CalculateItemTypePenalty(uint32 itemId);
    bool NotBestArmorType(uint32 item_subclass_armor);

private:
    Player* player_;
    std::unique_ptr<StatsCollector> collector_;
    uint8 cls;
    int tab;
    bool hit_and_expertise_overflow_;
    float weight_;
    float stats_weights_[STATS_TYPE_MAX];
};


#endif
