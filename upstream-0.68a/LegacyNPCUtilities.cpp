#include "RabidFramework.h"

void StackInit(Stack *s)
{
	s->TOS = -1;
	s->Data = GE_RAM_ALLOCATE_ARRAY(int32, MAX_TRACKS);
	assert(s->Data);
	s->Size = MAX_TRACKS;
}

void StackReset(Stack *s) { s->TOS = -1; }

void StackPush(Stack *s, int32 data)
{
	s->TOS++;
	assert(s->TOS < s->Size);
	s->Data[s->TOS] = data;
}

int32 StackPop(Stack *s)
{
	int32 value;
	if(s->TOS <= -1)
		return (s->TOS = -1);
	value = s->Data[s->TOS];
	s->TOS--;
	return value;
}

int32 StackTop(Stack *s)
{
	if(s->TOS <= -1)
		return -1;
	return s->Data[s->TOS];
}

geBoolean StackIsEmpty(Stack *s) { return (s->TOS <= -1); }

int32 RandomRange(int32 range)
{
	return (int)EffectC_Frand(0.0f, (float)(range - 1));
}

float DistWeightedY(const geVec3d *Pos1, const geVec3d *Pos2, float Scale)
{
	geVec3d LPos1 = *Pos1;
	geVec3d LPos2 = *Pos2;
	LPos1.Y *= Scale;
	LPos2.Y *= Scale;
	return geVec3d_DistanceBetween(&LPos1, &LPos2);
}

void VectorRotateY(geVec3d *vec, float delta_ang, geVec3d *result)
{
	geXForm3d XForm;
	geXForm3d_SetIdentity(&XForm);
	geXForm3d_RotateY(&XForm, delta_ang);
	geXForm3d_Rotate(&XForm, vec, result);
}

