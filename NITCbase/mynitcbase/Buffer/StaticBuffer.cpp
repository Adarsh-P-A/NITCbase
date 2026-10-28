#include "StaticBuffer.h"
#include <cstring>

unsigned char StaticBuffer::blocks[BUFFER_CAPACITY][BLOCK_SIZE];
struct BufferMetaInfo StaticBuffer::metainfo[BUFFER_CAPACITY];

unsigned char StaticBuffer::blockAllocMap[DISK_BLOCKS];

StaticBuffer::StaticBuffer() {
    // copy blockAllocMap from disk to buffer (using readBlock() of disk)
    for(int i=0; i<BLOCK_ALLOCATION_MAP_SIZE; i++) {
        Disk::readBlock(blockAllocMap + i*BLOCK_SIZE, i);
    }

    // all entries in buffer is free initially
    for(int bufferIndex=0; bufferIndex < BUFFER_CAPACITY; bufferIndex++) {
        metainfo[bufferIndex].free = true;
        metainfo[bufferIndex].dirty = false;
        metainfo[bufferIndex].timeStamp = -1;
        metainfo[bufferIndex].blockNum = -1;
    }
}

// destructor with writeback feature will be implemented later
StaticBuffer::~StaticBuffer() {
    // copy blockAllocMap from buffer to disk (using writeBlock() of disk)
    for(int i=0; i<BLOCK_ALLOCATION_MAP_SIZE; i++) {
        Disk::writeBlock(blockAllocMap + i*BLOCK_SIZE, i);
    }

    // write back the modified buffer to disk
    for(int bufferIndex = 0; bufferIndex < BUFFER_CAPACITY; bufferIndex++) {
        if(metainfo[bufferIndex].free == false && metainfo[bufferIndex].dirty == true) {
            Disk::writeBlock(StaticBuffer::blocks[bufferIndex], metainfo[bufferIndex].blockNum);
        }
    }
}

int StaticBuffer::getFreeBuffer(int blockNum) {
    if(blockNum < 0 || blockNum > DISK_BLOCKS-1) {
        return E_OUTOFBOUND;
    }
    // timestamp of each buffer entry should be incremented whenever we use the buffer
    for(int bufferIndex=0; bufferIndex < BUFFER_CAPACITY; bufferIndex++) {
        if(metainfo[bufferIndex].free == false) {
            metainfo[bufferIndex].timeStamp++;
        }

    }

    int allocatedBuffer = -1;
    
    // try to get a free buffer if possible
    for(int bufferIndex=0; bufferIndex < BUFFER_CAPACITY; bufferIndex++) {
        if(metainfo[bufferIndex].free == true) {
            allocatedBuffer = bufferIndex;
            break;
        }
    }

    // if free buffer is not availble, use the buffer of most stale entry
    if(allocatedBuffer==-1) {
        int staleIndex = 0;
        for(int bufferIndex = 0; bufferIndex < BUFFER_CAPACITY; bufferIndex++) {
            if(metainfo[bufferIndex].timeStamp > metainfo[staleIndex].timeStamp) {
                staleIndex = bufferIndex;
            }
        }
        // write if back safely if modified
        if(metainfo[staleIndex].dirty) {
            Disk::writeBlock(StaticBuffer::blocks[staleIndex], metainfo[staleIndex].blockNum);
        }
        allocatedBuffer = staleIndex;
    }

    metainfo[allocatedBuffer].free = false;
    metainfo[allocatedBuffer].blockNum = blockNum;
    metainfo[allocatedBuffer].dirty = false;
    metainfo[allocatedBuffer].timeStamp = 0;

    return allocatedBuffer;
}

int StaticBuffer::getBufferNum(int blockNum) {

    if(blockNum < 0 || blockNum > DISK_BLOCKS-1) {
        return E_OUTOFBOUND;
    }

    for(int bufferIndex=0; bufferIndex < BUFFER_CAPACITY; bufferIndex++) {
        if (!metainfo[bufferIndex].free && metainfo[bufferIndex].blockNum == blockNum) {
                return bufferIndex;
        }
    }

    return E_BLOCKNOTINBUFFER;

}


int StaticBuffer::setDirtyBit(int blockNum) {
    int targetIndex = getBufferNum(blockNum);

    if(targetIndex == E_BLOCKNOTINBUFFER) {
        return E_BLOCKNOTINBUFFER;
    }

    if(targetIndex == E_OUTOFBOUND) {
        return E_OUTOFBOUND;
    }

    metainfo[targetIndex].dirty = true;

    return SUCCESS;
}

int StaticBuffer::getStaticBlockType(int blockNum) {

    if(blockNum < 0 || blockNum >= DISK_BLOCKS)
        return E_OUTOFBOUND;

    return (int)blockAllocMap[blockNum];
}