#include "BlockAccess.h"

#include <cstring>

RecId BlockAccess::linearSearch(int relId, char attrName[ATTR_SIZE], union Attribute attrVal, int op) {
    // get the searchIndex from relational cache table - this contains the position where last search ended
    RecId prevRecID;
    RecId invalid = {-1, -1};
    int ret = RelCacheTable::getSearchIndex(relId, &prevRecID);
    if(ret != SUCCESS) {
        return invalid;
    }
    int block, slot;
    // if it is fresh search
    if(prevRecID.block == -1 && prevRecID.slot == -1) {
        RelCatEntry relCatBuf;
        ret = RelCacheTable::getRelCatEntry(relId, &relCatBuf);
        if(ret != SUCCESS) {
            return invalid;
        }
        block = relCatBuf.firstBlk;
        slot = 0;
    }
    // if there is a hit in previous search
    else {
        block = prevRecID.block;
        slot = prevRecID.slot + 1;
    }

    AttrCatEntry attrCatEntry;
    ret = AttrCacheTable::getAttrCatEntry(relId, attrName, &attrCatEntry);
    if(ret != SUCCESS) {
        return invalid;
    }

    int offset = attrCatEntry.offset;
    int type = attrCatEntry.attrType;

    // iterate over all blocks of this relation(start from prev-search index if exists)
    while(block != -1) {

        
        // create a recbuffer object
        RecBuffer recBuffer(block);

        // get header of the block
        HeadInfo head;
        ret = recBuffer.getHeader(&head);
        if(ret != SUCCESS){
            return invalid;
        }

        // Handle case where slot was advanced past the last slot of prev block
        if (slot >= head.numSlots) {
            block = head.rblock;
            slot = 0;
            continue;
        }
        
        // get slotmap of the block
        unsigned char *slotMap = new unsigned char[head.numSlots];
        ret = recBuffer.getSlotMap(slotMap);
        if(ret != SUCCESS) {
            delete[] slotMap;
            return invalid;
        }

        while(slot < head.numSlots) {
            // check if the slot is valid
            if(slotMap[slot]==SLOT_UNOCCUPIED) {
                slot++;
                continue;
            }

            // if the slot is occupied, get that record from buffer
            Attribute record[head.numAttrs];
            ret = recBuffer.getRecord(record, slot);
            if(ret != SUCCESS) {
                delete[] slotMap;
                return invalid;
            }

            // compare using function and decide based on the argument operator
            int cmpVal = compareAttrs(record[offset], attrVal, type);
            if (
                (op == EQ && cmpVal == 0) ||
                (op == NE && cmpVal != 0) ||
                (op == LT && cmpVal < 0)  ||
                (op == LE && cmpVal <= 0) ||
                (op == GT && cmpVal > 0)  ||
                (op == GE && cmpVal >= 0)
            ) {
                RecId searchIndex = {block, slot};
                RelCacheTable::setSearchIndex(relId,&searchIndex);
                delete[] slotMap;
                return RecId{block, slot};
            }
            slot++;
        }
        delete[] slotMap;
        block = head.rblock;
        slot = 0;
    }

    // if nothing is found
    RelCacheTable::setSearchIndex(relId, &invalid);
    return invalid;

}