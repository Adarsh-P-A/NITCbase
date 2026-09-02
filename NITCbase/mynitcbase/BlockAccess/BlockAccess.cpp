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

int BlockAccess::renameRelation(char oldName[ATTRCAT_BLOCK], char newName[ATTR_SIZE]) {

    RelCacheTable::resetSearchIndex(RELCAT_RELID);

    Attribute newRelName;
    strcpy(newRelName.sVal, newName);
    char relName[ATTR_SIZE];
    strcpy(relName, RELCAT_ATTR_RELNAME);
    RecId recId = BlockAccess::linearSearch(RELCAT_RELID, relName, newRelName, EQ);

    // what if the newName already exists
    if(recId.block != -1 && recId.slot != -1) {
        return E_RELEXIST;
    }

    /**********Update new name in relation catalog entries**********/

    RelCacheTable::resetSearchIndex(RELCAT_RELID);
    Attribute oldRelName;
    strcpy(oldRelName.sVal, oldName);
    recId = BlockAccess::linearSearch(RELCAT_RELID, relName, oldRelName, EQ);

    // what is the oldName never existed
    if(recId.block == -1 && recId.slot == -1) {
        return E_RELNOTEXIST;
    }

    // get the record of this relation in relation catalog
    RecBuffer buffer(recId.block);
    Attribute rec[RELCAT_NO_ATTRS];
    buffer.getRecord(rec, recId.slot);

    // rename the relation
    strcpy(rec[RELCAT_REL_NAME_INDEX].sVal, newName);
    buffer.setRecord(rec, recId.slot);

    /************Update new name in attribute catalog entries of this relation***********/
    RelCacheTable::resetSearchIndex(ATTRCAT_RELID);
    strcpy(relName, ATTRCAT_ATTR_RELNAME);

    while(true) {
        recId = BlockAccess::linearSearch(ATTRCAT_RELID, relName, oldRelName, EQ);
        if(recId.block == -1 && recId.slot == -1) {
            break;
        }

        // change the name of relation in each record of this 
        RecBuffer attrCatBuffer(recId.block);
        Attribute attrRec[ATTRCAT_NO_ATTRS];
        attrCatBuffer.getRecord(attrRec, recId.slot);
        
        strcpy(attrRec[ATTRCAT_REL_NAME_INDEX].sVal, newName);

        attrCatBuffer.setRecord(attrRec, recId.slot);
    }

    return SUCCESS;
    
}   

int BlockAccess::renameAttribute(char relName[ATTR_SIZE], char oldName[ATTR_SIZE], char newName[ATTR_SIZE]) {

    // reset search index before a fresh search
    RelCacheTable::resetSearchIndex(RELCAT_RELID);

    Attribute rel;
    strcpy(rel.sVal, relName);
    char relCatRelName[ATTR_SIZE];
    strcpy(relCatRelName, RELCAT_ATTR_RELNAME);

    // search to see if such a relation exists
    RecId recId = BlockAccess::linearSearch(RELCAT_RELID, relCatRelName, rel, EQ);

    if(recId.block == -1 && recId.slot == -1) {
        return E_RELNOTEXIST;
    }

    RelCacheTable::resetSearchIndex(ATTRCAT_RELID);

    /* declare variable attrToRenameRecId used to store the attr-cat recId
    of the attribute to rename */
    RecId attrToRenameRecId{-1, -1};
    Attribute attrCatEntryRecord[ATTRCAT_NO_ATTRS];
    char attrCatRelName[ATTR_SIZE];
    strcpy(attrCatRelName, ATTRCAT_ATTR_RELNAME);
    Attribute attribute;
    strcpy(attribute.sVal, relName);

    /* iterate over all Attribute Catalog Entry record corresponding to the relation to find the required attribute */
    while(true) {

        RecId attrRecId = BlockAccess::linearSearch(ATTRCAT_RELID, attrCatRelName, attribute, EQ);
        if(attrRecId.block == -1 && attrRecId.slot == -1) {
            break;
        }

        RecBuffer buffer(attrRecId.block);
        buffer.getRecord(attrCatEntryRecord, attrRecId.slot);

        // if an attribute with new name already exists
        if(strcmp(attrCatEntryRecord[ATTRCAT_ATTR_NAME_INDEX].sVal, newName)==0) {
            return E_ATTREXIST;
        }

        // collect the block number and slot number of the target attribute
        if(strcmp(attrCatEntryRecord[ATTRCAT_ATTR_NAME_INDEX].sVal, oldName) == 0){
            attrToRenameRecId = attrRecId;
        }
    }

    if(attrToRenameRecId.block == -1 && attrToRenameRecId.slot == -1) {
        return E_ATTRNOTEXIST;
    }

    // load the block into buffer and get the record
    RecBuffer attrCatBuffer(attrToRenameRecId.block);
    attrCatBuffer.getRecord(attrCatEntryRecord, attrToRenameRecId.slot);
    
    // Rename the attribute
    strcpy(attrCatEntryRecord[ATTRCAT_ATTR_NAME_INDEX].sVal, newName);

    // write it back
    attrCatBuffer.setRecord(attrCatEntryRecord, attrToRenameRecId.slot);

    return SUCCESS;

}