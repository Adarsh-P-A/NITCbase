#include "StaticBuffer.h"

unsigned char StaticBuffer::blocks[BUFFER_CAPACITY][BLOCK_SIZE];
struct BufferMetaInfo StaticBuffer::metainfo[BUFFER_CAPACITY];

StaticBuffer::StaticBuffer() {
    // all entries in buffer is free initially
    for(int bufferIndex=0; bufferIndex < BUFFER_CAPACITY; bufferIndex++) {
        metainfo[bufferIndex].free = true;
    }
}

// destructor with writeback feature will be implemented later
StaticBuffer::~StaticBuffer() {}

int StaticBuffer::getFreeBuffer(int blockNum) {
    if(blockNum < 0 || blockNum > DISK_BLOCKS-1) {
        return E_OUTOFBOUND;
    }

    int allocatedBuffer;

    for(int bufferIndex=0; bufferIndex < BUFFER_CAPACITY; bufferIndex++) {
        if(metainfo[bufferIndex].free == true) {
            allocatedBuffer = bufferIndex;
            break;
        }
    }

    metainfo[allocatedBuffer].free = false;
    metainfo[allocatedBuffer].blockNum = blockNum;

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
