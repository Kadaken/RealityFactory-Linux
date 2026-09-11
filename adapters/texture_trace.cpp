#include <Genesis.h>

#include <string.h>

void getSingleTextureNameByTrace(geWorld *world, const geVec3d *start,
	const geVec3d *end, char *texture_name)
{
	GE_Collision collision;
	if(texture_name == NULL)
		return;
	texture_name[0] = '\0';
	if(world == NULL || start == NULL || end == NULL)
		return;

	memset(&collision, 0, sizeof(collision));
	if(geWorld_Collision(world, NULL, NULL, start, end,
		GE_CONTENTS_SOLID_CLIP | GE_CONTENTS_WINDOW, GE_COLLIDE_MODELS,
		0, NULL, NULL, &collision) == GE_FALSE)
		return;

	if(geWorld_GetTextureNameFromCollision(world, &collision,
		texture_name) == GE_FALSE)
		texture_name[0] = '\0';
}
