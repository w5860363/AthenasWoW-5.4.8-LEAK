#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "heart_of_fear.h"
#include "Vehicle.h"

enum Events
{
    EVENT_INTRO           = 1,
};

enum Actions
{
    // Garalon
    ACTION_FUR_SWIPE_FAILED = 1,
    ACTION_PHEROMONES_JUMP_OR_PLAYERS_UNDERNEATH, // Normal Difficulty - Galaron casts Crush when Pheromones jump to another player / when he detects players underneath him.
};

enum Yells
{
    // He's a bug! Has no yells, just announces.
    ANN_CRUSH   = 0, // Garalon begins to [Crush] his opponents!
    ANN_MEND    = 1, // Garalon [Mends] one of his legs!
    ANN_FURY    = 2, // Garalon fails to hit at least two players and gains [Fury]!
    ANN_DAMAGED = 3, // Garalon's massive armor plating begins to crack and split!
    ANN_BERSERK = 4  // Garalon becomes [Enraged] and prepares to execute a [Massive Crush]!
};

enum Spells
{
    SPELL_FURIOUS_SWIPE        = 122735, // Primary attack ability.
    SPELL_FURY                 = 122754, // If he doesn't hit at least two targets with Swipe.

    SPELL_CRUSH_BODY_VIS       = 122082, // Visual for boss body, blue circle underneath. Triggers SPELL_CRUSH_TRIGGER each sec.
    SPELL_CRUSH_TRIGGER        = 117709, // Dummy effect for SPELL_CRUSH_DUMMY casted on players in 14 yards.
    SPELL_CRUSH_DUMMY          = 124068, // Dummy placed on players underneath. They take the extra damage.

    SPELL_CRUSH                = 122774, // Extra damage for players underneath (in 14 yards), on Effect 1.
    SPELL_MASSIVE_CRUSH        = 128555, // Hard enrage spell, used to wipe the raid: 10 million damage.

    SPELL_PHER_INIT_CAST       = 123808, // When boss is pulled, after 2 secs this casts SPELL_PHEROMONES_AURA on nearest player.
    SPELL_PHEROMONES_AURA      = 122835, // Trig SPELL_PHEROMONES_TAUNT, SPELL_PUNGENCY, SPELL_PHEROMONES_DMG, SPELL_PHEROMONES_FC / 2s + SPELL_PHER_CANT_BE_HIT, SPELL_PHEROMONES_DUMMY.

    SPELL_PHEROMONES_TAUNT     = 123109, // Garalon Taunt / Attack Me spell.
    SPELL_PUNGENCY             = 123081, // Stacking 10% damage increase aura.
    SPELL_PHEROMONES_DMG       = 123092, // Damage spell, triggers SPELL_SUMM_PHER_TRAIL.
    SPELL_SUMM_PHER_TRAIL      = 128573, // Summon spell for NPC_PHEROMONE_TRAIL.
    SPELL_PHEROMONES_FC        = 123100, // Force Cast of SPELL_PHEROMONES_JUMP in 5 yards.
    SPELL_PHER_CANT_BE_HIT     = 124056, // Some kind of dummy check.
    SPELL_PHEROMONES_DUMMY     = 130662, // Special prereq dummy for SPELL_PHER_INIT_CAST.

    SPELL_DAMAGED              = 123818, // Heroic, 33%. Uses melee, increased speed, ignores Pheromones taunt.
    SPELL_BERSERK              = 47008,

    SPELL_PHER_TRAIL_DMG_A     = 123106, // Triggers SPELL_PHER_TRAIL_DMG each sec.
    SPELL_PHER_TRAIL_DMG       = 123120, // 25000 damage in 4 yards.

    SPELL_SHARED_HEALTH        = 117189, // Triggers share damage aura (from body to vehicle)

    SPELL_GARALON_INTRO        = 123991, // NYI
    SPELL_GARALON_INTRO_DONE   = 128547,
    SPELL_GARALON_INTRO_DONE_2 = 128549,

    // Ride Vehicle auras for each Leg.
    SPELL_RIDE_FRONT_RIGHT     = 123430, // Ride Vehicle (Front Right Leg) - Triggers 122757 Weak Points aura which triggers SPELL_WEAK_POINT_VIS1 and SPELL_WEAK_POINTS_FR.
    SPELL_RIDE_FRONT_LEFT      = 123431, // Ride Vehicle (Front Left Leg)  - Triggers 123424 Weak Points aura which triggers SPELL_WEAK_POINT_VIS2 and SPELL_WEAK_POINTS_FL.
    SPELL_RIDE_BACK_LEFT       = 123432, // Ride Vehicle (Back Left Leg)   - Triggers 123425 Weak Points aura which triggers SPELL_WEAK_POINT_VIS3 and SPELL_WEAK_POINTS_BL.
    SPELL_RIDE_BACK_RIGHT      = 123433, // Ride Vehicle (Back Right Leg)  - Triggers 123427 Weak Points aura which triggers SPELL_WEAK_POINT_VIS4 and SPELL_WEAK_POINTS_BR.

    // Weak Points: 12 yard 100% proximity leg damage increase.
    SPELL_WEAK_POINTS_FR       = 123235, // Right, front.
    SPELL_WEAK_POINTS_FL       = 123423, // Left,  front.
    SPELL_WEAK_POINTS_BL       = 123426, // Left,  back.
    SPELL_WEAK_POINTS_BR       = 123428, // Right, back.

    // Weak Points: Visual triggers for Broken Leg animations. --! No scripting usage.
    SPELL_WEAK_POINT_VIS1      = 128599, // All these 4 spells trigger SPELL_BROKEN_LEG_VIS (one for each Leg).
    SPELL_WEAK_POINT_VIS2      = 128596,
    SPELL_WEAK_POINT_VIS3      = 128600,
    SPELL_WEAK_POINT_VIS4      = 128601,

    // Broken Leg: Boss 15% speed decrease stacking aura / 3 % HP damage spell. Stacks to 4 x 15% = 60% speed decrease.
    SPELL_BROKEN_LEG           = 122786, // Boss self-cast spell.

    // Broken Leg: Visual (triggered by spells from SPELL_WEAK_POINT_VIS) - "Blood" dripping out.
    SPELL_BROKEN_LEG_VIS       = 123500,

    // Mend Leg: Boss Leg heal spell. Used every 30 seconds after a leg dies.
    SPELL_MEND_LEG             = 123495  // Dummy, handled to "revive" the leg. Triggers 123796 script effect to remove SPELL_BROKEN_LEG from boss, we don't use it.
};

const std::vector<uint32> WeakSpotList =
{
    SPELL_WEAK_POINTS_FR,
    SPELL_WEAK_POINTS_FL,
    SPELL_WEAK_POINTS_BL,
    SPELL_WEAK_POINTS_BR,
};

static Position const PositionParabolicStrat{ -1800.708f, 474.6267f, 465.0608f, 3.203864f };
static Position const PositionParabolicEnd{ -1907.94f,  475.651f,  470.958f,  3.203864f };

enum MovementPoints
{
    POINT_PARABOLIC_START   = 1,
    POINT_PARABOLIC_END     = 2,
};

enum Misc
{
    NPC_GARALON_BODY    = 63191,
    BODY_SEAT           = 4,
    GARALON_VEHICLE_ID  = 2257,

    WORLD_STATE_LIKE_AN_ARROR_TO_THE_FACE = 11791,
};

enum eGaralonEvents
{
    // Garalon
    EVENT_FURIOUS_SWIPE   = 1,      // About 8 - 11 seconds after pull. Every 8 seconds.
    EVENT_PHEROMONES,               // About 2 -  3 seconds after pull.
    EVENT_CRUSH,                    // About 30     seconds after pull. Every 37.5 seconds.
    EVENT_MEND_LEG,

    EVENT_GARALON_BERSERK           // Goes with SPELL_MASSIVE_CRASH.
};

class boss_garalon : public CreatureScript
{
    public:
        boss_garalon() : CreatureScript("boss_garalon") { }

        struct boss_garalonAI : public BossAI
        {
            boss_garalonAI(Creature* creature) 
                : BossAI(creature, DATA_GARALON) { }

            bool damagedHeroic, castingCrush;
            std::deque<uint64> legsToRestore;

            void SummonAndAddLegs()
            {
                static uint32 const legSpells[] =
                {
                    SPELL_RIDE_BACK_LEFT, SPELL_RIDE_BACK_RIGHT, SPELL_RIDE_FRONT_LEFT, SPELL_RIDE_FRONT_RIGHT
                };

                for (auto&& spell : legSpells)
                {
                    if (Creature* leg = me->SummonCreature(NPC_GARALON_LEG, me->GetPositionX(), me->GetPositionY(), me->GetPositionZ(), me->GetOrientation(), TEMPSUMMON_MANUAL_DESPAWN))
                    {
                        leg->CastSpell(me, spell, true);
                        leg->SetReactState(REACT_PASSIVE);
                    }
                }
            }

            void SummonBody()
            {
                if (!me->GetVehicleKit() || me->GetVehicleKit()->GetPassenger(BODY_SEAT))
                    return;
                if (Creature* body = me->SummonCreature(NPC_GARALON_BODY, me->GetPositionX(), me->GetPositionY(), me->GetPositionZ(), me->GetOrientation(), TEMPSUMMON_MANUAL_DESPAWN))
                {
                    body->EnterVehicle(me, BODY_SEAT, true);
                    body->SetMaxHealth(me->GetMaxHealth());
                    body->SetFullHealth();
                }
            }

            void ScheduleTasks() override
            {
                scheduler
                    .Schedule(Seconds(8), [this](TaskContext context)
                    {
                        uint32 targetGUID = 0;
                        if (Unit* target = me->GetVictim())
                            targetGUID = target->GetGUID();

                        me->PrepareChanneledCast(me->GetOrientation(), SPELL_FURIOUS_SWIPE);

                        scheduler.Schedule(Milliseconds(2600), [=](TaskContext)
                        {
                            me->RemoveChanneledCast(targetGUID);
                        });

                        context.Repeat(Seconds(8));
                    })
                    .Schedule(Minutes(7), [this](TaskContext)
                    {
                        Talk(ANN_BERSERK);
                        me->AddAura(SPELL_BERSERK, me);
                        DoCast(me, SPELL_MASSIVE_CRUSH);
                    });

                if (me->GetMap()->IsHeroic())
                    scheduler.Schedule(Seconds(35), [this](TaskContext context)
                    {
                        if (!me->HasAura(SPELL_DAMAGED))
                        {
                            DoCast(me, SPELL_CRUSH);
                            context.Repeat();
                        }
                    });
            }

            void DespawnCreatures(uint32 entry)
            {
                std::list<Creature*> creatures;
                GetCreatureListWithEntryInGrid(creatures, me, entry, 1000.0f);

                for (auto&& it : creatures)
                    it->DespawnOrUnsummon();
            }

            void Reset() override
            {
                _Reset();
                legsToRestore.clear();
                me->SetReactState(REACT_AGGRESSIVE);
                damagedHeroic = false;
                castingCrush  = false;
                RemoveFeromonesFromPlayers();

                if (instance)
                    instance->DoRemoveBloodLustDebuffSpellOnPlayers();

                DoCast(SPELL_SHARED_HEALTH);

                me->GetMap()->SetWorldState(WORLD_STATE_LIKE_AN_ARROR_TO_THE_FACE, 1);
            }

            void InitializeAI() override 
            {
                me->SetVisible(false);
                me->SetReactState(REACT_PASSIVE);
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                damagedHeroic = false;
                castingCrush  = false;

                if (instance && instance->GetData(DATA_GARALON) > DONE && me->IsAlive())
                    me->AI()->DoAction(ACTION_GARALON_INITIALIZE);

                // Reset won't be called
                me->GetMap()->SetWorldState(WORLD_STATE_LIKE_AN_ARROR_TO_THE_FACE, 1);
            }

            void EnterEvadeMode() override
            {
                // Open walls
                std::list<GameObject*> doorList;
                GetGameObjectListWithEntryInGrid(doorList, me, GO_GARALON_DOOR, 100.0f);

                for (GameObject* door : doorList)
                    door->SetGoState(GO_STATE_ACTIVE);
                
                DespawnCreatures(NPC_PHEROMONE_TRAIL);

                if (instance && me->GetVehicleKit())
                {
                    // Before calling BossAI::EnterEvadeMode
                    for (auto&& it : me->GetVehicleKit()->Seats)
                        if (Unit* passenger = ObjectAccessor::GetUnit(*me, it.second.Passenger.Guid))
                            instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, passenger);
                    me->GetVehicleKit()->RemoveAllPassengers();

                    instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_PHEROMONES_AURA);
                    instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_PUNGENCY);
                    instance->DoRemoveBloodLustDebuffSpellOnPlayers();
                }

                BossAI::EnterEvadeMode();
            }

            void RemoveFeromonesFromPlayers()
            {
                std::list<Player*> playersInArea;
                GetPlayerListInGrid(playersInArea, me, 200.0f);

                for (auto&& player : playersInArea)
                    player->RemoveAura(SPELL_PHEROMONES_AURA);
            }

            void JustReachedHome() override
            {
                _JustReachedHome();

                if (instance)
                    instance->SetBossState(DATA_GARALON, FAIL);

                SummonBody();
            }

            void EnterCombat(Unit* who) override
            {
                // Activation of the walls
                std::list<GameObject*> doorList;
                GetGameObjectListWithEntryInGrid(doorList, me, GO_GARALON_DOOR, 100.0f);

                for (GameObject* door : doorList)
                    door->SetGoState(GO_STATE_READY);

                SummonAndAddLegs();

                me->AddAura(SPELL_CRUSH_BODY_VIS, me);  // And add the body crush marker.

                events.ScheduleEvent(EVENT_FURIOUS_SWIPE, urand(8000, 11000));
                events.ScheduleEvent(EVENT_PHEROMONES, urand(2000, 3000));
                events.ScheduleEvent(EVENT_CRUSH, 30000); // First Crush always seems to have this timer, on any difficulty.
                events.ScheduleEvent(EVENT_GARALON_BERSERK, 7 * MINUTE * IN_MILLISECONDS); // 7 min enrage timer.

                if (instance)
                {
                    for (auto&& it : me->GetVehicleKit()->Seats)
                        if (Unit* passenger = ObjectAccessor::GetUnit(*me, it.second.Passenger.Guid))
                            instance->SendEncounterUnit(ENCOUNTER_FRAME_ENGAGE, passenger);

                    instance->SetBossState(DATA_GARALON, IN_PROGRESS);
                    instance->SendEncounterUnit(ENCOUNTER_FRAME_ENGAGE, me); // Add
                }

                if (Creature* meljarak = GetClosestCreatureWithEntry(me, NPC_WIND_LORD_MELJARAK_INTRO, 50.0f))
                    meljarak->AI()->DoAction(ACTION_MELJARAK_GARALON_IN_COMBAT);
            }

            void MovementInform(uint32 type, uint32 pointId) override
            {
                switch (pointId)
                {
                    case POINT_PARABOLIC_START:
                    {
                        if (type == POINT_MOTION_TYPE)
                            me->m_Events.Schedule(500, [=]
                        {
                            me->GetMotionMaster()->MoveJump(PositionParabolicEnd, 15.00194f, 27.7746045783614f, POINT_PARABOLIC_END);
                        });
                        break;
                    }
                    case POINT_PARABOLIC_END:
                    {
                        if (type != EFFECT_MOTION_TYPE)
                            break;

                        events.Reset();
                        me->SetHomePosition(me->GetPositionX(), me->GetPositionY(), me->GetPositionZ(), me->GetOrientation());
                        me->SetReactState(REACT_AGGRESSIVE);
                        me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                        me->SetFlying(false);
                        me->RemoveUnitMovementFlag(MOVEMENTFLAG_FLYING);
                        me->OverrideInhabitType(INHABIT_GROUND);
                        me->UpdateMovementFlags();
                        me->CastSpell(me, SPELL_GARALON_INTRO_DONE);
                        // TODO: Basically this must be not in the such way. But we don't need to reset it to original vehicle, so...
                        me->RemoveVehicleKit();
                        me->CreateVehicleKit(GARALON_VEHICLE_ID, me->GetEntry());
                        me->CastSpell(me, SPELL_GARALON_INTRO_DONE_2);
                        DoCast(SPELL_SHARED_HEALTH);
                        SummonBody();
                        break;
                    }
                }
            }

            void DoAction(int32 actionId) override 
            {
                switch (actionId)
                {
                    // Furious Swipe failed to hit at least 2 targets, Garalon gains Fury.
                    case ACTION_FUR_SWIPE_FAILED:
                        Talk(ANN_FURY);
                        me->CastSpell(me, SPELL_FURY, true);
                        break;
                        // Pheromones jumped to another player / there are players underneath his body, in Normal Difficulty Garalon casts Crush.
                    case ACTION_PHEROMONES_JUMP_OR_PLAYERS_UNDERNEATH:
                        if (IsHeroic())
                            break;

                        if (!castingCrush && !me->HasAura(SPELL_DAMAGED))
                        {
                            scheduler.Schedule(Seconds(3), [this](TaskContext)
                            {
                                if (!me->HasAura(SPELL_DAMAGED))
                                    DoCast(me, SPELL_CRUSH);
                                castingCrush = false;
                            });
                            castingCrush = true;
                        }
                        break;
                    case ACTION_GARALON_INITIALIZE:
                        me->SetVisible(true);
                        me->SetCanFly(true);
                        me->m_Events.Schedule(1000, [=]
                        {
                            me->GetMotionMaster()->MovePoint(POINT_PARABOLIC_START, PositionParabolicStrat);
                        });
                        break;
                    default:
                        break;
                }
            }

            void DamageTaken(Unit*, uint32& damage)
            {
                // Only for debug purposes
                if (damage >= me->GetHealth())
                {
                    if (Creature* body = me->FindNearestCreature(NPC_GARALON_BODY, 200.0f))
                    {
                        Aura* sharedDamage = body->GetAura(117190);
                        if (body->GetHealth() > damage || !sharedDamage || sharedDamage->GetCaster() != me)
                        {
                            TC_LOG_ERROR("garalon", "Garalon: vehicle %" UI64FMTD ", body " UI64FMTD ", health %u, damage %u, has aura %u, aura caster " UI64FMTD,
                                me->GetGUID(), body->GetGUID(), body->GetHealth(), damage, sharedDamage ? 1 : 0, sharedDamage ? sharedDamage->GetCasterGUID() : uint64(0));
                            if (Player* player = me->GetMap()->GetFirstPlayerInInstance())
                                player->Kill(body);
                        }
                    }
                }
            }

            void JustDied(Unit* /*killer*/) override
            {
                _JustDied();

                RemoveFeromonesFromPlayers();
                DespawnCreatures(NPC_PHEROMONE_TRAIL);
                //Talk(TALK_DEATH);
                if (instance)
                {
                    instance->DoRemoveBloodLustDebuffSpellOnPlayers();
                    instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_PUNGENCY);
                }

                if (Creature* meljarak = GetClosestCreatureWithEntry(me, NPC_WIND_LORD_MELJARAK_INTRO, 50.0f))
                    meljarak->AI()->DoAction(ACTION_MELJARAK_LEAVE);
                me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
            }

            void UpdateAI(uint32 diff) override
            {
                if (!UpdateVictim())
                    return;

                // Damaged debuff for Heroic.
                if (!damagedHeroic && me->HealthBelowPct(35) && IsHeroic())
                {
                    Talk(ANN_DAMAGED);
                    me->CastSpell(me, SPELL_DAMAGED, false);
                    damagedHeroic = true;
                }

                scheduler.Update(diff);

                if (me->HasAura(SPELL_DAMAGED)) // Only on Heroic, and from 33% HP.
                    DoMeleeAttackIfReady();

                EnterEvadeIfOutOfCombatArea(diff);
            }

            void SpellHit(Unit* caster, SpellInfo const* spell) override
            {
                if (spell->Id == SPELL_BROKEN_LEG)
                {
                    legsToRestore.push_back(caster->GetGUID());
                    scheduler.Schedule(Seconds(30), [this](TaskContext)
                    {
                        DoCast(me, SPELL_MEND_LEG);
                    });
                }
            }

            Creature* GetBrokenLeg()
            {
                if (legsToRestore.empty())
                    return nullptr;
                Creature* leg = ObjectAccessor::GetCreature(*me, legsToRestore.front());
                legsToRestore.pop_front();
                return leg;
            }

            void GetPassengerEnterPosition(Unit*, int8 seat, Position& pos) override
            {
                static std::vector<Position> const offsets =
                {
                    {  16.5f, -12.5f, 0.0f },
                    {  16.5f,  12.5f, 0.0f },
                    { -22.5f, -15.0f, 0.0f },
                    { -22.5f,  15.0f, 0.0f },
                };
                if (seat >= 0 && size_t(seat) < offsets.size())
                    pos.Relocate(offsets[seat]);
            }
        };

        CreatureAI* GetAI(Creature* creature) const override
        {
            return new boss_garalonAI(creature);
        }
};

// 63191
class npc_garalon_body : public CreatureScript
{
    public:
        npc_garalon_body() : CreatureScript("npc_garalon_body") { }

        struct npc_garalon_bodyAI : public ScriptedAI
        {
            npc_garalon_bodyAI(Creature* c) : ScriptedAI(c)
            {
                me->SetDisplayId(45084);
                instance = me->GetInstanceScript();
            }

            InstanceScript* instance;

            void UpdateAI(uint32 diff) override { }

            void DamageTaken(Unit* /*attacker*/, uint32& damage) override
            {
                if (damage >= me->GetHealth())
                {
                    if (Creature* garalon = me->GetVehicleCreatureBase())
                    {
                        Player* player = me->GetMap()->GetFirstPlayerInInstance();
                        garalon->LowerPlayerDamageReq(garalon->GetMaxHealth());
                        if (player)
                            garalon->SetLootRecipient(player);

                        // Only for debug purposes
                        Aura* sharedDamage = me->GetAura(117190);
                        if (garalon->GetHealth() > damage || !sharedDamage || sharedDamage->GetCaster() != garalon)
                        {
                            TC_LOG_ERROR("garalon", "Garalon body: vehicle %" UI64FMTD ", body " UI64FMTD ", health %u, damage %u, has aura %u, aura caster " UI64FMTD,
                                garalon->GetGUID(), me->GetGUID(), garalon->GetHealth(), damage, sharedDamage ? 1 : 0, sharedDamage ? sharedDamage->GetCasterGUID() : uint64(0));
                            if (Player* player = me->GetMap()->GetFirstPlayerInInstance())
                                player->Kill(garalon);
                        }
                    }
                }
            }

            void EnterCombat(Unit* who) override
            {
                if (Creature* garalon = me->GetVehicleCreatureBase())
                    garalon->AI()->AttackStart(who);
            }
        };

        CreatureAI* GetAI(Creature* creature) const override
        {
            return new npc_garalon_bodyAI(creature);
        }
};

// Garalon's Leg: 63053.
class npc_garalon_leg : public CreatureScript
{
    public:
        npc_garalon_leg() : CreatureScript("npc_garalon_leg") { }

        struct npc_garalon_legAI : public ScriptedAI
        {
            npc_garalon_legAI(Creature* creature) : ScriptedAI(creature) { }

            void Reset() override
            {
                me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IMMUNE_TO_PC | UNIT_FLAG_IMMUNE_TO_NPC | UNIT_FLAG_NOT_SELECTABLE);
                me->SetDisplayId(me->GetCreatureTemplate()->Modelid4);
            }

            void DamageTaken(Unit* attacker, uint32& damage) override
            {
                // Basically, must not happen
                if (me->HasAura(SPELL_BROKEN_LEG_VIS))
                {
                    damage = 0;
                    return;
                }

                // Players cannot actually kill the legs, they damage them enough and they become unselectable etc.
                if (damage > me->GetHealth())
                {
                    damage = 0;
                    me->AddAura(SPELL_BROKEN_LEG_VIS, me);
                    me->CastSpell(me, SPELL_BROKEN_LEG, TRIGGERED_WITH_SPELL_START);
                    me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IMMUNE_TO_PC | UNIT_FLAG_IMMUNE_TO_NPC | UNIT_FLAG_NOT_SELECTABLE);

                    me->GetMap()->SetWorldState(WORLD_STATE_LIKE_AN_ARROR_TO_THE_FACE, 0);
                }
            }

            void UpdateAI(uint32 diff) override { } // Override, no evade on !UpdateVictim() + no melee.
        };

        CreatureAI* GetAI(Creature* creature) const override
        {
            return new npc_garalon_legAI(creature);
        }
};

// Pheromone Trail: 63021.
class npc_pheromone_trail : public CreatureScript
{
    public:
        npc_pheromone_trail() : CreatureScript("npc_pheromone_trail") { }

        struct npc_pheromone_trailAI : public ScriptedAI
        {
            npc_pheromone_trailAI(Creature* creature) : ScriptedAI(creature) { }

            uint32 pheromonesDelay = 0;

            void IsSummonedBy(Unit* /*summoner*/) override
            {
                pheromonesDelay = 0;
                me->SetDisplayId(11686); // invisible
                me->SetInCombatWithZone();
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE | UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE);
                me->SetReactState(REACT_PASSIVE);

                me->AddAura(SPELL_PHER_TRAIL_DMG_A, me); // Damage aura.

                me->m_Events.Schedule(1500, [this]()
                {
                    pheromonesDelay = 1;
                });
            }

            uint32 GetData(uint32 type) const override
            {
                if (type == TYPE_PHEROMONES_DELAY)
                    return pheromonesDelay;

                return 0;
            }
        };

        CreatureAI* GetAI(Creature* creature) const override
        {
            return new npc_pheromone_trailAI(creature);
        }
};

// Mend Leg: 123495
class spell_garalon_mend_leg_trigger : public SpellScript
{
    PrepareSpellScript(spell_garalon_mend_leg_trigger);

    void FilterTargets(std::list<WorldObject*>& targets)
    {
        Creature* leg = nullptr;
        if (auto script = dynamic_cast<boss_garalon::boss_garalonAI*>(GetCaster()->GetAI()))
            leg = script->GetBrokenLeg();
        targets.remove_if([=](WorldObject const* target) { return target != leg; });
    }

    void RemoveTarget(WorldObject*& target)
    {
        target = nullptr;
    }

    void HandleHit()
    {
        GetHitUnit()->RemoveAurasDueToSpell(SPELL_BROKEN_LEG_VIS);
        GetHitUnit()->SetHealth(GetHitUnit()->GetMaxHealth());
        GetHitUnit()->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IMMUNE_TO_PC | UNIT_FLAG_IMMUNE_TO_NPC | UNIT_FLAG_NOT_SELECTABLE);
    }

    void Register() override
    {
        OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_garalon_mend_leg_trigger::FilterTargets, EFFECT_0, TARGET_UNIT_SRC_AREA_ENTRY);
        OnObjectTargetSelect += SpellObjectTargetSelectFn(spell_garalon_mend_leg_trigger::RemoveTarget, EFFECT_1, TARGET_UNIT_CASTER);
        OnHit += SpellHitFn(spell_garalon_mend_leg_trigger::HandleHit);
    }
};

// Mend Leg: 123796
class spell_garalon_mend_leg : public SpellScript
{
    PrepareSpellScript(spell_garalon_mend_leg);

    void HandleHit()
    {
        if (Aura* brokenLegAura = GetHitUnit()->GetAura(SPELL_BROKEN_LEG))
            brokenLegAura->ModStackAmount(-1);
    }

    void Register() override
    {
        OnHit += SpellHitFn(spell_garalon_mend_leg::HandleHit);
    }
};

// Furious Swipe: 122735.
class spell_garalon_furious_swipe : public SpellScriptLoader
{
public:
    spell_garalon_furious_swipe() : SpellScriptLoader("spell_garalon_furious_swipe") { }

    class spell_garalon_furious_swipeSpellScript : public SpellScript
    {
        PrepareSpellScript(spell_garalon_furious_swipeSpellScript);

        bool Validate(SpellInfo const* spellEntry) override
        {
            if (!sSpellMgr->GetSpellInfo(SPELL_FURIOUS_SWIPE))
                return false;
            return true;
        }

        bool Load() override
        {
            return true;
        }

        void FilterTargets(std::list<WorldObject*>& targets)
        {
            // The target list size indicates how many players Garalon hits. We let him know what to do afterwards.
            if (targets.empty() || targets.size() < 2) // If he hits less than two players, it's time to go for Fury.
            {
                CAST_AI(boss_garalon::boss_garalonAI, GetCaster()->ToCreature()->AI())->DoAction(ACTION_FUR_SWIPE_FAILED);
                //if (Unit* caster = GetCaster())
                    //caster->GetAI()->DoAction(ACTION_FUR_SWIPE_FAILED);
            }
        }

        void Register() override
        {
            OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_garalon_furious_swipeSpellScript::FilterTargets, EFFECT_0, TARGET_UNIT_CONE_ENEMY_104);
        }
    };

    SpellScript* GetSpellScript() const override
    {
        return new spell_garalon_furious_swipeSpellScript();
    }
};

// Pheromones (Force_Cast, 2 sec. cast time): 123808.
class spell_garalon_pheromones_forcecast : public SpellScriptLoader
{
public:
    spell_garalon_pheromones_forcecast() : SpellScriptLoader("spell_garalon_pheromones_forcecast") { }

    class spell_garalon_pheromones_forcecastSpellScript : public SpellScript
    {
        PrepareSpellScript(spell_garalon_pheromones_forcecastSpellScript);

        bool Validate(SpellInfo const* spellEntry) override
        {
            if (!sSpellMgr->GetSpellInfo(SPELL_PHER_INIT_CAST))
                return false;
            return true;
        }

        bool Load() override
        {
            return true;
        }

        void FilterTargets(std::list<WorldObject*>& targets)
        {
            if (targets.empty())
                return;

            // Find the nearest player in 100 yards, and that will be the target (done like that on off).
            WorldObject* target = GetCaster()->ToCreature()->SelectNearestPlayer(100.0f);

            targets.clear();
            targets.push_back(target);
        }

        void Register() override
        {
            OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_garalon_pheromones_forcecastSpellScript::FilterTargets, EFFECT_0, TARGET_UNIT_SRC_AREA_ENTRY);
        }
    };

    SpellScript* GetSpellScript() const override
    {
        return new spell_garalon_pheromones_forcecastSpellScript();
    }
};

// Crush Trigger: 117709.
class spell_garalon_crush_trigger : public SpellScriptLoader
{
public:
    spell_garalon_crush_trigger() : SpellScriptLoader("spell_garalon_crush_trigger") { }

    class spell_garalon_crush_triggerSpellScript : public SpellScript
    {
        PrepareSpellScript(spell_garalon_crush_triggerSpellScript);

        bool Validate(SpellInfo const* spellEntry) override
        {
            if (!sSpellMgr->GetSpellInfo(SPELL_CRUSH_TRIGGER))
                return false;
            return true;
        }

        bool Load() override
        {
            return true;
        }

        void HandleDummy(SpellEffIndex /*effIndex*/)
        {
            // If there are no hit players, means there are no players underneath Garalon's body, so there's nothing to do.
            if (!GetCaster() || !GetHitUnit())
                return;

            // Now, if there are players under Garalon, he will cast Crush.
            GetCaster()->ToCreature()->AI()->DoAction(ACTION_PHEROMONES_JUMP_OR_PLAYERS_UNDERNEATH);
        }

        void Register() override
        {
            OnEffectHitTarget += SpellEffectFn(spell_garalon_crush_triggerSpellScript::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
        }
    };

    SpellScript* GetSpellScript() const override
    {
        return new spell_garalon_crush_triggerSpellScript();
    }
};

// Target check for Pheromones  Taunt / Attack Me + Broken Leg spells.
class BossCheck : public std::unary_function<Unit*, bool>
{
public:
    bool operator()(WorldObject* object)
    {
        return object->GetEntry() != NPC_GARALON;
    }
};

// Pheromones Taunt: 123109.
class spell_garalon_pheromones_taunt : public SpellScriptLoader
{
public:
    spell_garalon_pheromones_taunt() : SpellScriptLoader("spell_garalon_pheromones_taunt") { }

    class spell_garalon_pheromones_tauntSpellScript : public SpellScript
    {
        PrepareSpellScript(spell_garalon_pheromones_tauntSpellScript);

        bool Validate(SpellInfo const* spellEntry) override
        {
            if (!sSpellMgr->GetSpellInfo(SPELL_CRUSH_TRIGGER))
                return false;
            return true;
        }

        bool Load() override
        {
            return true;
        }

        void FilterTargets(std::list<WorldObject*>& targets)
        {
            if (targets.empty())
                return;

            // Only the boss gets taunted.
            targets.remove_if(BossCheck());
        }

        void Register() override
        {
            OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_garalon_pheromones_tauntSpellScript::FilterTargets, EFFECT_0, TARGET_UNIT_SRC_AREA_ENTRY);
            OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_garalon_pheromones_tauntSpellScript::FilterTargets, EFFECT_1, TARGET_UNIT_SRC_AREA_ENTRY);
        }
    };

    SpellScript* GetSpellScript() const override
    {
        return new spell_garalon_pheromones_tauntSpellScript();
    }
};

// Broken Leg: 122786.
class spell_garalon_broken_leg : public SpellScriptLoader
{
public:
    spell_garalon_broken_leg() : SpellScriptLoader("spell_garalon_broken_leg") { }

    class spell_garalon_broken_leg_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_garalon_broken_leg_SpellScript);

        bool Validate(SpellInfo const* spellEntry) override
        {
            if (!sSpellMgr->GetSpellInfo(SPELL_BROKEN_LEG))
                return false;
            return true;
        }

        bool Load() override
        {
            return true;
        }

        void FilterTargets(std::list<WorldObject*>& targets)
        {
            /*if (targets.empty())
                return;
            // Only casted by boss on self.
            targets.remove_if(BossCheck());*/
            targets.clear();
        }

        void Register() override
        {
            OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_garalon_broken_leg_SpellScript::FilterTargets, EFFECT_0, TARGET_UNIT_SRC_AREA_ENTRY);
            OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_garalon_broken_leg_SpellScript::FilterTargets, EFFECT_1, TARGET_UNIT_NEARBY_ENTRY);
        }
    };

    SpellScript* GetSpellScript() const override
    {
        return new spell_garalon_broken_leg_SpellScript();
    }
};

// Damaged: 123818
class spell_garalon_damaged : public SpellScriptLoader
{
public:
    spell_garalon_damaged() : SpellScriptLoader("spell_garalon_damaged") { }

    class spell_garalon_damaged_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_garalon_damaged_SpellScript);

        bool Validate(SpellInfo const* spellEntry) override
        {
            if (!sSpellMgr->GetSpellInfo(SPELL_DAMAGED))
                return false;
            return true;
        }

        bool Load() override
        {
            return true;
        }

        void HandleScript(SpellEffIndex /*effIndex*/)
        {
            if (!GetCaster() || !GetHitUnit())
                return;

            // He becomes immune to the Pheromones Taunt / Attack Me spell.
            GetCaster()->ApplySpellImmune(0, IMMUNITY_ID, SPELL_PHEROMONES_TAUNT, true);
        }

        void Register() override
        {
            OnEffectHitTarget += SpellEffectFn(spell_garalon_damaged_SpellScript::HandleScript, EFFECT_0, SPELL_EFFECT_APPLY_AURA);
        }
    };

    SpellScript* GetSpellScript() const override
    {
        return new spell_garalon_damaged_SpellScript();
    }
};

// Pheromones summon 128573
class spell_garalon_pheromones_summon : public SpellScriptLoader
{
public:
    spell_garalon_pheromones_summon() : SpellScriptLoader("spell_garalon_pheromones_summon") { }

    class spell_garalon_pheromones_summon_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_garalon_pheromones_summon_SpellScript);

        bool Validate(SpellInfo const* spellEntry) override
        {
            if (!sSpellMgr->GetSpellInfo(SPELL_SUMM_PHER_TRAIL))
                return false;
            return true;
        }

        bool Load() override
        {
            return true;
        }

        SpellCastResult CheckCast()
        {
            Unit* caster = GetCaster();
            if (!caster)
                return SPELL_CAST_OK;

            std::list<Creature*> pheromonesList;
            caster->GetCreatureListWithEntryInGrid(pheromonesList, NPC_PHEROMONE_TRAIL, 4.0f);

            if (!pheromonesList.empty())
                return SPELL_FAILED_DONT_REPORT;

            return SPELL_CAST_OK;
        }


        void Register() override
        {
            OnCheckCast += SpellCheckCastFn(spell_garalon_pheromones_summon_SpellScript::CheckCast);
        }
    };

    SpellScript* GetSpellScript() const override
    {
        return new spell_garalon_pheromones_summon_SpellScript();
    }
};

// Pheromone Trail Dmg 123120
class spell_garalon_pheromones_trail_dmg : public SpellScriptLoader
{
public:
    spell_garalon_pheromones_trail_dmg() : SpellScriptLoader("spell_garalon_pheromones_trail_dmg") { }

    class spell_garalon_pheromones_trail_dmg_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_garalon_pheromones_trail_dmg_SpellScript);

        bool Validate(SpellInfo const* spellEntry) override
        {
            if (!sSpellMgr->GetSpellInfo(SPELL_PHER_TRAIL_DMG))
                return false;
            return true;
        }

        bool Load() override
        {
            return true;
        }

        void HandleOnHit()
        {
            if (Unit* target = GetHitUnit())
            {
                if (Aura* aur = target->GetAura(SPELL_PUNGENCY))
                    SetHitDamage(int32(GetHitDamage() * (1.0f + float(aur->GetStackAmount() / 10.0f))));
            }
        }


        void Register() override
        {
            OnHit += SpellHitFn(spell_garalon_pheromones_trail_dmg_SpellScript::HandleOnHit);
        }
    };

    SpellScript* GetSpellScript() const override
    {
        return new spell_garalon_pheromones_trail_dmg_SpellScript();
    }
};

// Pheromones Switch 123100
class spell_garalon_pheromones_switch : public SpellScriptLoader
{
public:
    spell_garalon_pheromones_switch() : SpellScriptLoader("spell_garalon_pheromones_switch") { }

    class spell_garalon_pheromones_switch_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_garalon_pheromones_switch_SpellScript);

        bool Validate(SpellInfo const* spellEntry) override
        {
            if (!sSpellMgr->GetSpellInfo(SPELL_PHER_TRAIL_DMG))
                return false;
            return true;
        }

        bool Load() override
        {
            return true;
        }

        void HandleScript(SpellEffIndex /*effIndex*/)
        {
            if (!GetCaster() || !GetHitUnit())
                return;

            GetCaster()->RemoveAurasDueToSpell(SPELL_PHEROMONES_AURA);
            if (InstanceScript* pInstance = GetCaster()->GetInstanceScript())
            {
                if (Creature* garalon = GetCaster()->GetMap()->GetCreature(pInstance->GetData64((NPC_GARALON))))
                    garalon->AI()->DoAction(ACTION_PHEROMONES_JUMP_OR_PLAYERS_UNDERNEATH);
            }
        }

        void FillTargets(std::list<WorldObject*>& targets)
        {
            if (!targets.empty())
                targets.resize(1);
        }

        void Register() override
        {
            OnEffectHitTarget += SpellEffectFn(spell_garalon_pheromones_switch_SpellScript::HandleScript, EFFECT_0, SPELL_EFFECT_FORCE_CAST);
            OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_garalon_pheromones_switch_SpellScript::FillTargets, EFFECT_0, TARGET_UNIT_SRC_AREA_ALLY);
        }
    };

    SpellScript* GetSpellScript() const override
    {
        return new spell_garalon_pheromones_switch_SpellScript();
    }
};

// 123081 - Pungency
class spell_garalon_pungency : public SpellScriptLoader
{
public:
    spell_garalon_pungency() : SpellScriptLoader("spell_garalon_pungency") { }

    class spell_garalon_pungencyAuraScript : public AuraScript
    {
        PrepareAuraScript(spell_garalon_pungencyAuraScript);

        void Duration(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
        {
            if (Unit* target = GetTarget())
            {
                if (target->GetInstanceScript()->instance->IsHeroic())
                    SetDuration(240000);
            }
        }

        void Register() override
        {
            OnEffectApply += AuraEffectApplyFn(spell_garalon_pungencyAuraScript::Duration, EFFECT_0, SPELL_AURA_MOD_DAMAGE_DONE_FOR_MECHANIC, AURA_EFFECT_HANDLE_REAL);
        }
    };

    AuraScript* GetAuraScript() const override
    {
        return new spell_garalon_pungencyAuraScript();
    }
};

void AddSC_boss_garalon()
{
    new boss_garalon();
    new npc_garalon_body();
    new npc_garalon_leg();
    new npc_pheromone_trail();
    new spell_garalon_furious_swipe();          // 122735
    new spell_garalon_pheromones_forcecast();   // 123808
    new spell_script<spell_garalon_mend_leg_trigger>("spell_garalon_mend_leg_trigger");
    new spell_script<spell_garalon_mend_leg>("spell_garalon_mend_leg");
    new spell_garalon_crush_trigger();          // 117709
    new spell_garalon_pheromones_taunt();       // 123109
    new spell_garalon_broken_leg();             // 122786
    new spell_garalon_damaged();                // 123818
    new spell_garalon_pheromones_summon();      // 128573 INSERT INTO spell_script_names (spell_id, ScriptName) VALUES (128573, "spell_garalon_pheromones_summon");
    new spell_garalon_pheromones_trail_dmg();   // 123120 INSERT INTO spell_script_names (spell_id, ScriptName) VALUES (123120, "spell_garalon_pheromones_trail_dmg");
    new spell_garalon_pheromones_switch();      // 123100 INSERT INTO spell_script_names (spell_id, ScriptName) VALUES (123100, "spell_garalon_pheromones_switch");
    new spell_garalon_pungency();               // 123081
}
