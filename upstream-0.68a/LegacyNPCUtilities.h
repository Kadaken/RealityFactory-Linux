#ifndef RF_LEGACY_NPC_UTILITIES_H
#define RF_LEGACY_NPC_UTILITIES_H

#ifndef M_PI
#define M_PI (3.14159f)
#endif
#define PI_2 (M_PI * 2.0f)

typedef struct Stack
{
	int32 TOS, Size, *Data;
} Stack;

void StackInit(Stack *s);
void StackReset(Stack *s);
void StackPush(Stack *s, int32 data);
int32 StackPop(Stack *s);
int32 StackTop(Stack *s);
geBoolean StackIsEmpty(Stack *s);
int32 RandomRange(int32 range);
float DistWeightedY(const geVec3d *Pos1, const geVec3d *Pos2, float Scale);
void VectorRotateY(geVec3d *vec, float delta_ang, geVec3d *result);

#endif

