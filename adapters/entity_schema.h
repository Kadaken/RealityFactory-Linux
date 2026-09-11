#ifndef RF_ENTITY_SCHEMA_H
#define RF_ENTITY_SCHEMA_H
#include "Genesis.h"
/* Caller destroys the copied declaration container after world construction. */
geEntity_EntitySet *RF_DeclareEntitySchemas();
/* Must run before exposing any world entity userdata to runtime managers. */
bool RF_ReserveEntitySchemas(geWorld *world);
#endif
