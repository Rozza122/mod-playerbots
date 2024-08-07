/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU GPL v2 license, you may redistribute it
 * and/or modify it under version 2 of the License, or (at your option), any later version.
 */

#include "StatsWeightCalculator.h"
#include <memory>
#include "AiFactory.h"
#include "ObjectMgr.h"
#include "PlayerbotAI.h"
#include "SharedDefines.h"
#include "StatsCollector.h"

StatsWeightCalculator::StatsWeightCalculator(Player* player): player_(player)
{
    CollectorType type;
    if (PlayerbotAI::IsCaster(player))
        type = CollectorType::SPELL;
    else if (PlayerbotAI::IsMelee(player))
        type = CollectorType::MELEE;
    else
        type = CollectorType::RANGED;
    collector_ = std::make_unique<StatsCollector>(type);

    cls = player->getClass();
    tab = AiFactory::GetPlayerSpecTab(player);

    hit_and_expertise_overflow_ = false;

    weight_ = 0;
    for (uint32 i = 0; i < STATS_TYPE_MAX; i++)
    {
        stats_weights_[i] = 0;
    }
    GenerateWeights(player_);
}

float StatsWeightCalculator::CalculateItem(uint32 itemId)
{
    collector_->Reset();
    collector_->CollectItemStats(itemId);
    
    for (uint32 i = 0; i < STATS_TYPE_MAX; i++)
    {
        weight_ = stats_weights_[i] * collector_->stats[i];
    }

    CalculateItemTypePenalty(itemId);
    return weight_;
}

void StatsWeightCalculator::GenerateWeights(Player* player)
{
    GenerateBasicWeights(player);
    GenerateAdditionalWeights(player);
}

void StatsWeightCalculator::GenerateBasicWeights(Player* player)
{
    // Basic weights
    stats_weights_[STATS_TYPE_STAMINA] += 0.01f;
    stats_weights_[STATS_TYPE_ARMOR] += 0.001f;
    
    if (cls == CLASS_HUNTER)
    {
        stats_weights_[STATS_TYPE_AGILITY] += 2.5f;
        stats_weights_[STATS_TYPE_ATTACK_POWER] += 1.0f;
        stats_weights_[STATS_TYPE_ARMOR_PENETRATION] += 2.0f;
        stats_weights_[STATS_TYPE_HIT] += 2.0f;
        stats_weights_[STATS_TYPE_CRIT] += 2.0f;
        stats_weights_[STATS_TYPE_HASTE] += 2.0f;
        stats_weights_[STATS_TYPE_RANGED_DPS] += 5.0f;
    }
    else if (cls == CLASS_ROGUE || (cls == CLASS_DRUID && tab == DRUID_TAB_FERAL && !PlayerbotAI::IsTank(player)))
    {
        stats_weights_[STATS_TYPE_AGILITY] += 2.0f;
        stats_weights_[STATS_TYPE_STRENGTH] += 1.0f;
        stats_weights_[STATS_TYPE_ATTACK_POWER] += 1.0f;
        stats_weights_[STATS_TYPE_ARMOR_PENETRATION] += 1.0f;
        stats_weights_[STATS_TYPE_HIT] += 1.5f;
        stats_weights_[STATS_TYPE_CRIT] += 1.5f;
        stats_weights_[STATS_TYPE_HASTE] += 1.5f;
        stats_weights_[STATS_TYPE_EXPERTISE] += 2.5f;
        stats_weights_[STATS_TYPE_MELEE_DPS] += 5.0f;
    }
    else if ((cls == CLASS_PALADIN && tab == 2) ||    // retribution
             (cls == CLASS_WARRIOR && tab != 2) ||    // arm / fury
             (cls == CLASS_DEATH_KNIGHT && tab != 0)  // ice / unholy
    )
    {
        stats_weights_[STATS_TYPE_AGILITY] += 1.0f;
        stats_weights_[STATS_TYPE_STRENGTH] += 2.0f;
        stats_weights_[STATS_TYPE_ATTACK_POWER] += 1.0f;
        stats_weights_[STATS_TYPE_ARMOR_PENETRATION] += 1.0f;
        stats_weights_[STATS_TYPE_HIT] += 1.5f;
        stats_weights_[STATS_TYPE_CRIT] += 1.5f;
        stats_weights_[STATS_TYPE_HASTE] += 1.5f;
        stats_weights_[STATS_TYPE_EXPERTISE] += 2.0f;
        stats_weights_[STATS_TYPE_MELEE_DPS] += 5.0f;
    }
    else if ((cls == CLASS_SHAMAN && tab == 1)) // enhancement
    {  
        stats_weights_[STATS_TYPE_AGILITY] += 1.5f;
        stats_weights_[STATS_TYPE_STRENGTH] += 1.0f;
        stats_weights_[STATS_TYPE_INTELLECT] += 0.5f;
        stats_weights_[STATS_TYPE_ATTACK_POWER] += 1.0f;
        stats_weights_[STATS_TYPE_SPELL_POWER] += 1.5f;
        stats_weights_[STATS_TYPE_ARMOR_PENETRATION] += 1.5f;
        stats_weights_[STATS_TYPE_HIT] += 1.5f;
        stats_weights_[STATS_TYPE_CRIT] += 1.5f;
        stats_weights_[STATS_TYPE_HASTE] += 1.5f;
        stats_weights_[STATS_TYPE_EXPERTISE] += 2.0f;
        stats_weights_[STATS_TYPE_MELEE_DPS] += 5.0f;
    }
    else if (cls == CLASS_WARLOCK || cls == CLASS_MAGE || (cls == CLASS_PRIEST && tab == PRIEST_TAB_SHADOW) ||  // shadow
             (cls == CLASS_SHAMAN && tab == SHAMAN_TAB_ELEMENTAL) ||                                            // element
             (cls == CLASS_DRUID && tab == DRUID_TAB_BALANCE))                                                  // balance
    {
        stats_weights_[STATS_TYPE_INTELLECT] += 0.5f;
        stats_weights_[STATS_TYPE_SPIRIT] += 0.5f;
        stats_weights_[STATS_TYPE_SPELL_POWER] += 1.0f;
        stats_weights_[STATS_TYPE_SPELL_PENETRATION] += 1.0f;
        stats_weights_[STATS_TYPE_HIT] += 1.0f;
        stats_weights_[STATS_TYPE_CRIT] += 1.0f;
        stats_weights_[STATS_TYPE_HASTE] += 1.0f;
        stats_weights_[STATS_TYPE_RANGED_DPS] += 1.0f;
    }
    else if ((cls == CLASS_PALADIN && tab == PALADIN_TAB_HOLY) ||  // holy
             (cls == CLASS_PRIEST && tab != PRIEST_TAB_SHADOW) ||   // discipline / holy
             (cls == CLASS_SHAMAN && tab == SHAMAN_TAB_RESTORATION) ||   // heal
             (cls == CLASS_DRUID && tab == DRUID_TAB_RESTORATION))
    {
        stats_weights_[STATS_TYPE_INTELLECT] += 0.5f;
        stats_weights_[STATS_TYPE_SPIRIT] += 0.5f;
        stats_weights_[STATS_TYPE_SPELL_POWER] += 1.0f;
        stats_weights_[STATS_TYPE_MANA_REGENERATION] += 0.5f;
        stats_weights_[STATS_TYPE_CRIT] += 0.5f;
        stats_weights_[STATS_TYPE_HASTE] += 1.0f;
        stats_weights_[STATS_TYPE_RANGED_DPS] += 1.0f;
    }
    
    else if ((cls == CLASS_WARRIOR && tab == 2) || (cls == CLASS_PALADIN && tab == 1))
    {
        stats_weights_[STATS_TYPE_AGILITY] += 2.0f;
        stats_weights_[STATS_TYPE_STRENGTH] += 1.0f;
        stats_weights_[STATS_TYPE_STAMINA] += 3.0f;
        stats_weights_[STATS_TYPE_ATTACK_POWER] += 0.2f;
        stats_weights_[STATS_TYPE_DEFENSE] += 2.5f;
        stats_weights_[STATS_TYPE_PARRY] += 2.0f;
        stats_weights_[STATS_TYPE_DODGE] += 2.0f;
        stats_weights_[STATS_TYPE_RESILIENCE] += 2.0f;
        stats_weights_[STATS_TYPE_BLOCK] += 2.0f;
        stats_weights_[STATS_TYPE_ARMOR] += 0.3f;
        stats_weights_[STATS_TYPE_HIT] += 0.5f;
        stats_weights_[STATS_TYPE_CRIT] += 0.2f;
        stats_weights_[STATS_TYPE_HASTE] += 0.5f;
        stats_weights_[STATS_TYPE_EXPERTISE] += 3.0f;
        stats_weights_[STATS_TYPE_MELEE_DPS] += 2.0f;
    }
    else if (cls == CLASS_DEATH_KNIGHT && tab == 0)
    {
        stats_weights_[STATS_TYPE_AGILITY] += 2.0f;
        stats_weights_[STATS_TYPE_STRENGTH] += 1.0f;
        stats_weights_[STATS_TYPE_STAMINA] += 2.5f;
        stats_weights_[STATS_TYPE_ATTACK_POWER] += 0.2f;
        stats_weights_[STATS_TYPE_DEFENSE] += 3.5f;
        stats_weights_[STATS_TYPE_PARRY] += 2.0f;
        stats_weights_[STATS_TYPE_DODGE] += 2.0f;
        stats_weights_[STATS_TYPE_RESILIENCE] += 2.0f;
        stats_weights_[STATS_TYPE_ARMOR] += 0.3f;
        stats_weights_[STATS_TYPE_HIT] += 0.5f;
        stats_weights_[STATS_TYPE_CRIT] += 0.5f;
        stats_weights_[STATS_TYPE_HASTE] += 0.5f;
        stats_weights_[STATS_TYPE_EXPERTISE] += 3.5f;
        stats_weights_[STATS_TYPE_MELEE_DPS] += 2.0f;
    }
    else
    {
        // BEAR DRUID TANK
        stats_weights_[STATS_TYPE_AGILITY] += 1.5f;
        stats_weights_[STATS_TYPE_STRENGTH] += 1.0f;
        stats_weights_[STATS_TYPE_STAMINA] += 1.5f;
        stats_weights_[STATS_TYPE_ATTACK_POWER] += 0.2f;
        stats_weights_[STATS_TYPE_DEFENSE] += 2.0f;
        stats_weights_[STATS_TYPE_DODGE] += 2.0f;
        stats_weights_[STATS_TYPE_RESILIENCE] += 2.0f;
        stats_weights_[STATS_TYPE_ARMOR] += 0.3f;
        stats_weights_[STATS_TYPE_HIT] += 0.5f;
        stats_weights_[STATS_TYPE_CRIT] += 0.5f;
        stats_weights_[STATS_TYPE_HASTE] += 0.5f;
        stats_weights_[STATS_TYPE_EXPERTISE] += 3.0f;
        stats_weights_[STATS_TYPE_MELEE_DPS] += 2.0f;
    }
}

void StatsWeightCalculator::GenerateAdditionalWeights(Player* player)
{
    uint8 cls = player->getClass();
    // int tab = AiFactory::GetPlayerSpecTab(player);
    if (cls == CLASS_HUNTER)
    {
        if (player->HasAura(34484))
            stats_weights_[STATS_TYPE_INTELLECT] += 1.0f;
    } else if (cls == CLASS_WARRIOR)
    {
        if (player->HasAura(61222))
            stats_weights_[STATS_TYPE_ARMOR] += 0.03f;
    } else if (cls == CLASS_SHAMAN)
    {
        if (player->HasAura(51885))
            stats_weights_[STATS_TYPE_INTELLECT] += 1.0f;
    }
}

void StatsWeightCalculator::CalculateItemTypePenalty(uint32 itemId)
{
    ItemTemplate const* proto = &sObjectMgr->GetItemTemplateStore()->at(itemId);
    // penalty for different type armor
    if (proto->Class == ITEM_CLASS_ARMOR && proto->SubClass >= ITEM_SUBCLASS_ARMOR_CLOTH &&
        proto->SubClass <= ITEM_SUBCLASS_ARMOR_PLATE && NotBestArmorType(proto->SubClass))
    {
        weight_ *= 0.8;
    }
    // double hand
    if (proto->Class == ITEM_CLASS_WEAPON)
    {
        bool isDoubleHand = proto->Class == ITEM_CLASS_WEAPON &&
                            !(ITEM_SUBCLASS_MASK_SINGLE_HAND & (1 << proto->SubClass)) &&
                            !(ITEM_SUBCLASS_MASK_WEAPON_RANGED & (1 << proto->SubClass));

        if (isDoubleHand)
        {
            weight_ *= 0.5;
        }
        // spec without double hand
        // enhancement, rogue, ice dk, unholy dk, shield tank, fury warrior without titan's grip but with duel wield
        if (isDoubleHand &&
            ((cls == CLASS_SHAMAN && tab == SHAMAN_TAB_ENHANCEMENT && player_->CanDualWield()) || (cls == CLASS_ROGUE) ||
             (cls == CLASS_DEATH_KNIGHT && tab != DEATHKNIGT_TAB_BLOOD) ||
             (cls == CLASS_WARRIOR && tab == WARRIOR_TAB_FURY && !player_->CanTitanGrip() && player_->CanDualWield()) || 
             (cls == CLASS_WARRIOR && tab == WARRIOR_TAB_PROTECTION) || (cls == CLASS_PALADIN && tab == PALADIN_TAB_PROTECTION)))
        {
            weight_ *= 0.1;
        }
        // spec with double hand
        // fury without duel wield, arms, bear, retribution, blood dk
        if (isDoubleHand && ((cls == CLASS_WARRIOR && tab == WARRIOR_TAB_FURY && !player_->CanDualWield()) ||
                             (cls == CLASS_WARRIOR && tab == WARRIOR_TAB_ARMS) || (cls == CLASS_DRUID && tab == DRUID_TAB_FERAL) ||
                             (cls == CLASS_PALADIN && tab == PALADIN_TAB_RETRIBUTION) || (cls == CLASS_DEATH_KNIGHT && tab == DEATHKNIGT_TAB_BLOOD) ||
                             (cls == CLASS_SHAMAN && tab == SHAMAN_TAB_ENHANCEMENT && !player_->CanDualWield())))
        {
            weight_ *= 10;
        }
        // fury with titan's grip
        if (isDoubleHand && proto->SubClass != ITEM_SUBCLASS_WEAPON_POLEARM &&
            (cls == CLASS_WARRIOR && tab == WARRIOR_TAB_FURY && player_->CanTitanGrip()))
        {
            weight_ *= 10;
        }
    }
    if (proto->Class == ITEM_CLASS_WEAPON)
    {
        if (cls == CLASS_HUNTER && proto->SubClass == ITEM_SUBCLASS_WEAPON_THROWN)
        {
            weight_ *= 0.1;
        }
        if (cls == CLASS_ROGUE && tab == ROGUE_TAB_ASSASSINATION && proto->SubClass != ITEM_SUBCLASS_WEAPON_DAGGER)
        {
            weight_ *= 0.1;
        }
    }
}

bool StatsWeightCalculator::NotBestArmorType(uint32 item_subclass_armor)
{
    if (player_->HasSkill(SKILL_PLATE_MAIL))
    {
        return item_subclass_armor != ITEM_SUBCLASS_ARMOR_PLATE;
    }
    if (player_->HasSkill(SKILL_MAIL))
    {
        return item_subclass_armor != ITEM_SUBCLASS_ARMOR_MAIL;
    }
    if (player_->HasSkill(SKILL_LEATHER))
    {
        return item_subclass_armor != ITEM_SUBCLASS_ARMOR_LEATHER;
    }
    return false;
}