#pragma once
#include "Core/CoreDataTypes.h"
#include "Core/Collections/FixedArrary.h"

//Forward Delcarations
class EventHandlerContainer;
struct EventData;
class EventHandler;


enum EEventType
{
    OnTurnStartGlobal,
    OnTurnEndGlobal,
    OnTurnStartLocal,
    OnTurnEndLocal,
    OnMoveInitiated,
    OnMove,
    OnFullyHealed,
    OnPostDeath,
    OnDeath,
    OnKillMonster,
    RegenerateStats,
    OnEnergyGained,
    OnAttacked,
    OnDealDamage,
    OnTakeDamage,
    OnHealing,
    OnApplyStatusEffects,
    OnActivateItem,
    OnCastAbility,
    OnGainResources,
    OnGainXP,
    OnLevelUp,
    OnLoseResources,
    OnRegenerateAbilityStats,
    OnCheckAvailability,
    OnTargetsSelected,
    OnPreCast,
    OnPostCast,
    OnTargetedByAbility,
    OnHitByAbility,
    OnStartAttack,
    OnStartAttackTarget,
    OnGenerateArmedAttacks,
    OnBeginPrimaryAttack,
    OnPrimaryAttackResult,
    OnEndPrimaryAttack,
    OnBeginSecondaryAttack,
    OnSecondaryAttackResult,
    OnEndSecondaryAttack,
    OnGenerateUnarmedAttacks,
    OnBeginUnarmedAttack,
    OnUnarmedAttackResult,
    OnEndUnarmedAttack,
    OnBeforePrimaryAttackTarget,
    OnAfterPrimaryAttackTarget,
    OnBeforeSecondaryAttackTarget,
    OnAfterSecondaryAttackTarget,
    OnBeforeUnarmedAttackTarget,
    OnAfterUnarmedAttackTarget,
    OnGenerateLOSPreCollection,
    OnGenerateLOSPostCollection
};

class EventHandlerContainer
{
public:
    std::vector<EventHandler*> eventHandlers;
};

struct EventData
{
    EEventType type;
    FixedArray<EventHandlerContainer*, 32> targets;
};

class EventHandler
{
public:
    virtual int GetPriority(EEventType eventType) const { return -1; }
    virtual bool HandleEvent(EEventType type, EventData& data) { return false; }
};

void FireEvent(EventData& data);

template <EEventType T, int prio, int num>
class TestEvent : public EventHandler
{
public:
    int GetPriority(EEventType eventType) const override
    {
        switch (eventType)
        {
        case T:
            return prio;
        default:
            return -1;
        }
    }

    bool HandleEvent(EEventType eventType, EventData& data) override
    {
        localValue++;
        //DEBUG_PRINT("Event %d: Handling with prio %d", num, prio);
        return false;
    }

    int localValue;
};