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
    BlockBuffer::getHeader(&head);
    
    // attrCount for finding recordSize & slotCount is the length of slotmap
    int attrCount = head.numAttrs;
    int slotCount = head.numSlots;

    // Instead of reading directly from disk, use a function instead
    unsigned char *bufferPtr; 
    int ret = loadBlockAndGetBufferPtr(&bufferPtr);
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

