#include "BlockBuffer.h"

#include <cstdlib>
#include <cstring>

BlockBuffer::BlockBuffer(int blockNum) {
    this->blockNum = blockNum;
}

// calls the parent class constructor
RecBuffer::RecBuffer(int blockNum) : BlockBuffer::BlockBuffer(blockNum) {}

// Load the block header into the argument pointer
int BlockBuffer::getHeader(struct HeadInfo *head) {
    // Instead of reading directly from disk, use a function to do it
    unsigned char *bufferPtr;
    int ret = loadBlockAndGetBufferPtr(&bufferPtr);
    if(ret != SUCCESS) {
        return ret; // if it is error, return as it is
    }

    // populate the entries in HeadInfo
    memcpy(&head->lblock, bufferPtr + 8, 4);
    memcpy(&head->rblock, bufferPtr + 12, 4);
    memcpy(&head->numEntries, bufferPtr + 16, 4);
    memcpy(&head->numAttrs, bufferPtr + 20, 4);
    memcpy(&head->numSlots, bufferPtr + 24, 4);

    return SUCCESS;

}

int RecBuffer::getRecord(union Attribute *rec, int slotNum) {
    struct HeadInfo head;

    // use getHeader to assign head
    // Properly check error return status
    int ret = BlockBuffer::getHeader(&head);
    if (ret != SUCCESS) {
        return ret;
    }
    
    // Bounds check slotNum against valid slot range
    if (slotNum < 0 || slotNum >= head.numSlots) {
        return E_OUTOFBOUND;
    }
    
    // attrCount for finding recordSize & slotCount is the length of slotmap
    int attrCount = head.numAttrs;
    int slotCount = head.numSlots;

    // Instead of reading directly from disk, use a function instead
    unsigned char *bufferPtr; 
    ret = loadBlockAndGetBufferPtr(&bufferPtr);
    if(ret != SUCCESS) {
        return ret; // If it is error, return as it is
    }

    // make a pointer pointing to start of our record
    int recordSize = attrCount*ATTR_SIZE;
    int offset = HEADER_SIZE + slotCount + slotNum*recordSize;
    unsigned char *slotPointer = bufferPtr + offset;    

    // copy from slotPointer to the length record size to rec
    memcpy(rec, slotPointer, recordSize);

    return SUCCESS;
}   

int BlockBuffer::loadBlockAndGetBufferPtr(unsigned char **bufferPtr) {
    // check if the the block is already present in the buffer
    int bufferNum = StaticBuffer::getBufferNum(this->blockNum);
    if(bufferNum == E_BLOCKNOTINBUFFER) {
        bufferNum = StaticBuffer::getFreeBuffer(this->blockNum);

        if(bufferNum == E_OUTOFBOUND) {
            return E_OUTOFBOUND;
        }
        Disk::readBlock(StaticBuffer::blocks[bufferNum], this->blockNum);
    }

    // bufferPtr stores the pointer to block in buffer
    *bufferPtr = StaticBuffer::blocks[bufferNum];
    
    return SUCCESS;
}

int RecBuffer::getSlotMap(unsigned char *slotMap) {
    // load into buffer if not exist
    unsigned char *bufferPtr;
    int status = loadBlockAndGetBufferPtr(&bufferPtr);
    if(status != SUCCESS) {
        return status;
    }

    // get headerInfo
    struct HeadInfo head;
    status = getHeader(&head);
    if(status != SUCCESS) {
        return status;
    }

    // go to offset of slotMap
    int slotCount = head.numSlots;
    unsigned char *slotMapInBuffer = bufferPtr + HEADER_SIZE;
    
    // copy slotmap to the argument variable
    memcpy(slotMap, slotMapInBuffer, slotCount);

    // return success
    return SUCCESS;
}

// compare two attributes and return -1, 0 or 1
int compareAttrs(union Attribute attr1, union Attribute attr2, int attrType) {
    double diff;
    if(attrType == STRING) {
        diff = strcmp(attr1.sVal, attr2.sVal);
    }
    else {
        diff = attr1.nVal - attr2.nVal;
    }

    if(diff < 0) 
        { return -1;}
    else if(diff > 0) 
        { return 1;}
    else 
        { return 0;}
}