# MiniBase-HeapFile-Page
In this repository you can find an implementation of the hfpage.c (heap file page) which is part of MiniBase. That was part of course Assignment on Databases. In the following lines you can find the assignment description.

## Introduction

In this assignment, you will implement the Page Structure for the Heap File layer. You will be given libraries for the Buffer Manager and Disk Space Manager, and some driver routines to test the code.

## Preliminary Work

You will be implementing just the HFPage class, and not all of the HeapFile code. Read the description in the text of how variable length records can be stored on a slotted page, and follow this page organization.

## Compiling Your Code and Running the Tests

Please Download the the files from the repository into your working directory.

Now you will see 3 generated directories:

lib/

include/

src/

src/ contains the files you will be working on. If you cd src/ and then make the project, it will create an executable named hfpage . Right now, it does not work; you will need to fill in the bodies of the HFPage class methods. The methods are defined (empty) in file hfpage.C.
Sample output of a correct implementation is available in sample_output.

## Design Overview and Implementation Details

Have a look at the file **hfpage.h in include/.** It contains the interfaces for the HFPage class. This class implements a "heap-file page" object. Note that the protected data members of the page are given to you. All you should need to do is implement the public member functions. You should put all your code into the file src/hfpage.C.

![Heap File Representation](https://i.ibb.co/YyNFx7c/dire.gif)

**A note on the slot directory:** In the description in the text, the slot directory is located at the end of the page, and grows toward the beginning. This has confused students in the past, since it means that negative offsets into the slot directory have to be used, so the current definition of HFPage has the slot directory at the beginning of the page, after a few fixed member fields, and growing toward the end. This does mean, however, that you will need to write the code so the records themselves are placed beginning at the end of the page. Be very careful with your pointer arithmetic.

Also note that in order to add a record to a page, there has to be a room for the record itself in the data area, and also room for a new slot in the data area (unless there happens to be a pre-allocated slot that's empty).

Please follow the Minibase [Error Protocol](https://research.cs.wisc.edu/coral/minibase/system/error.html). An example file illustrating the use of the error protocol is available in **src/ErrProc.sample.** It covers much of what you need to know about the protocol. You can look at new_error.h for more details. It is in **include/.**

## The Methods to be Implemented

**void HFPage::init(PageId pageNo):** This member function is used to initialize a new heap file page with page number pageNo. It should set the following data members to reasonable defaults: nextPage, PrevPage, slotCnt, curPage, usedPtr, freeSpace. You will find the definitions of these data members in hfpage.h. The nextPage and prevPage data members are used for keeping track of pages in a HeapFile. A good default unknown value for a PageId is INVALID_PAGE, as defined in page.h. Note that usedPtr is an offset into the data array, not a pointer.

**PageId HFPage::getNextPage():** This member function should return the page id stored in the nextPage data member.

**PageId HFPage::getPrevPage():** This member function should return the page id stored in the prevPage data member.

**void HFPage::setNextPage(PageId pageNo):** This member function sets the nextPage data member.

**void HFPage::setPrevPage(PageId pageNo):** This member function sets the prevPage data member.

**Status HFPage::insertRecord(char* recPtr, int reclen, RID& rid):** This member function should add a new record to the page. It returns OK if everything went OK, and DONE if sufficient space does not exist on the page for the new record. If it returns OK, it should set rid to be the RID of the new record (otherwise it can leave rid untouched.) Please note in the parameter list recPtr is a char pointer and RID& denotes passed by reference. The Status enumerated type is defined in new_error.h if you're curious about it. You may want to look that file over and handle errors in a more informative manner than suggested here. The RID struct is defined to be:

**Struct RID {

**PageID pageNo;
**int slotNo;

**int operator == (const RID rid) const
**{ return (pageNo == rid.pageNo) && (slotNo == rid.slotNo); };

**int operator != (const RID rid) const
**{ return (pageNo != rid.pageNo) || (slotNo != rid.slotNo); };

**friend ostream& operator << (ostream& out, const struct RID rid); };

In C++, struct are aggregate data types built using elements of other types. The pageNo identifies a physical page number (something that the buffer manager and the DB layers understand) in the file. The slotNo specifies an entry in the slot array on the page.

**Status HFPage::deleteRecord(const RID& rid):** This member function deletes the record with RID rid from the page, compacting the hole created. Compacting the hole, in turn, requires that all the offsets (in the slot array) of all records after the hole be adjusted by the size of the hole, because you are moving these records to "fill" the hole. You should leave a "hole" in the slot array for the slot which pointed to the deleted record, if necessary, to make sure that the rids of the remaining records do not change. The slot array can be compacted only if the record corresponding to the last slot is being deleted. It returns OK if everything goes OK, or FAIL otherwise. (what could go wrong here?)

**Status HFPage::firstRecord(RID& firstRid):** This routine should set firstRid to be the rid of the "first" record on the page. The order in which you return records from a page is entirely up to you. If you find a first record, return OK, else return DONE.

**Status HFPage::nextRecord(RID curRid, RID& nextRid):** Given a valid current RID, curRid, this member function stores the next RID on the page in the nextRid variable. Again, the order of your return records is up to you, but do make sure you return each record exactly once if someone continually calls nextRecord! Don't worry about changes to the page between successive calls (e.g. records inserted to or deleted from the page). If you find a next RID, return OK, else return DONE. In case of an error, return FAIL.

**Status HFPage::getRecord(RID rid, char * recPtr, int& recLen):** Given a rid, this routine copies the associated record into the memory address *recPtr. You may assume that the memory pointed by *recPtr has been allocated by the caller. RecLen is set to the number of bytes that the record occupies. If all goes well, return OK, else return FAIL.

**Status HFPage::returnRecord(RID rid, char*& recPtr, int& recLen):** This routine is very similar to HFPage::getRecord, except in this case you do not copy the record into a caller-provided pointer, but instead you set the caller's recPtr to point directly to the record on the page. Again, return either OK or FAIL.

DONE is a special code for non-errors that are nonetheless not "OK": it generally means "finished" or "not found." FAIL is for errors that happen outside the bounds of a subsystem.

**int HFPage::available_space(void):** This routine should return the amount of space available for a new record that is left on the page. For instance, if all slots are full and there are 100 bytes of free space on the page, this method should return (100 - sizeof(slot_t)) bytes. This accounts for the fact that sizeof(slot_t) bytes must be reserved for a new slot and cannot be used by a new record.

**bool HFPage::empty(void):** Returns true if the page has no records in it, and false otherwise.






