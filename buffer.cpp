#include "buffer.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

void DSMgr::openFile(string fname) {
    currFile = fopen(fname.c_str(), "rb+");
}


void DSMgr::readPage(int page_id, bFrame& frame) {
    int page_offset = page_id * FRAMESIZE;
    fseek(currFile, page_offset, SEEK_SET);
    int succ = fread(&frame.field, sizeof(frame.field), 1, currFile);
    assert(succ);
    io_num ++;
}

int DSMgr::writePage(int page_id, bFrame& frame) {
    int page_offset = page_id * FRAMESIZE;
    fseek(currFile, page_offset, SEEK_SET);
    int succ = fwrite(&frame.field, sizeof(frame.field), 1, currFile);
    assert(succ);
    io_num ++;
    return sizeof(frame.field);
}

int Bgr::fixPage(int page_id) {
    assert(ptof.size() <= BUFFERSIZE);

    // In frames already
    auto search_bcb = ptof.find(page_id);
    if (search_bcb != ptof.end()) {
        BCB* bcb = search_bcb->second;
        hit_count++;
        // move the bcb to the most frequent;
        updateHitted(bcb);
        return bcb->frame_id;
    }

    // Not in frames
    int num_free = numFreeFrames();
    int frame_id;
    if (num_free > 0) {
        frame_id = frame_num;
        incNumFrames();
    } else {
        frame_id = selectVictim(); 
        // Remove old bcb
        BCB* old_bcb = ftob[frame_id];
        assert(old_bcb != NULL);    
        removeBCB(old_bcb);
    }
    // Should be empty
    assert(frame_id < BUFFERSIZE);
    BCB* bcb = addBCB(page_id, frame_id);
    // insert the new bcb to the most frequent; 
    updateUnhitted(bcb);
    return frame_id;
}

void Bgr::removeBCB(BCB* ptr) {
    int frame_id = ptr->frame_id; 
    int page_id = ptr->page_id;
    ptof.erase(page_id);
    ftop.erase(frame_id);
    // May Need to write back to disk if the frame is dirty.
    if (ptr->dirty) {
       dsmgr.writePage(page_id, buf[frame_id]); 
    }
    delete ftob[frame_id];
    ftob[frame_id] = NULL;
}

BCB* Bgr::addBCB(int page_id, int frame_id) {
    BCB* bcb = new BCB();
    bcb->page_id = page_id;
    bcb->frame_id = frame_id;
    ftob[frame_id] = bcb;
    ftop.insert({frame_id, page_id});
    ptof.insert({page_id, bcb});
    return bcb;
}


bFrame* Bgr::readPage(int page_id) {
    int frame_id = fixPage(page_id);
    dsmgr.readPage(page_id, buf[frame_id]);
    access_num ++;
    assert(frame_id < BUFFERSIZE);
    return &buf[frame_id]; 
}

void Bgr::writePage(int page_id, char * data) {
    int frame_id = fixPage(page_id);
    bFrame& frame = buf[frame_id];
    memcpy(frame.field, data, sizeof(frame.field));
    BCB* bcb = ftob[frame_id];
    bcb->dirty = 1;
    access_num ++;
}

