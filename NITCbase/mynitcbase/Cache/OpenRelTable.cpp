#include "OpenRelTable.h"
#include <iostream>
#include <cstring>
#include <stdlib.h>

OpenRelTable::OpenRelTable() {

    // initialise everything to null
    for(int i=0; i < MAX_OPEN; i++) {
        RelCacheTable::relCache[i] = nullptr;
        AttrCacheTable::attrCache[i] = nullptr;
    }


    /************ Setting up Relation Cache entries ************/
    // (we need to populate relation cache with entries for the relation catalog and attribute catalog)

    /**** setting up Relation Catalog relation in the Relation Cache Table****/

    RecBuffer relCatBlock(RELCAT_BLOCK);
    Attribute relCatRecord[RELCAT_NO_ATTRS];
    relCatBlock.getRecord(relCatRecord, RELCAT_SLOTNUM_FOR_RELCAT);

    // Use new instead of malloc
    RelCacheEntry *relCacheEntry  = new RelCacheEntry;
    RelCacheTable::recordToRelCatEntry(relCatRecord, &relCacheEntry->relCatEntry);
    // additional metadata
    relCacheEntry->recId.block = RELCAT_BLOCK;
    relCacheEntry->recId.slot = RELCAT_SLOTNUM_FOR_RELCAT;
    RelCacheTable::relCache[RELCAT_RELID] = relCacheEntry;


    /**** setting up Attribute Catalog relation in the Relation Cache Table ****/
    relCatBlock.getRecord(relCatRecord, RELCAT_SLOTNUM_FOR_ATTRCAT);

    RelCacheEntry *relCacheEntry2 = new RelCacheEntry;
    RelCacheTable::recordToRelCatEntry(relCatRecord, &relCacheEntry2->relCatEntry);
    // additional metadata
    relCacheEntry2->recId.block = RELCAT_BLOCK;
    relCacheEntry2->recId.slot = RELCAT_SLOTNUM_FOR_ATTRCAT;
    RelCacheTable::relCache[ATTRCAT_RELID] = relCacheEntry2;

    /**** setting up Student Catalog relation in the Relation Cache Table ****/
    for(int i=0; i<MAX_OPEN; i++) {
        relCatBlock.getRecord(relCatRecord, i);
        if(!strcmp(relCatRecord[RELCAT_REL_NAME_INDEX].sVal, "Students")) {
            RelCacheEntry *relCacheEntry3 = new RelCacheEntry;
            RelCacheTable::recordToRelCatEntry(relCatRecord, &relCacheEntry3->relCatEntry);
            // additional metadata
            relCacheEntry3->recId.block = RELCAT_BLOCK;
            relCacheEntry2->recId.slot = i;
            RelCacheTable::relCache[2] = relCacheEntry3;
            printf("\nPRINTING FROM CACHE\n");
            printf("\nRelation catalog entry of Student Relation\n");
            printf("-----------------------------------------\n");
            printf("relName: %s\n", relCacheEntry3->relCatEntry.relName);
            printf("numAttrs: %d\n", relCacheEntry3->relCatEntry.numAttrs);
            printf("firstBlk: %d\n", relCacheEntry3->relCatEntry.firstBlk);
            printf("lastBlk: %d\n", relCacheEntry3->relCatEntry.lastBlk);
            printf("numSlotsperBlk: %d\n\n", relCacheEntry3->relCatEntry.numSlotsPerBlk);
            break;
        }
    }

    /************ Setting up Attribute cache entries ************/
    // (we need to populate attribute cache with entries for the relation catalog
    //  and attribute catalog.)

    /**** setting up Relation Catalog relation in the Attribute Cache Table ****/
    RecBuffer attrCatBlock(ATTRCAT_BLOCK);
    Attribute attrCatRecord[ATTRCAT_NO_ATTRS];
    AttrCacheEntry *head = nullptr;
    AttrCacheEntry *tail = nullptr;
    // first 6 entries are attribtues of relation catalog. make it a linked list and add its head to attrCache
    for(int i=0; i < RELCAT_NO_ATTRS; i++) {
        attrCatBlock.getRecord(attrCatRecord, i);
        AttrCacheEntry *curr = new AttrCacheEntry;
        AttrCacheTable::recordToAttrCatEntry(attrCatRecord, &curr->attrCatEntry);
        curr->recId.block = ATTRCAT_BLOCK;
        curr->recId.slot = i;
        curr->next = nullptr;

        if(head==nullptr) {
            head = curr;
            tail = curr;
        } else {
            tail->next = curr;
            tail = tail->next;
        }

    }

    AttrCacheTable::attrCache[RELCAT_RELID] = head;

    /**** setting up Attribute Catalog relation in the Attribute Cache Table ****/
    head = nullptr;
    tail = nullptr;
    for(int i=RELCAT_NO_ATTRS; i < RELCAT_NO_ATTRS + ATTRCAT_NO_ATTRS; i++) {
        attrCatBlock.getRecord(attrCatRecord, i);
        AttrCacheEntry *curr = new AttrCacheEntry;
        AttrCacheTable::recordToAttrCatEntry(attrCatRecord, &curr->attrCatEntry);
        curr->recId.block = ATTRCAT_BLOCK;
        curr->recId.slot = i;
        curr->next = nullptr;

        if(head==nullptr) {
            head = curr;
            tail =curr;
        } else {
            tail->next = curr;
            tail = tail->next;
        }
    }
    
    AttrCacheTable::attrCache[ATTRCAT_RELID] = head;

    /**** setting up Student Catalog relation in the Attribute Cache Table ****/
    head = nullptr;
    tail = nullptr;
    HeadInfo attrCatheader;
    attrCatBlock.getHeader(&attrCatheader);
    int blockNumber = ATTRCAT_BLOCK;
    while(true) {
        for(int i=0; i < attrCatheader.numEntries; i++) {
            attrCatBlock.getRecord(attrCatRecord, i);
            if(!strcmp(attrCatRecord[ATTRCAT_REL_NAME_INDEX].sVal, "Students")) {
                AttrCacheEntry *curr = new AttrCacheEntry;
                AttrCacheTable::recordToAttrCatEntry(attrCatRecord, &curr->attrCatEntry);
                //additional metadata
                curr->recId.block = blockNumber;
                curr->recId.slot = i;
                curr->next = nullptr;

                printf("\nAttribute catalog entry of Student Relation\n");
                printf("-------------------------------------------\n");
                printf("relName: %s\n", curr->attrCatEntry.relName);
                printf("attrName: %s\n", curr->attrCatEntry.attrName);
                printf("attrType: %d\n", curr->attrCatEntry.attrType);
                printf("offset: %d\n", curr->attrCatEntry.offset);
                printf("primaryFlag: %d\n", curr->attrCatEntry.primaryFlag);
                printf("rootBlock: %d\n\n", curr->attrCatEntry.rootBlock);
                
                if(head==nullptr) {
                head = curr;
                tail = curr;
                }
                else {
                    tail->next = curr;
                    tail = tail->next;
                }
            }

        }
        if(attrCatheader.rblock == -1) {
            break;
        }
        blockNumber = attrCatheader.rblock;
        attrCatBlock = RecBuffer(attrCatheader.rblock);
        attrCatBlock.getHeader(&attrCatheader);
    }

    AttrCacheTable::attrCache[2] = head;
}

OpenRelTable::~OpenRelTable() {
    // free the memory allocated for Relation cache 
    for(int i=0; i<MAX_OPEN; i++) {
        if(RelCacheTable::relCache[i] != nullptr) {
            delete RelCacheTable::relCache[i];
            RelCacheTable::relCache[i] = nullptr;
        }
    }

    //free the memory allocated for Attribute cache
    for(int i=0; i<MAX_OPEN; i++) {
        if(AttrCacheTable::attrCache[i] != nullptr) {
            AttrCacheEntry *curr = AttrCacheTable::attrCache[i];
            while(curr != nullptr) {
                AttrCacheEntry *next = curr->next;
                delete curr;
                curr = next;
            }
        }
        AttrCacheTable::attrCache[i] = nullptr;
    }
}