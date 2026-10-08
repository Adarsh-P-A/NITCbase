#include "RelCacheTable.h"

#include <cstring>

RelCacheEntry *RelCacheTable::relCache[MAX_OPEN];

/* Gets relational catalog entry for a relation with rel-id from relational cache*/
int RelCacheTable::getRelCatEntry(int relId, RelCatEntry *relCatBuf) {
    // check bounds
    if(relId < 0 || relId >= MAX_OPEN) {
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


void RelCacheTable::recordToRelCatEntry(union Attribute record[RELCAT_NO_ATTRS], RelCatEntry* relCatEntry) {
    strcpy(relCatEntry->relName, record[RELCAT_REL_NAME_INDEX].sVal);
    // do typecasting from double to int
    relCatEntry->numAttrs = (int)record[RELCAT_NO_ATTRIBUTES_INDEX].nVal;
    relCatEntry->numRecs = (int)record[RELCAT_NO_RECORDS_INDEX].nVal;
    relCatEntry->firstBlk = (int)record[RELCAT_FIRST_BLOCK_INDEX].nVal;
    relCatEntry->lastBlk = (int)record[RELCAT_LAST_BLOCK_INDEX].nVal;
    relCatEntry->numSlotsPerBlk = (int)record[RELCAT_NO_SLOTS_PER_BLOCK_INDEX].nVal;
}

// NOTE: RecId is a structure with block and slot numbers
int RelCacheTable::getSearchIndex(int relId, RecId *searchIndex) {
    if(relId < 0 || relId >= MAX_OPEN) {
        return E_OUTOFBOUND;
    }

    if(relCache[relId]==nullptr) {
        return E_RELNOTOPEN;
    }

    *searchIndex = relCache[relId]->searchIndex;
    return SUCCESS;
}

// sets the searchIndex for the relation corresponding to relId
int RelCacheTable::setSearchIndex(int relId, RecId *searchIndex) {
    if(relId < 0 || relId >= MAX_OPEN) {
        return E_OUTOFBOUND;
    }

    if(relCache[relId]==nullptr) {
        return E_RELNOTOPEN;
    }

    relCache[relId]->searchIndex = *searchIndex;
    return SUCCESS;
}

// use setSearchIndex to set the search index to {-1, -1}
int RelCacheTable::resetSearchIndex(int relId) {
    RecId searchIndex = {-1, -1};
    int ret =  RelCacheTable::setSearchIndex(relId, &searchIndex);
    return ret;
}

int RelCacheTable::setRelCatEntry(int relId, RelCatEntry *relCatBuf) {
    if (relId < 0 || relId >= MAX_OPEN)
        return E_OUTOFBOUND;

    // if there's no entry at the rel-id
    if (relCache[relId] == nullptr)
        return E_RELNOTOPEN;

    relCache[relId]->relCatEntry = *relCatBuf;

    // set the dirty flag of the corresponding Relation Cache entry
    relCache[relId]->dirty = true;

    return SUCCESS;
}


void RelCacheTable::relCatEntryToRecord(RelCatEntry *relCatEntry, union Attribute record[RELCAT_NO_ATTRS]) {
    // reverse of RelCacheTable::recordToRelCatEntry, object to array
    strcpy(record[RELCAT_REL_NAME_INDEX].sVal, relCatEntry->relName);

    record[RELCAT_NO_ATTRIBUTES_INDEX].nVal = relCatEntry->numAttrs;
    record[RELCAT_NO_RECORDS_INDEX].nVal = relCatEntry->numRecs;
    record[RELCAT_FIRST_BLOCK_INDEX].nVal = relCatEntry->firstBlk;
    record[RELCAT_LAST_BLOCK_INDEX].nVal = relCatEntry->lastBlk;
    record[RELCAT_NO_SLOTS_PER_BLOCK_INDEX].nVal = relCatEntry->numSlotsPerBlk;
}