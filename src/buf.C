
//#include <new.h>

#include "buf.h"
#include <new>

// ************** ALL BELOW are purely local to buffer Manager *******
static const char *bufErrMsgs[] = {
                    "hash table error",
                    "hash entry not found",
                    "buffer pool full",
                    "page not pinned",
                    "buffer pool corrupted",
                    "page still pinned",
                    "replacer error",
                    "illegal buffer frame number received by replacer",
                    "Page not found in the buffer pool"
                    "Frame already empty"
                   };
static error_string_table bufTable( BUFMGR, bufErrMsgs );

#ifdef DEBUG
int total; // global pincount for the buffer manager - should be a variable
           // within the BufMgr class.
#endif



//----------------------------------------
// Constructor of the class BufMgr
//----------------------------------------

BufMgr::BufMgr( int bufsize, Replacer *replacerArg ) {
    numBuffers = bufsize;


    bufPool = (Page*) MINIBASE_SHMEM->malloc( numBuffers * sizeof(Page) );


    for ( unsigned i=0; i < numBuffers; i++ ) {
         // Construct each buffer frame.
        new(bufPool+i) Page;
    }

    frmeTable = (FrameDesc*)
        MINIBASE_SHMEM->malloc( numBuffers * sizeof(FrameDesc) );
    for ( unsigned int index=0; index < numBuffers; index++ )
        new(frmeTable+index) FrameDesc;


    if (replacerArg) {
        replacer = replacerArg;
    } else {
        char *replacerAddress = (char *)MINIBASE_SHMEM->malloc(sizeof(REPLACER));
        replacer = new (replacerAddress) REPLACER;
    }

    replacer->setBufferManager( this );
}

BufMgr::~BufMgr()
{
#ifdef DEBUG
    cout <<"Entering the BM destructor\n"<<endl;
    cout << "total = numPins - numunPins = " << total << endl;
#endif

    // write all the dirty pages to the disk
    Status status = flushAllPages();
    if (status != OK)
        MINIBASE_CHAIN_ERROR( BUFMGR, status );

    delete replacer;
    delete (void *) bufPool;
    delete (void *) frmeTable;
}

Status BufMgr::flushAllPages()
{
    return privFlushPages(INVALID_PAGE,1);
}

Status BufMgr::flushPage(int pageid)
{
    return privFlushPages(pageid);
}

// Factor out the common code for the two versions of Flush
Status BufMgr::privFlushPages(int pageid, int all_pages)
{
    unsigned int i;
    int unpinned = 0;
    Status status = OK;

    for (i=0; i < numBuffers; i++)   // write all valid dirty pages to disk
        if ( all_pages || (frmeTable[i].pageNo == pageid)) {

            if (frmeTable[i].pageNo == INVALID_PAGE)
                continue;

            if ( frmeTable[i].pin_count() != 0 )
                unpinned++;

            PageId pageid = frmeTable[i].pageNo;
            status = MINIBASE_DB->write_page(pageid, &bufPool[i]);

            if (status != OK) 
                return MINIBASE_CHAIN_ERROR( BUFMGR, status );

            frmeTable[i].pageNo = INVALID_PAGE; // frame is empty

            if (!all_pages) {
                if (status == OK) {
                    if (unpinned) 
                        return MINIBASE_FIRST_ERROR( BUFMGR, PAGE_PINNED );
                }
                return status;
            }
        }

    if (all_pages) {
        if (unpinned) 
            return MINIBASE_FIRST_ERROR( BUFMGR, PAGE_PINNED );
        return OK;
    }

    return MINIBASE_FIRST_ERROR(BUFMGR,PAGE_NOT_FOUND);
}

Status BufMgr::pinPage(int pin_pgid, Page*& page, int emptyPage, const char*)
{
#ifdef DEBUG
    total++;
    printf("BufMgr::PinPage,Incremented pin_count to) %d\n",total);
#endif
    int     frameNo;
    Status  status,st;
    int     oldpageNo = INVALID_PAGE;

    for (frameNo=0; frameNo < (int) numBuffers; frameNo++)
        if (frmeTable[frameNo].pageNo == pin_pgid) 
            break;

    if (frameNo >= (int) numBuffers) {       // Not in the buffer pool
  
        frameNo = replacer->pick_victim(); // frameNo is pinned
        if (frameNo == INVALID_PAGE) { 
            page = NULL;     
            return MINIBASE_FIRST_ERROR( BUFMGR, REPLACER_ERROR );
        }

        if (frmeTable[frameNo].pageNo != INVALID_PAGE)
            oldpageNo = frmeTable[frameNo].pageNo;

        frmeTable[frameNo].pageNo = INVALID_PAGE; // frame is empty
        frmeTable[frameNo].pageNo = pin_pgid;

        if (oldpageNo != INVALID_PAGE) {
            status = MINIBASE_DB->write_page(oldpageNo, &bufPool[frameNo]);
            if (status != OK)
                return MINIBASE_CHAIN_ERROR( BUFMGR, status );
        }

        // read in the page if not empty
        if (emptyPage == FALSE) {
            status = MINIBASE_DB->read_page(pin_pgid, &bufPool[frameNo]);
            if (status != OK) {
                MINIBASE_CHAIN_ERROR( BUFMGR, status );

                frmeTable[frameNo].pageNo = INVALID_PAGE; // frame is empty

                st = (Status) replacer->unpin(frameNo);

                if (st != OK)
                    return MINIBASE_FIRST_ERROR( BUFMGR, REPLACER_ERROR );

                return BUFMGR;
            }
        }

        page = &bufPool[frameNo];
        return OK;

    } else {    // the page is in the buffer pool ( frameNo > 0 )

        page = &bufPool[frameNo];
        if ((status = (Status) replacer->pin(frameNo)) != OK) {
            MINIBASE_FIRST_ERROR( BUFMGR, REPLACER_ERROR );
            return BUFMGR;
        }
        return OK;
   }
}


Status BufMgr::unpinPage(int PageId_in_a_DB, int /* dirty */, const char *)
{
#ifdef DEBUG
    total--;
    printf("BufMgr::UnPinPage,Decremented pin_count to) %d\n",total);
#endif
    int frameNo;

    for (frameNo=0; frameNo < (int) numBuffers; frameNo++)
        if (frmeTable[frameNo].pageNo == PageId_in_a_DB) 
            break;

    if (frameNo >= (int) numBuffers)
        return MINIBASE_FIRST_ERROR( BUFMGR, HASH_NOT_FOUND );

    if (frmeTable[frameNo].pageNo == INVALID_PAGE)
        return MINIBASE_FIRST_ERROR( BUFMGR, PAGE_NOT_PINNED );

    if ((replacer->unpin(frameNo)) != 0)
        return MINIBASE_FIRST_ERROR( BUFMGR, REPLACER_ERROR );

    return OK;
}

//////////////////////////////////////////////////////////////////////////////
// This is called by DB::deallocate_page()
// It frees a page, given a page Id

Status BufMgr::freePage(int globalPageId)
{
    int frameNo;
    Status status;

    for (frameNo=0; frameNo < (int) numBuffers; frameNo++)
        if (frmeTable[frameNo].pageNo == globalPageId) 
            break;

    // if globalPageId is not in pool then call deallocate
    if (frameNo >= (int) numBuffers)
        return MINIBASE_DB->deallocate_page(globalPageId);

    status = (Status) replacer->free(frameNo);
    if (status != OK)
        return MINIBASE_FIRST_ERROR( BUFMGR, REPLACER_ERROR );

    frmeTable[frameNo].pageNo = INVALID_PAGE; // frame is empty

    status=MINIBASE_DB->deallocate_page(globalPageId);
    if (status != OK)
        return MINIBASE_CHAIN_ERROR( BUFMGR, status );

    return OK;
}

Status BufMgr::newPage(int& firstPageId, Page*& firstpage, int howmany)
{
    int       i;
    Status    status, st2;

    if ((status=MINIBASE_DB->allocate_page(firstPageId,howmany)) != OK)
      return MINIBASE_CHAIN_ERROR( BUFMGR, status );

    if ((status = pinPage(firstPageId,firstpage,TRUE)) != OK) {
        // rollback because pin failed
        MINIBASE_FIRST_ERROR( BUFMGR, BUFFER_EXCEEDED );
        for (i=0; i < howmany; i++){
            st2 = MINIBASE_DB->deallocate_page(i+firstPageId);
            if (st2 != OK)
                return MINIBASE_CHAIN_ERROR( BUFMGR, st2 );
         }
         return  BUFMGR;    
     } else
         return OK;
}


unsigned int BufMgr::getNumUnpinnedBuffers()
{
    return replacer->getNumUnpinnedBuffers();
}

//-------------------------------------------------------------------         
Replacer::Replacer()
{
    mgr = NULL;
    state_bit = 0;
    head = -1;
}

Replacer::~Replacer()
{
    delete [] state_bit;
}

void Replacer::setBufferManager( BufMgr *mgrArg )
{
    delete [] state_bit;
    mgr = mgrArg;
    unsigned numBuffers = mgr->getNumBuffers();

    char *Sh_StateArr = MINIBASE_SHMEM->malloc((numBuffers * sizeof(STATE)));
    state_bit = new(Sh_StateArr) STATE[numBuffers];

    for ( unsigned index=0; index < numBuffers; ++index ) {
        state_bit[index] = Available;
    }

    head = -1; // maintain the head of the clock.
}

int Replacer::pin( int frameNo )
{
    if ((frameNo < 0) || (frameNo >= (int)mgr->getNumBuffers())) {
        MINIBASE_FIRST_ERROR( BUFMGR, BAD_BUF_FRAMENO );
        return(-1);
    }


    (mgr->frameTable())[frameNo].pin();
    state_bit[frameNo] = Pinned;

    return OK;
}

int Replacer::unpin( int frameNo )
{
    if ((frameNo < 0) || (frameNo >= (int)mgr->getNumBuffers())) {
      MINIBASE_FIRST_ERROR( BUFMGR, BAD_BUF_FRAMENO );
      return(-1);
    }

    if ((mgr->frameTable())[frameNo].pin_count() == 0) {
        MINIBASE_FIRST_ERROR( BUFMGR, PAGE_NOT_PINNED );
        return(-1);
    }

    (mgr->frameTable())[frameNo].unpin();

    if ((mgr->frameTable())[frameNo].pin_count() == 0)
        state_bit[frameNo] = Referenced;

    return OK;
}

int Replacer::free( int frameNo )
{
    if ( (mgr->frameTable())[frameNo].pin_count() > 1 ) {
        MINIBASE_FIRST_ERROR( BUFMGR, PAGE_PINNED );
        return -1;
    }

    (mgr->frameTable())[frameNo].unpin();
    state_bit[frameNo] = Available;

    return OK;
}


void Replacer::info()
{
    cout << "\nInfo:\nstate_bits:(R)eferenced | (A)vailable | (P)inned"
         << endl;

    unsigned numBuffers = mgr->getNumBuffers();

    for ( unsigned i = 0; i < numBuffers; ++i ) {
        if (((i + 1) % 9) == 0)
            cout << endl;
        cout << "(" << i << ") ";
        switch(state_bit[i]){
          case Referenced:
              cout << "R\t";
              break;
          case Available:
              cout << "A\t";
              break;
          case Pinned:
              cout << "P\t";
              break;
          default:
              cerr << "ERROR from Replacer.info()" << endl;
              break;
        }
    }

    cout << "\n\n";
}

unsigned Replacer::getNumUnpinnedBuffers()
{
    unsigned int numBuffers = mgr->getNumBuffers();
    unsigned int answer = 0;

    for ( unsigned int index=0; index < numBuffers; ++index )
        if ( (mgr->frameTable())[index].pin_count() == 0 )
            ++answer;

    return answer;
}

Clock::Clock()
{
}

Clock::~Clock()
{
}

int Clock::pick_victim()
{
    unsigned num = 0;
    unsigned numBuffers = mgr->getNumBuffers();

    head = (head+1) % numBuffers;
    while ( state_bit[head] != Available ) {
        if ( state_bit[head] == Referenced )
            state_bit[head] = Available;

        if ( num == 2*numBuffers ) {
            MINIBASE_FIRST_ERROR( BUFMGR, BUFFER_EXCEEDED );
            return -1;    // no frame available
        }
        ++num;
        head = (head+1) % numBuffers;
    }

        // Make sure pin count is 0.
    assert( (mgr->frameTable())[head].pin_count() == 0 );
    state_bit[head] = Pinned;        // Pin this victim so that other
    (mgr->frameTable())[head].pin();    // process can't pick it as victim (???)

    return head;
}

void Clock::info()
{
    Replacer::info();
    cout << "Clock hand:\t" << head;
    cout << "\n\n";
}

