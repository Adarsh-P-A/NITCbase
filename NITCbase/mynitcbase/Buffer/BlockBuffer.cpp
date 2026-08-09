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
    unsigned char Buffer[BLOCK_SIZE];

    // read the block correspoding to this object to buffer. What if the block number is invalid?
    Disk::readBlock(Buffer, this->blockNum);

    // populate the entries in HeadInfo
    memcpy(&head->lblock, Buffer + 8, 4);
    memcpy(&head->rblock, Buffer + 12, 4);
    memcpy(&head->numEntries, Buffer + 16, 4);
    memcpy(&head->numAttrs, Buffer + 20, 4);
    memcpy(&head->numSlots, Buffer + 24, 4);

    return SUCCESS;

}

int RecBuffer::getRecord(union Attribute *rec, int slotNum) {
    struct HeadInfo head;

    // use getHeader to assign head
    BlockBuffer::getHeader(&head);
    
    // attrCount for finding recordSize & slotCount is the length of slotmap
    int attrCount = head.numAttrs;
    int slotCount = head.numSlots;

    // read this block to a buffer
    unsigned char Buffer[BLOCK_SIZE];
    Disk::readBlock(Buffer, this->blockNum);

    // make a pointer pointing to start of our record
    int recordSize = attrCount*ATTR_SIZE;
    int offset = HEADER_SIZE + slotCount + slotNum*recordSize;
    unsigned char *slotPointer = Buffer + offset;

    // copy from slotPointer to the length record size to rec
    memcpy(rec, slotPointer, recordSize);

    return SUCCESS;
}   