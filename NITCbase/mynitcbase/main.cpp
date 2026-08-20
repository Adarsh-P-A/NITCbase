#include <iostream>
#include <cstring>
#include "Buffer/StaticBuffer.h"
#include "Cache/OpenRelTable.h"
#include "Disk_Class/Disk.h"
#include "FrontendInterface/FrontendInterface.h"


void printSchemas() {
  // create objects for relation catalog and attribute catalog
  RecBuffer relCatBuffer(RELCAT_BLOCK);
  RecBuffer attrCatBuffer(ATTRCAT_BLOCK);
  
  // Head info is a structure
  HeadInfo relCatHeader;
  HeadInfo attrCatHeader;

  relCatBuffer.getHeader(&relCatHeader);
  attrCatBuffer.getHeader(&attrCatHeader);

    for(int i=0; i < relCatHeader.numEntries; i++) {
      Attribute relCatRecord[RELCAT_NO_ATTRS];
      relCatBuffer.getRecord(relCatRecord, i);

      const char *relationName = relCatRecord[RELCAT_REL_NAME_INDEX].sVal;
      // print name of relation
      printf("Relation %s\n", relationName);
      while(true) {
            // the outer loop is for traversing multiple blocks of attribute catalog if present
            // inner loop is for traversing inside one block of attribute catalog
        for(int j=0; j < attrCatHeader.numEntries; j++) {

          Attribute attrCatRecord[ATTRCAT_NO_ATTRS];
          attrCatBuffer.getRecord(attrCatRecord, j);

          if(strcmp(relationName, "Students")==0 && strcmp(attrCatRecord[ATTRCAT_ATTR_NAME_INDEX].sVal, "Class")==0) {
            strcpy(attrCatRecord[ATTRCAT_ATTR_NAME_INDEX].sVal,"Batch");
          }

          if(strcmp(relationName, attrCatRecord[ATTRCAT_REL_NAME_INDEX].sVal)==0) {
            const char *attrType = (attrCatRecord[ATTRCAT_ATTR_TYPE_INDEX].nVal==NUMBER) ? "NUM" : "STR";
            printf("  %s: %s\n", attrCatRecord[ATTRCAT_ATTR_NAME_INDEX].sVal, attrType);
          }
        }
        if(attrCatHeader.rblock == -1)
          attrCatBuffer = RecBuffer(ATTRCAT_BLOCK);
        break;
      
        attrCatBuffer = RecBuffer(attrCatHeader.rblock);
        attrCatBuffer.getHeader(&attrCatHeader);
      }
    }
  }


int main(int argc, char *argv[]) {
  /* Initialize the Run Copy of Disk */
  Disk disk_run;
  // Initialise a buffer with 32*2048 bytes
  StaticBuffer buffer;
  OpenRelTable cache;
  
  return FrontendInterface::handleFrontend(argc, argv);

}