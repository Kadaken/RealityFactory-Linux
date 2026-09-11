#ifndef RF_LEGACY_NPC_ENTITY_DATA_TYPES_H
#define RF_LEGACY_NPC_ENTITY_DATA_TYPES_H

/* Declarations recovered from GameEntityDataTypes.h at upstream 250b750. */
typedef struct _NonPlayerCharacter
{
	geBoolean alive;
	geBoolean active;
	geActor *Actor;
	geFloat Tick;
	geBoolean bState;
	geBoolean CallBack;
	void *DBot;
	geVec3d Location;
	geVec3d origin;
	char *szEntityName;
	char *AnimIdle;
	char *AnimWalk;
	char *AnimWalkAttack;
	char *AnimAttack;
	char *AnimJump;
	char *AnimDie;
	char *TriggerName;
	char *ActivateTriggerName;
	char *szActorName;
	geVec3d ActorRotation;
	geFloat Speed;
	geFloat Delay;
	geBoolean ReSpawn;
	int MaxNumber;
	geBoolean Gravity;
	geFloat Scale;
	int AttributeAmt;
	char *Attribute;
	geFloat DamageAmt;
	char *DamageAttribute;
	int Aggresiveness;
	geFloat DyingTime;
	geBoolean StopToAttack;
	geFloat AttackDelay;
	geBoolean Melee;
	char *Projectile;
	char *FireBone;
	geVec3d FireOffset;
	char *AttackSound;
	char *DieSound;
	int AimingSkill;
	geFloat ActorAlpha;
	geBoolean HideFromRadar;
	char *ChangeMaterial;
#pragma GE_DefaultValue(szEntityName, "")
#pragma GE_DefaultValue(szActorName, "")
#pragma GE_DefaultValue(ActorAlpha, "255")
#pragma GE_DefaultValue(HideFromRadar, "False")
#pragma GE_DefaultValue(ChangeMaterial, "")
#pragma GE_DefaultValue(AnimIdle, "Idle")
#pragma GE_DefaultValue(AnimWalkAttack, "Walk")
#pragma GE_DefaultValue(AnimAttack, "Shoot")
#pragma GE_DefaultValue(AnimWalk, "Walk")
#pragma GE_DefaultValue(AnimJump, "Jump")
#pragma GE_DefaultValue(AnimDie, "Die")
#pragma GE_DefaultValue(ActorRotation, "0 180 0.0")
#pragma GE_DefaultValue(Speed, "10.0")
#pragma GE_DefaultValue(DyingTime, "5.0")
#pragma GE_DefaultValue(AttackDelay, "2.0")
#pragma GE_DefaultValue(TriggerName, "")
#pragma GE_DefaultValue(ActivateTriggerName, "")
#pragma GE_DefaultValue(StopToAttack, "False")
#pragma GE_DefaultValue(Melee, "False")
#pragma GE_DefaultValue(Projectile, "")
#pragma GE_DefaultValue(FireBone, "")
#pragma GE_DefaultValue(FireOffset, "0 0 0")
#pragma GE_DefaultValue(Delay, "0.0")
#pragma GE_DefaultValue(ReSpawn, "False")
#pragma GE_DefaultValue(MaxNumber, "1")
#pragma GE_DefaultValue(Scale, "1.0")
#pragma GE_DefaultValue(Gravity, "True")
#pragma GE_DefaultValue(AttributeAmt, "100")
#pragma GE_DefaultValue(Attribute, "health")
#pragma GE_DefaultValue(DamageAmt, "10")
#pragma GE_DefaultValue(DamageAttribute, "health")
#pragma GE_DefaultValue(Aggresiveness, "5")
#pragma GE_DefaultValue(AttackSound, "")
#pragma GE_DefaultValue(DieSound, "")
#pragma GE_DefaultValue(AimingSkill, "5")
} NonPlayerCharacter;

typedef struct NPCPathPoint
{
	geVec3d origin;
	int PathType;
	int ActionType;
	geFloat Time;
	geFloat Dist;
	geFloat VelocityScale;
	NPCPathPoint *Next;
	geWorld_Model *MoveWithModel;
	int Direction;
	int ShootTimes;
	NPCPathPoint *WatchPoint;
	geVec3d Pos;
	geVec3d OriginOffset;
#pragma GE_DefaultValue(PathType, "-1")
#pragma GE_DefaultValue(ActionType, "-1")
#pragma GE_DefaultValue(Direction, "1")
} NPCPathPoint;

#endif
