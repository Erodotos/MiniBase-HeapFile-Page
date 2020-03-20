/**************************************************************************
* FILENAME :        hfpage.c            
*
* DESCRIPTION :
*       Class implementation for a minibase data page.   
*
* AUTHOR :    Erodotos Demetriou        MODIFICATION DATE :    12 Jan 2020
*
***************************************************************************/

#include <memory.h>
#include <stdlib.h>
#include <iostream>

#include "buf.h"
#include "db.h"
#include "heapfile.h"
#include "hfpage.h"

// **********************************************************
// page class constructor

void HFPage::init(PageId pageNo) {
    this->slotCnt = 0;

    this->curPage = pageNo;
    this->prevPage = INVALID_PAGE;
    this->nextPage = INVALID_PAGE;

    this->usedPtr = MAX_SPACE - DPFIXED;
    this->freeSpace = MAX_SPACE - DPFIXED + sizeof(slot_t);

    this->slot[0].length = EMPTY_SLOT;
    this->slot[0].offset = 0;
}

// **********************************************************
// dump page utlity
void HFPage::dumpPage() {
    int i;

    cout << "dumpPage, this: " << this << endl;
    cout << "curPage= " << curPage << ", nextPage=" << nextPage << endl;
    cout << "usedPtr=" << usedPtr << ",  freeSpace=" << freeSpace
         << ", slotCnt=" << slotCnt << endl;

    for (i = 0; i < slotCnt; i++) {
        cout << "slot[" << i << "].offset=" << slot[i].offset
             << ", slot[" << i << "].length=" << slot[i].length << endl;
    }
}

// **********************************************************
PageId HFPage::getPrevPage() {
    return this->prevPage;
}

// **********************************************************
void HFPage::setPrevPage(PageId pageNo) {
    this->prevPage = pageNo;
}

// **********************************************************
void HFPage::setNextPage(PageId pageNo) {
    this->nextPage = pageNo;
}

// **********************************************************
PageId HFPage::getNextPage() {
    return this->nextPage;
}

// **********************************************************
// Add a new record to the page. Returns OK if everything went OK
// otherwise, returns DONE if sufficient space does not exist
// RID of the new record is returned via rid parameter.
Status HFPage::insertRecord(char* recPtr, int recLen, RID& rid) {
    if (available_space() < recLen) {
        return DONE;
    }

    int slotNum = 0;
    for (slotNum = 0; slotNum < slotCnt; slotNum++) {
        if (slot[slotNum].length == EMPTY_SLOT)
            break;
    }

    usedPtr = usedPtr - recLen;

    slot[slotNum].offset = usedPtr;
    slot[slotNum].length = recLen;
    if (slotNum == slotCnt) slotCnt++;

    memcpy(data + usedPtr, recPtr, recLen);

    freeSpace -= recLen;

    rid.pageNo = curPage;
    rid.slotNo = slotNum;

    return OK;
}

// **********************************************************
// Delete a record from a page. Returns OK if everything went okay.
// Compacts remaining records but leaves a hole in the slot array.
// Use memmove() rather than memcpy() as space may overlap.
Status HFPage::deleteRecord(const RID& rid) {
    if (slotCnt == 0 || rid.slotNo < 0 || rid.slotNo >= slotCnt || slot[rid.slotNo].length == EMPTY_SLOT)
        return FAIL;

    int offset = slot[rid.slotNo].offset;
    int length = slot[rid.slotNo].length;
    memmove(data + usedPtr + length, data + usedPtr, offset - usedPtr);
    usedPtr += length;

    slot[rid.slotNo].length = EMPTY_SLOT;
    for (int i = slotCnt - 1; i >= 0; i--) {
        if (slot[i].length != EMPTY_SLOT) {
            break;
        }
        slotCnt--;
    }

    if (rid.slotNo < slotCnt) {
        for (int i = rid.slotNo + 1; i < slotCnt; i++) {
            slot[i].offset += length;
        }
    }

    freeSpace += length;

    return OK;
}

// **********************************************************
// returns RID of first record on page
Status HFPage::firstRecord(RID& firstRid) {
    if (empty()) {
        return DONE;
    }

    if (slotCnt == 0)
        return FAIL;

    bool hasRecord = false;

    for (int i = 0; i < slotCnt; i++) {
        if (slot[i].length != EMPTY_SLOT) {
            firstRid.slotNo = i;
            firstRid.pageNo = curPage;
            hasRecord = true;
            break;
        }
    }

    if (!hasRecord) {
        return DONE;
    }

    return OK;
}

// **********************************************************
// returns RID of next record on the page
// returns DONE if no more records exist on the page; otherwise OK
Status HFPage::nextRecord(RID curRid, RID& nextRid) {
    if (curRid.pageNo != curPage) {
        return FAIL;
    }

    if (empty()) {
        return FAIL;
    }

    bool foundNext = false;

    for (int i = curRid.slotNo + 1; i < slotCnt; i++) {
        if (slot[i].length != EMPTY_SLOT) {
            nextRid.slotNo = i;
            nextRid.pageNo = curPage;
            foundNext = true;
            break;
        }
    }

    if (foundNext) {
        return OK;
    }

    return DONE;
}

// **********************************************************
// returns length and copies out record with RID rid
Status HFPage::getRecord(RID rid, char* recPtr, int& recLen) {
    if (rid.pageNo != curPage || slotCnt == 0 || rid.slotNo < 0 || rid.slotNo >= slotCnt || slot[rid.slotNo].length == EMPTY_SLOT) {
        return FAIL;
    }

    for (int i = 0; i < slot[rid.slotNo].length; i++) {
        *recPtr = data[slot[rid.slotNo].offset + i];
        recPtr++;
    }

    recLen = slot[rid.slotNo].length;

    return OK;
}

// **********************************************************
// returns length and pointer to record with RID rid.  The difference
// between this and getRecord is that getRecord copies out the record
// into recPtr, while this function returns a pointer to the record
// in recPtr.
Status HFPage::returnRecord(RID rid, char*& recPtr, int& recLen) {
    if (rid.pageNo != curPage || slotCnt == 0 || rid.slotNo < 0 || rid.slotNo >= slotCnt || slot[rid.slotNo].length == EMPTY_SLOT) {
        return FAIL;
    }

    recPtr = &data[this->slot[rid.slotNo].offset];
    recLen = slot[rid.slotNo].length;

    return OK;
}

// **********************************************************
// Returns the amount of available space on the heap file page
int HFPage::available_space(void) {
    if (slotCnt == 0) {
        return freeSpace - sizeof(slot_t);
    }
    return freeSpace - slotCnt * sizeof(slot_t);
}

// **********************************************************
// Returns 1 if the HFPage is empty, and 0 otherwise.
// It scans the slot directory looking for a non-empty slot.
bool HFPage::empty(void) {
    for (int i = 0; i < this->slotCnt; i++) {
        if (slot[i].length != EMPTY_SLOT) {
            return 0;
        }
    }
    return 1;
}