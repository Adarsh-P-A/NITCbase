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