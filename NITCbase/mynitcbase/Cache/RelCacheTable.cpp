#include "RelCacheTable.h"

#include <cstring>

RelCacheEntry *RelCacheTable::relCache[MAX_OPEN];

/* Gets relational catalog entry for a relation with rel-id from relational cache*/
int RelCacheTable::getRelCatEntry(int relId, RelCatEntry *relCatBuf) {
    // check bounds
    if(relId < 0 || relId > MAX_OPEN) {
        return E_OUTOFBOUND;
    }

    // if there is not entry at the rel-id
    if(relCache[relId] == nullptr) {
        return E_RELNOTOPEN;
    }

    // else the entry is valid
    *relCatBuf = relCache[relId]->relCatEntry;

    return SUCCESS;
}

/* */

void RelCacheTable::recordToRelCatEntry(union Attribute record[RELCAT_NO_ATTRS], RelCatEntry* relCatEntry) {
    strcpy(relCatEntry->relName, record[RELCAT_REL_NAME_INDEX].sVal);
    // do typecasting from double to int
    relCatEntry->numAttrs = (int)record[RELCAT_NO_ATTRIBUTES_INDEX].nVal;
    relCatEntry->numRecs = (int)record[RELCAT_NO_RECORDS_INDEX].nVal;
    relCatEntry->firstBlk = (int)record[RELCAT_FIRST_BLOCK_INDEX].nVal;
    relCatEntry->lastBlk = (int)record[RELCAT_LAST_BLOCK_INDEX].nVal;
    relCatEntry->numSlotsPerBlk = (int)record[RELCAT_NO_SLOTS_PER_BLOCK_INDEX].nVal;
}
