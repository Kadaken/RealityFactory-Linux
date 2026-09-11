#include <Genesis.h>

#include <string.h>

void getSingleTextureNameByTrace(geWorld *world, const geVec3d *start,
	const geVec3d *end, char *texture_name);

namespace
{
const geVec3d ExpectedImpact = {10.0f, 20.0f, 30.0f};
bool CollisionCalled = false;
bool TextureLookupCalled = false;
}

extern "C" geBoolean geWorld_Collision(geWorld *, const geVec3d *,
	const geVec3d *, const geVec3d *, const geVec3d *, uint32, uint32,
	uint32, GE_CollisionCB *, void *, GE_Collision *collision)
{
	CollisionCalled = true;
	collision->Impact = ExpectedImpact;
	collision->Plane.Normal.Y = 1.0f;
	collision->Plane.Dist = 20.0f;
	collision->Model = reinterpret_cast<geWorld_Model *>(2);
	return GE_TRUE;
}

extern "C" geBoolean geWorld_GetTextureNameFromCollision(geWorld *,
	const GE_Collision *collision, char *texture_name)
{
	TextureLookupCalled = collision->Impact.X == ExpectedImpact.X &&
		collision->Impact.Y == ExpectedImpact.Y &&
		collision->Impact.Z == ExpectedImpact.Z &&
		collision->Plane.Normal.Y == 1.0f &&
		collision->Plane.Dist == 20.0f &&
		collision->Model == reinterpret_cast<geWorld_Model *>(2);
	strcpy(texture_name, "NEUTRAL_FLOOR");
	return GE_TRUE;
}

int main()
{
	geVec3d start = {0.0f, 100.0f, 0.0f};
	geVec3d end = {0.0f, -100.0f, 0.0f};
	char texture_name[32];

	getSingleTextureNameByTrace(reinterpret_cast<geWorld *>(1), &start, &end,
		texture_name);
	if(!CollisionCalled || !TextureLookupCalled)
		return 1;
	if(strcmp(texture_name, "NEUTRAL_FLOOR") != 0)
		return 2;
	return 0;
}
