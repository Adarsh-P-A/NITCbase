#include "Algebra.h"

#include <stdio.h>
#include <cstring>
#include <cstdlib>


bool isNumber(char *str) {
    int len;
    float ignore;
    int ret = sscanf(str, "%f %n", &ignore, &len);
    return ret == 1 && len == strlen(str);
    // f reads the actual floating point number and %n reads number of characters read into len
    // 123.01 -> len will become 6 and it matches strlen
    // 123.01abc -> len will become 6 and it doesnt match strlen - cause %f can read only float values
}


/* used to select all the records that satisfy a condition.
the arguments of the function are
- srcRel - the source relation we want to select from
- targetRel - the relation we want to select into. (ignore for now)
- attr - the attribute that the condition is checking
- op - the operator of the condition
- strVal - the value that we want to compare against (represented as a string)
*/

int Algebra::select(char srcRel[ATTR_SIZE], char targetRel[ATTR_SIZE], char attr[ATTR_SIZE], int op, char strVal[ATTR_SIZE]) {
    int srcRelId = OpenRelTable::getRelId(srcRel);
    if(srcRelId == E_RELNOTOPEN) {
        return E_RELNOTOPEN;
    }

    // get attr cat entry
    AttrCatEntry attrCatEntry;
    int ret = AttrCacheTable::getAttrCatEntry(srcRelId, attr, &attrCatEntry);
    if(ret != SUCCESS) {
        return E_ATTRNOTEXIST;
    }

     /*** Convert strVal (string) to an attribute of data type NUMBER or STRING ***/
     int type = attrCatEntry.attrType;
     Attribute attrVal;
     if(type == NUMBER) {
        if(isNumber(strVal)) {
            attrVal.nVal = atof(strVal);
        }
        else {
            return E_ATTRTYPEMISMATCH;
        }
     }
     else if(type == STRING) {
        strcpy(attrVal.sVal, strVal);
     }

    /*** Selecting records from the source relation ***/

    // Before calling the search function, reset the search to start from the first hit
    // using RelCacheTable::resetSearchIndex()
    RelCacheTable::resetSearchIndex(srcRelId);

    RelCatEntry relCatEntry;
    ret = RelCacheTable::getRelCatEntry(srcRelId, &relCatEntry);
    if(ret != SUCCESS) {
        return ret;
    }

    /************************
     The following code prints the contents of a relation directly to the output
    console. Direct console output is not permitted by the actual the NITCbase
    specification and the output can only be inserted into a new relation. We will
    be modifying it in the later stages to match the specification.
    ************************/

    printf("|");
    for(int i=0; i < relCatEntry.numAttrs; i++) {
        AttrCatEntry attrCatEntry;
        ret = AttrCacheTable::getAttrCatEntry(srcRelId, i, &attrCatEntry);
        if( ret != SUCCESS) {
            return ret;
        }
        printf(" %s |", attrCatEntry.attrName);
    }
    printf("\n");

    while(true) {
        RecId searchRes = BlockAccess::linearSearch(srcRelId, attr, attrVal, op);
        if (searchRes.block != -1 && searchRes.slot != -1) {
            RecBuffer buffer(searchRes.block);
            Attribute rec[relCatEntry.numAttrs];
            ret = buffer.getRecord(rec, searchRes.slot);
            if(ret != SUCCESS) {
                return ret;
            }
            printf("|");
            for(int i=0; i < relCatEntry.numAttrs; i++) {
                AttrCatEntry attrCatEntry;
                ret = AttrCacheTable::getAttrCatEntry(srcRelId, i, &attrCatEntry);
                if(attrCatEntry.attrType == NUMBER) {
                    printf(" %.2f |", rec[i].nVal);
                }
                else if(attrCatEntry.attrType == STRING) {
                    printf(" %s |", rec[i].sVal);
                }
            }
            printf("\n");
        }
        else {
            break;
        }
    }
    return SUCCESS;

}

int Algebra::insert(char relName[ATTR_SIZE], int nAttrs, char record[][ATTR_SIZE]) {
    if (!strcmp(relName, RELCAT_RELNAME) || !strcmp(relName, ATTRCAT_RELNAME))
        return E_NOTPERMITTED;

    int relId = OpenRelTable::getRelId(relName);
    if (relId == E_RELNOTOPEN)
        return relId;

    RelCatEntry relCatBuf;
    RelCacheTable::getRelCatEntry(relId, &relCatBuf);

    // If no. of attributes mismatch, throw error
    if (relCatBuf.numAttrs != nAttrs)
        return E_NATTRMISMATCH;

    Attribute recordValues[nAttrs];

    for (int i = 0; i < nAttrs; i++) {
        // get the attrCat entry for the i'th attribute from the attrCache
        AttrCatEntry attrCatBuf;
        AttrCacheTable::getAttrCatEntry(relId, i, &attrCatBuf);

        int type = attrCatBuf.attrType;

        if (type == NUMBER) {
            if (isNumber(record[i])) {
                recordValues[i].nVal = atof(record[i]);
            } else {
                return E_ATTRTYPEMISMATCH;
            }
        } else if (type == STRING) {
            strcpy(recordValues[i].sVal, record[i]);
        }
    }

    // insert the record by calling BlockAccess::insert()
    int ret = BlockAccess::insert(relId, recordValues);
    return ret;
}

int RecBuffer::setSlotMap(unsigned char *slotMap) {
    unsigned char *bufferPtr;

    int ret = BlockBuffer::loadBlockAndGetBufferPtr(&bufferPtr); // Load block into buffer if not present already
    if (ret != SUCCESS)
        return ret;

    // Get the block header
    struct HeadInfo head;
    BlockBuffer::getHeader(&head);

    // Copy the contents of the argument `slotMap` to the buffer replacing the existing slotmap
    int numSlots = head.numSlots;
    memcpy(bufferPtr + HEADER_SIZE, slotMap, numSlots);

    // update dirty bit in StaticBuffer
    ret = StaticBuffer::setDirtyBit(this->blockNum);
    if (ret != SUCCESS)
        return ret;

    return SUCCESS;
}

// This will be used when a new block is assigned from Constructor 2 using block type.
// The new blockNum is unknown. So, call this method to get the new blockNum.
int BlockBuffer::getBlockNum() { return this->blockNum; }