#include "RabidFramework.h"
#include "entity_schema.h"
#include <climits>
#include <cstdio>
#include <cstddef>

struct RFEntitySize { const char *name; size_t bytes; };
#include "entity_sizes.generated.h"
struct RFEntitySchema { const char *name; const geEntity_FieldDecl *fields; int count; };
#include "entity_schema.generated.h"

geEntity_EntitySet *RF_DeclareEntitySchemas()
{
    geEntity_EntitySet *schema=geEntity_EntitySetCreate();
    if (!schema) return NULL;
    for (size_t i=0;i<sizeof(rf_entity_schemas)/sizeof(rf_entity_schemas[0]);++i) {
        const RFEntitySchema &entry=rf_entity_schemas[i];
        if (!geEntity_EntitySetDeclareClass(schema,entry.name,entry.fields,entry.count)) {
            std::fprintf(stderr,"schema: %s declaration failed; refusing level startup\n",entry.name);
            geEntity_EntitySetDestroy(schema);
            return NULL;
        }
    }
    return schema;
}

bool RF_ReserveEntitySchemas(geWorld *world)
{
    if (!world) return false;
    geEntity_EntitySet *all = NULL;
    for (size_t i=0; i<sizeof(rf_entity_sizes)/sizeof(rf_entity_sizes[0]); ++i) {
        const RFEntitySize &entry = rf_entity_sizes[i];
        /* Empty worlds have no owning set; class lookup is safe in that case.
           Classes with no instances have no userdata to reserve. */
        if (!geWorld_GetEntitySet(world, entry.name)) continue;
        if (!all) all = geWorld_GetEntitySet(world, NULL);
        int declared = 0;
        if (entry.bytes > INT_MAX ||
            !geEntity_EntitySetGetClassFieldSize(all, entry.name, &declared) ||
            !geEntity_EntitySetReserveUserData(all, entry.name, (int)entry.bytes)) {
            std::fprintf(stderr, "schema: %s reservation failed; refusing level startup\n", entry.name);
            return false;
        }
        if ((size_t)declared < entry.bytes)
            std::fprintf(stderr, "schema: %s declared=%d runtime=%zu tail-padding=%zu\n",
                entry.name, declared, entry.bytes, entry.bytes-(size_t)declared);
    }
    return true;
}
