#include "Schema.h"

#include <cmath>
#include <cstring>

int Schema::openRel(char relName[ATTR_SIZE]) {
    int ret = OpenRelTable::openRel(relName);
    if(ret >= 0) {
        return SUCCESS;
    }
    return ret;
    // OpenRelTable::openrel returns relid which is positive is successful, otherwise a negative error value
}

int Schema::closeRel(char relName[ATTR_SIZE]) {
    // rel cat and attr cat should not be closed
    if(strcmp(relName, RELCAT_RELNAME)==0 || strcmp(relName, ATTRCAT_RELNAME)==0) {
        return E_NOTPERMITTED;
    }

    // get the relId of relation
    // this call returns relId if relation is open, else an error E_RELNOTOPEN
    int relId = OpenRelTable::getRelId(relName);

    if(relId==E_RELNOTOPEN) {
        return E_RELNOTOPEN;
    }

    return OpenRelTable::closeRel(relId);
}

int Schema::renameRel(char oldRelName[ATTR_SIZE], char newRelName[ATTR_SIZE]) {
    if(
            !strcmp(oldRelName, RELCAT_RELNAME) || 
            !strcmp(oldRelName, ATTRCAT_RELNAME) ||
            !strcmp(newRelName, RELCAT_RELNAME) ||
            !strcmp(newRelName, ATTRCAT_RELNAME)
    ) {
        return E_NOTPERMITTED;
    }

    int ret = OpenRelTable::getRelId(oldRelName);

    if(ret != E_RELNOTOPEN) {
        return E_RELOPEN;
    }

    int retVal = BlockAccess::renameRelation(oldRelName, newRelName);
    return retVal;
}

int Schema::renameAttr(char relName[ATTR_SIZE], char oldAttrName[ATTR_SIZE], char newAttrName[ATTR_SIZE]) {
        if(
            !strcmp(relName, RELCAT_RELNAME) || 
            !strcmp(relName, ATTRCAT_RELNAME)
    ) {
        return E_NOTPERMITTED;
    }

    int ret = OpenRelTable::getRelId(relName);

    if(ret != E_RELNOTOPEN) {
        return E_RELOPEN;
    }

    int retVal = BlockAccess::renameAttribute(relName, oldAttrName, newAttrName);
    return retVal;
}