#include "StatsCollector.h"
#include <cstdint>
#include "DBCStores.h"
#include "ItemTemplate.h"
#include "ObjectMgr.h"
#include "SpellInfo.h"
#include "SpellMgr.h"

StatsCollector::StatsCollector(CollectorType type): type_(type)
{
    Reset();
}

void StatsCollector::Reset()
{
    for (uint32 i = 0; i < STATS_TYPE_MAX; i++)
    {
        stats[i] = 0;
    }
}

void StatsCollector::CollectItemStats(uint32 itemId)
{
    ItemTemplateContainer const* itemTemplates = sObjectMgr->GetItemTemplateStore();
    ItemTemplate const* proto = &itemTemplates->at(itemId);
    if (proto->IsRangedWeapon())
    {
        uint32 val = (proto->Damage[0].DamageMin + proto->Damage[0].DamageMax) * 1000 / 2 / proto->Delay;
        stats[STATS_TYPE_RANGED_DPS] += val;
    }
    else if (proto->IsWeapon())
    {
        uint32 val = (proto->Damage[0].DamageMin + proto->Damage[0].DamageMax) * 1000 / 2 / proto->Delay;
        stats[STATS_TYPE_MELEE_DPS] += val;
    }
    stats[STATS_TYPE_ARMOR] += proto->Armor;
    stats[STATS_TYPE_BLOCK] += proto->Block;
    for (int i = 0; i < proto->StatsCount; i++)
    {
        const _ItemStat& stat = proto->ItemStat[i];
        const int32& val = stat.ItemStatValue;
        CollectByItemStatType(stat.ItemStatType, val);
    }
    for (uint8 j = 0; j < MAX_ITEM_PROTO_SPELLS; j++)
    {
        CollectSpellStats(proto->Spells[j].SpellId, proto->Spells[j].SpellTrigger);
    }
}

void StatsCollector::CollectSpellStats(uint32 spellId, uint32 trigger)
{
    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId);
    
    if (!spellInfo)
        return;
    
    if (CollectSpecialCaseSpellStats(spellId))
        return;

    for (int i = 0; i < MAX_SPELL_EFFECTS; i++)
    {
        float multiplier = trigger == ITEM_SPELLTRIGGER_ON_EQUIP ? 1.0f : 0.2f;
        CollectSpellEffectStats(spellInfo->Effects[i], multiplier);
    }
}

void StatsCollector::CollectSpellEffectStats(const SpellEffectInfo &effectInfo, float multiplier)
{
    if (effectInfo.Effect != SPELL_EFFECT_APPLY_AURA)
        return;

    int32 val = effectInfo.BasePoints + 1;
    switch (effectInfo.ApplyAuraName)
    {
        case SPELL_AURA_MOD_DAMAGE_DONE:
        // case SPELL_AURA_MOD_HEALING_DONE is duplicated
            stats[STATS_TYPE_SPELL_POWER] += val * multiplier;
            break;
        case SPELL_AURA_MOD_ATTACK_POWER:
            stats[STATS_TYPE_ATTACK_POWER] += val * multiplier;
            break;
        case SPELL_AURA_MOD_SHIELD_BLOCKVALUE:
            stats[STATS_TYPE_BLOCK] += val * multiplier;
            break;
        case SPELL_AURA_MOD_RATING:
        {
            for (uint32 rating = CR_WEAPON_SKILL; rating < MAX_COMBAT_RATING; ++rating)
            {
                if (effectInfo.MiscValue & (1 << rating))
                {
                    switch (rating)
                    {
                        case CR_DEFENSE_SKILL:
                            stats[STATS_TYPE_DEFENSE] += val * multiplier;
                            break;
                        case CR_DODGE:
                            stats[STATS_TYPE_DODGE] += val * multiplier;
                            break;
                        case CR_PARRY:
                            stats[STATS_TYPE_PARRY] += val * multiplier;
                            break;
                        case CR_BLOCK:
                            stats[STATS_TYPE_BLOCK] += val * multiplier;
                            break;
                        case CR_HIT_MELEE:
                            if (type_ == CollectorType::MELEE)
                                stats[STATS_TYPE_HIT] += val * multiplier;
                            break;
                        case CR_HIT_RANGED:
                            if (type_ == CollectorType::RANGED)
                                stats[STATS_TYPE_HIT] += val * multiplier;
                            break;
                        case CR_HIT_SPELL:
                            if (type_ == CollectorType::SPELL)
                                stats[STATS_TYPE_HIT] += val * multiplier;
                            break;
                        case CR_CRIT_MELEE:
                            if (type_ == CollectorType::MELEE)
                                stats[STATS_TYPE_CRIT] += val * multiplier;
                            break;
                        case CR_CRIT_RANGED:
                            if (type_ == CollectorType::RANGED)
                                stats[STATS_TYPE_CRIT] += val * multiplier;
                            break;
                        case CR_CRIT_SPELL:
                            if (type_ == CollectorType::SPELL)
                                stats[STATS_TYPE_CRIT] += val * multiplier;
                            break;
                        case CR_HASTE_MELEE:
                            if (type_ == CollectorType::MELEE)
                                stats[STATS_TYPE_HASTE] += val * multiplier;
                            break;
                        case CR_HASTE_RANGED:
                            if (type_ == CollectorType::RANGED)
                                stats[STATS_TYPE_HASTE] += val * multiplier;
                            break;
                        case CR_HASTE_SPELL:
                            if (type_ == CollectorType::SPELL)
                                stats[STATS_TYPE_HASTE] += val * multiplier;
                            break;
                        case CR_EXPERTISE:
                            stats[STATS_TYPE_EXPERTISE] += val * multiplier;
                            break;
                        case CR_ARMOR_PENETRATION:
                            stats[STATS_TYPE_ARMOR_PENETRATION] += val * multiplier;
                            break;
                        default:
                            break;
                    }
                }
                break;
            }
        }
        case SPELL_AURA_PROC_TRIGGER_SPELL:
        {
            multiplier = 0.2f;
            if (effectInfo.TriggerSpell)
            {
                SpellInfo const* triggerSpellInfo = sSpellMgr->GetSpellInfo(effectInfo.TriggerSpell);
                if (!triggerSpellInfo)
                    return;
                for (uint8 k = 0; k < MAX_SPELL_EFFECTS; k++)
                {
                    CollectSpellEffectStats(triggerSpellInfo->Effects[k], multiplier);
                }
            }
            break;
        }
        default:
            break;
    }
}

void StatsCollector::CollectEnchantStats(uint32 enchantId)
{
    SpellItemEnchantmentEntry const* enchant = sSpellItemEnchantmentStore.LookupEntry(enchantId);
    if (!enchant)
    {
        return;
    }
    for (int s = 0; s < MAX_SPELL_ITEM_ENCHANTMENT_EFFECTS; ++s)
    {
        uint32 enchant_display_type = enchant->type[s];
        uint32 enchant_amount = enchant->amount[s];
        uint32 enchant_spell_id = enchant->spellid[s];
        
        if (CollectSpecialEnchantSpellStats(enchant_spell_id))
            continue;
        
        switch (enchant_display_type)
        {
            case ITEM_ENCHANTMENT_TYPE_STAT:
            {
                if (!enchant_amount)
                {
                    break;
                }
                CollectByItemStatType(enchant_spell_id, enchant_amount);
                break;
            }
            default:
                break;
        }
    }
}

/// @todo Special case for trinket
bool StatsCollector::CollectSpecialCaseSpellStats(uint32 spellId)
{
    return false;
}


bool StatsCollector::CollectSpecialEnchantSpellStats(uint32 enchantSpellId)
{
    switch (enchantSpellId)
    {
        case 28093: // mongoose
            if (type_ == CollectorType::MELEE)
            {
                stats[STATS_TYPE_AGILITY] += 40;
            }
            return true;
        case 20007: // crusader
            if (type_ == CollectorType::MELEE)
            {
                stats[STATS_TYPE_STRENGTH] += 30;
            }
            return true;
        case 59620: // Berserk
            if (type_ == CollectorType::MELEE)
            {
                stats[STATS_TYPE_ATTACK_POWER] += 120;
            }
            return true;
        case 64440: // Blade Warding
            if (type_ == CollectorType::MELEE)
            {
                stats[STATS_TYPE_PARRY] += 50;
            }
            return true;
        case 64571:
            if (type_ == CollectorType::MELEE)
            {
                stats[STATS_TYPE_STAMINA] += 50;
            }
            return true;
        default:
            break;
    }
    {
        int allStatsAmount = 0;
        switch (enchantSpellId)
        {
            case 13624:
                allStatsAmount = 1;
                break;
            case 13625:
                allStatsAmount = 2;
                break;
            case 13824:
                allStatsAmount = 3;
                break;
            case 19988:
            case 44627:
            case 56527:
                allStatsAmount = 4;
                break;
            case 27959:
            case 56529:
                allStatsAmount = 6;
                break;
            case 44624:
                allStatsAmount = 8;
                break;
            case 60694:
            case 68251:
                allStatsAmount = 10;
                break;
            default:
                break;
        }
        if (allStatsAmount != 0)
        {
            stats[STATS_TYPE_AGILITY] += allStatsAmount;
            stats[STATS_TYPE_STRENGTH] += allStatsAmount;
            stats[STATS_TYPE_INTELLECT] += allStatsAmount;
            stats[STATS_TYPE_SPIRIT] += allStatsAmount;
            stats[STATS_TYPE_STAMINA] += allStatsAmount;
            return true;
        }
    }
    
    return false;
}

void StatsCollector::CollectByItemStatType(uint32 itemStatType, int32 val)
{
    switch (itemStatType)
    {
        case ITEM_MOD_MANA:
            break;
        case ITEM_MOD_HEALTH:
            break;
        case ITEM_MOD_AGILITY:
            stats[STATS_TYPE_AGILITY] += val;
            break;
        case ITEM_MOD_STRENGTH:
            stats[STATS_TYPE_STRENGTH] += val;
            break;
        case ITEM_MOD_INTELLECT:
            stats[STATS_TYPE_INTELLECT] += val;
            break;
        case ITEM_MOD_SPIRIT:
            stats[STATS_TYPE_SPIRIT] += val;
            break;
        case ITEM_MOD_STAMINA:
            stats[STATS_TYPE_STAMINA] += val;
            break;
        case ITEM_MOD_DEFENSE_SKILL_RATING:
            stats[STATS_TYPE_DEFENSE] += val;
            break;
        case ITEM_MOD_DODGE_RATING:
            stats[STATS_TYPE_DODGE] += val;
            break;
        case ITEM_MOD_PARRY_RATING:
            stats[STATS_TYPE_PARRY] += val;
            break;
        case ITEM_MOD_BLOCK_RATING:
            stats[STATS_TYPE_BLOCK] += val;
            break;
        case ITEM_MOD_HIT_MELEE_RATING:
            if (type_ == CollectorType::MELEE)
                stats[STATS_TYPE_HIT] += val;
            break;
        case ITEM_MOD_HIT_RANGED_RATING:
            if (type_ == CollectorType::RANGED)
                stats[STATS_TYPE_HIT] += val;
            break;
        case ITEM_MOD_HIT_SPELL_RATING:
            if (type_ == CollectorType::SPELL)
                stats[STATS_TYPE_HIT] += val;
            break;
        case ITEM_MOD_CRIT_MELEE_RATING:
            if (type_ == CollectorType::MELEE)
                stats[STATS_TYPE_CRIT] += val;
            break;
        case ITEM_MOD_CRIT_RANGED_RATING:
            if (type_ == CollectorType::RANGED)
                stats[STATS_TYPE_CRIT] += val;
            break;
        case ITEM_MOD_CRIT_SPELL_RATING:
            if (type_ == CollectorType::SPELL)
                stats[STATS_TYPE_CRIT] += val;
            break;
        case ITEM_MOD_HASTE_MELEE_RATING:
            if (type_ == CollectorType::MELEE)
                stats[STATS_TYPE_HASTE] += val;
            break;
        case ITEM_MOD_HASTE_RANGED_RATING:
            if (type_ == CollectorType::RANGED)
                stats[STATS_TYPE_HASTE] += val;
            break;
        case ITEM_MOD_HASTE_SPELL_RATING:
            if (type_ == CollectorType::SPELL)
                stats[STATS_TYPE_HASTE] += val;
            break;
        case ITEM_MOD_HIT_RATING:
            stats[STATS_TYPE_HIT] += val;
            break;
        case ITEM_MOD_CRIT_RATING:
            stats[STATS_TYPE_CRIT] += val;
            break;
        case ITEM_MOD_RESILIENCE_RATING:
            stats[STATS_TYPE_RESILIENCE] += val;
            break;
        case ITEM_MOD_HASTE_RATING:
            stats[STATS_TYPE_HASTE] += val;
            break;
        case ITEM_MOD_EXPERTISE_RATING:
            stats[STATS_TYPE_EXPERTISE] += val;
            break;
        case ITEM_MOD_ATTACK_POWER:
            stats[STATS_TYPE_ATTACK_POWER] += val;
            break;
        case ITEM_MOD_RANGED_ATTACK_POWER:
            if (type_ == CollectorType::RANGED)
                stats[STATS_TYPE_ATTACK_POWER] += val;
            break;
        case ITEM_MOD_MANA_REGENERATION:
            stats[STATS_TYPE_MANA_REGENERATION] += val;
            break;
        case ITEM_MOD_ARMOR_PENETRATION_RATING:
            stats[STATS_TYPE_ARMOR_PENETRATION] += val;
            break;
        case ITEM_MOD_SPELL_POWER:
            stats[STATS_TYPE_SPELL_POWER] += val;
            break;
        case ITEM_MOD_HEALTH_REGEN:
            stats[STATS_TYPE_HEALTH_REGENERATION] += val;
            break;
        case ITEM_MOD_SPELL_PENETRATION:
            stats[STATS_TYPE_SPELL_PENETRATION] += val;
            break;
        case ITEM_MOD_BLOCK_VALUE:
            stats[STATS_TYPE_BLOCK] += val;
            break;
        case ITEM_MOD_SPELL_HEALING_DONE:  // deprecated
        case ITEM_MOD_SPELL_DAMAGE_DONE:   // deprecated
        default:
            break;
    }
}