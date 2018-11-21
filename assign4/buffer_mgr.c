#include "buffer_mgr.h"
#include "storage_mgr.h"
#include <string.h>
#include <stdlib.h>

/* Custom Functions*/

//this function is used increase fixcount if pageNumer nth frame was already pinned
frame *alreadyPinnedFixCount(BM_BufferPool *const bm, const PageNumber pageNum)
{
    buffer *queue = bm->mgmtData;
    frame *framepointer = queue->head;
    // Return frame pointer if found, or NULL
    do
    {
        if (framepointer->currpage==pageNum)
        {
            framepointer->fixCount++; //increase the fix count if pinned
            return framepointer;
        }
        framepointer=framepointer->next; 
    }while (framepointer!=queue->head);
    return NULL;// == return false
}

//this function will pin the page pointed by framepointer with page number nth page
int pinThispage(BM_BufferPool *const bm, frame *framepointer, PageNumber pageNum)
{
    buffer *queue = bm->mgmtData;
    
    //open a page file
    RC rc_return_value = openPageFile(bm->pageFile, &bm->fH);
    if (rc_return_value!=RC_OK) return rc_return_value;

    //check capacity of a file
    rc_return_value = ensureCapacity(pageNum, &bm->fH); 
    if (rc_return_value!=RC_OK) return rc_return_value;
    
    //check page is a dirty 
    if (framepointer->dirty)
    {
        rc_return_value = writeBlock(framepointer->currpage, &bm->fH, framepointer->data);//write block to file
        if (rc_return_value!=RC_OK) return rc_return_value;
        framepointer->dirty = false;
        queue->numWrite++;//increase writeIO
    }
    
    //read block from file
    rc_return_value = readBlock(pageNum, &bm->fH, framepointer->data);
    if (rc_return_value!=RC_OK) return rc_return_value;
    queue->numRead++;//increase readIO
    framepointer->currpage = pageNum; //update page number
    framepointer->fixCount++;//update fix count
    closePageFile(&bm->fH);//close the page file
    
    return 0;
}

/* Pinning Functions*/

//pins the page with page number with FIFO replacement strategy
RC pinFIFO (BM_BufferPool *const bm, BM_PageHandle *const page,const PageNumber pageNum, bool fromLRU)
//pin the first avaliable frame, then move the frame to tail
 
{
    frame *framepointer;
    //If was called by LRU then no need to check pinned
    if (!fromLRU)
    {
        framepointer = alreadyPinnedFixCount(bm,pageNum);//fix the fix count if page is pinned
        if (framepointer)
        {
            page->pageNum = pageNum;
            page->data = framepointer->data;
            return RC_OK;
        }
    }
    //load into memory using FIFO
    buffer *queue = bm->mgmtData;
    framepointer = queue->head;

    //finding first avaliable frame until framepointer until it reahces to tail
    bool notfind = true;
    do
    {
        if (framepointer->fixCount == 0) //check condition
        {
            notfind = false; 
            break;
        }
        framepointer = framepointer->next;
    }while (framepointer!=queue->head);
    
    if (notfind)
        return RC_IM_NO_MORE_ENTRIES; //if it is not able to find then send error no avaliable frame
    
    //pins the page if it available
    RC rc_return_value = pinThispage(bm, framepointer, pageNum);
    if (rc_return_value!=RC_OK) return rc_return_value;
    
    page->pageNum = pageNum;
    page->data = framepointer->data;
    
    //change lists
    if (framepointer == queue->head)
        queue->head = framepointer->next;
    //moving the pinned page to tail
    framepointer->prev->next = framepointer->next;
    framepointer->next->prev = framepointer->prev;
    framepointer->prev = queue->tail;
    queue->tail->next = framepointer;
    queue->tail = framepointer;
    framepointer->next = queue->head;
    queue->head->prev = framepointer;
    
    return RC_OK;
}
//pins the page with page number with LRU replacement strategy
RC pinLRU (BM_BufferPool *const bm, BM_PageHandle *const page,const PageNumber pageNum)
{
    frame *framepointer = alreadyPinnedFixCount(bm,pageNum);//if page is pinned then increase the fix count
    
    //If pinned then move the frame to tail. If not pinned then same as FIFO. 
    if (framepointer)
    {
        //change priority
        buffer *queue = bm->mgmtData;
        if (framepointer == queue->head)
            queue->head = framepointer->next;

        //moving the pinned page to tail
        framepointer->prev->next = framepointer->next;
        framepointer->next->prev = framepointer->prev;
        framepointer->prev = queue->tail;

        queue->tail->next = framepointer;
        queue->tail = framepointer;
        framepointer->next = queue->head;
        queue->head->prev = framepointer;
        
        page->pageNum = pageNum; 
        page->data = framepointer->data;
    }
    else
        return pinFIFO(bm, page, pageNum,true);
    return RC_OK;
}

//pins the page with page number with CLOCK replacement strategy
RC pinCLOCK (BM_BufferPool *const bm, BM_PageHandle *const page,const PageNumber pageNum)
{
    frame *framepointer = alreadyPinnedFixCount(bm,pageNum);//fix the fix count
    if (framepointer)
    {
        page->pageNum = pageNum;
        page->data = framepointer->data;
        return RC_OK;
    }

    buffer *queue = bm->mgmtData;
    framepointer = queue->pointer->next;
    bool notfind = true;
    
    //find the node with given page number
    while (framepointer!=queue->pointer)
    {
        if (framepointer->fixCount == 0)
        {
            if (!framepointer->refbit) //refbit = 0
            {
                notfind = false;
                break;
            }
            framepointer->refbit = false; //on the way set all bits to 0
        }
        framepointer = framepointer->next;
    };
    
    if (notfind)
        return RC_IM_NO_MORE_ENTRIES; //no avaliable frame
    
    RC rc_return_value = pinThispage(bm, framepointer, pageNum);//pin the page with given page number
    if (rc_return_value!=RC_OK) return rc_return_value;
    
    queue->pointer = framepointer;
    page->pageNum = pageNum;
    page->data = framepointer->data;
    
    return RC_OK;
}

//pins the page with page number with LRUK replacement strategy
RC pinLRUK (BM_BufferPool *const bm, BM_PageHandle *const page,
            const PageNumber pageNum)
{
    return RC_OK;
}

/*-------------------------Buffer Manager Interface Pool Handling functions begins---------------------*/
//creates a new buffer pool with numPages page frames using the page replacement strategy
RC initBufferPool(BM_BufferPool *const bm, const char *const pageFileName,const int numPages, ReplacementStrategy strategy,void *stratData)
{
    //check error if numpages is less than 0
    if (numPages<=0) 
        return RC_WRITE_FAILED;
    //intializing a buffer zize
    buffer *queue = malloc(sizeof(buffer));
    
    if (queue==NULL) return RC_WRITE_FAILED;
    queue->numFrames = numPages;
    queue->stratData = stratData;
    queue->numRead = 0;
    queue->numWrite = 0;

    //create a new list for page
    int i;
    frame *newpage = malloc(sizeof(frame));
    statlist *statlisthead = malloc(sizeof(statlist));
    if (newpage==NULL) return RC_WRITE_FAILED;
    newpage->currpage=NO_PAGE;
    newpage->refbit=false;
    newpage->dirty=false;
    newpage->fixCount=0;
    memset(newpage->data,'\0',PAGE_SIZE);
    statlisthead->fpt = newpage;
    
    queue->head = newpage;
    queue->stathead = statlisthead;
    
    for (i=1; i<numPages; i++) 
    { 
        //i start from 1
        frame *newpinfo = malloc(sizeof(frame));
        statlist *statlistnew = malloc(sizeof(statlist));
        if (newpinfo==NULL) return RC_WRITE_FAILED;
        newpinfo->currpage=NO_PAGE;
        newpinfo->dirty=false;
        newpinfo->refbit=false;
        newpinfo->fixCount=0;
        memset(newpinfo->data,'\0',PAGE_SIZE);
        
        statlistnew->fpt = newpinfo;
        statlisthead->next = statlistnew;
        statlisthead = statlistnew;
        
        newpage->next=newpinfo;
        newpinfo->prev=newpage;
        newpage=newpinfo;
    }
    statlisthead->next = NULL;
    queue->tail = newpage;
    queue->pointer = queue->head;
    
    //circular list for clock
    queue->tail->next = queue->head;
    queue->head->prev = queue->tail;
    
    //set bufferpool for bufferpool
    bm->numPages = numPages;
    bm->pageFile = (char *)pageFileName;
    bm->strategy = strategy;
    bm->mgmtData = queue;
    
    return RC_OK;
}

//destroys a buffer pool
RC shutdownBufferPool(BM_BufferPool *const bm)
{
    //write dirty pages back to disk
    RC rc_return_value = forceFlushPool(bm);
    if (rc_return_value!=RC_OK) return rc_return_value;
    //free up resources
    buffer *queue = bm->mgmtData;
    frame *framepointer = queue->head;//set framepoiner as a head
    
    //free all nodes
    while (framepointer!=queue->tail)
    {
        framepointer = framepointer->next;
        free(queue->head); // free head
        queue->head = framepointer; //set next framepointer as a head
    }
    free(queue->tail); //free tail
    free(queue);//free queue
    
    //set bufferpool parameters as null
    bm->numPages = 0; 
    bm->pageFile = NULL;
    bm->mgmtData = NULL;
    return RC_OK;
}

//causes all dirty pages (with fix count 0) from the buffer pool to be written to disk.
RC forceFlushPool(BM_BufferPool *const bm)
{
    buffer *queue = bm->mgmtData;
    
   //open page file
    RC rc_return_value = openPageFile(bm->pageFile, &bm->fH);
    if (rc_return_value!=RC_OK) return rc_return_value;
    
    frame *framepointer = queue->head;//set framepointer as a head
    do
    {
        if (framepointer->dirty) //check condition
        {
            rc_return_value = writeBlock(framepointer->currpage, &bm->fH, framepointer->data);//write block into file
            if (rc_return_value!=RC_OK) return rc_return_value;
            framepointer->dirty = false;//set false
            queue->numWrite++;//increase writeIO
        }
        framepointer = framepointer->next;
    }while (framepointer!=queue->head);
    
    closePageFile(&bm->fH);//close page file
    return RC_OK;
}

/*------------------------------Buffer Manager Interface Pool Handling functions ends ----------------------------------*/

/*------------------------------Buffer Manager Interface Access Pages functions begins----------------------------------*/
//marks a page as a dirty
RC markDirty (BM_BufferPool *const bm, BM_PageHandle *const page)
{
    buffer *queue = bm->mgmtData;
    frame *framepointer = queue->head;
    
    //find the page number
    while (framepointer->currpage!=page->pageNum)
    {
        framepointer=framepointer->next;
        if (framepointer==queue->head)//if framepinter is head then non existing page
            return RC_READ_NON_EXISTING_PAGE;
    }
    
    framepointer->dirty = true;//set page as a dirty
    return RC_OK;
}
//unpins the page
RC unpinPage (BM_BufferPool *const bm, BM_PageHandle *const page)
{
    buffer *queue = bm->mgmtData;
    frame *framepointer = queue->head;
    
    //find the page number
    while (framepointer->currpage!=page->pageNum)
    {
        framepointer=framepointer->next;
        if (framepointer==queue->head) //if framepinter is head then non existing page
            return RC_READ_NON_EXISTING_PAGE;
    }
    
    //unpins the page
    if (framepointer->fixCount > 0)
    {
        framepointer->fixCount--;
        if (framepointer->fixCount == 0)
            framepointer->refbit = false;//set false for clock
    }
    else
        return RC_READ_NON_EXISTING_PAGE;
    
    return RC_OK;
}

//write the current content of the page back to the page file on disk
RC forcePage (BM_BufferPool *const bm, BM_PageHandle *const page)
{
    //current frame2file
    buffer *queue = bm->mgmtData;
    RC rc_return_value;
    
    //open pag file
    rc_return_value = openPageFile(bm->pageFile, &bm->fH);
    if (rc_return_value!=RC_OK) return RC_FILE_NOT_FOUND;
    
    //write block into file
    rc_return_value = writeBlock(page->pageNum, &bm->fH, page->data);
    if (rc_return_value!=RC_OK)
    {
        closePageFile(&bm->fH);//close page file
        return RC_FILE_NOT_FOUND;
    }
    
    queue->numWrite++;
    closePageFile(&bm->fH);
    return RC_OK;
}

//pins the page with page number
RC pinPage (BM_BufferPool *const bm, BM_PageHandle *const page,const PageNumber pageNum)
{
    if (pageNum<0) return RC_IM_KEY_NOT_FOUND;
    //decide which replacement strategy should it pick
    switch (bm->strategy)
    {
        case RS_FIFO:
            return pinFIFO(bm,page,pageNum,false);//pins the page with page number with FIFO replacement strategy
            break;
        case RS_LRU:
            return pinLRU(bm,page,pageNum);//pins the page with page number with LRU replacement strategy
            break;
        case RS_CLOCK:
            return pinCLOCK(bm,page,pageNum);//pins the page with page number with clock replacement strategy
            break;
        case RS_LRU_K:
            return pinLRUK(bm,page,pageNum);//pins the page with page number with LRU_K replacement strategy
            break;
        default:
            return RC_IM_KEY_NOT_FOUND;
            break;
    }
    return RC_OK;
}
/*------------------------------Buffer Manager Interface Access Pages ends----------------------------------*/

/* -------------------------Statistics Interface functions begins----------------------*/
//returns an array of PageNumbers (of size numPages) where the ith element is the number of the page stored in the ith page frame
PageNumber *getFrameContents (BM_BufferPool *const bm)
{
    PageNumber *fc = calloc(bm->numPages, sizeof(int)); //need free alpha
    buffer *queue = bm->mgmtData;
    statlist *statelisthead = queue->stathead;
    int i;
    for (i=0; i<bm->numPages; i++)
    {
        fc[i]=statelisthead->fpt->currpage;
        statelisthead=statelisthead->next;
    }
    return fc;
}

//returns an array of bools (of size numPages) where the ith element is TRUE if the page stored in the ith page frame is dirty.
bool *getDirtyFlags (BM_BufferPool *const bm)
{
    bool *df = calloc(bm->numPages, sizeof(bool)); //need free beta
    buffer *queue = bm->mgmtData;
    statlist *statelisthead = queue->stathead;
    int i;
    for (i=0; i<bm->numPages; i++)
    {
        if (statelisthead->fpt->dirty)
            df[i]=true;
        statelisthead=statelisthead->next;
    }
    return df;
}
//returns an array of ints (of size numPages) where the ith element is the fix count of the page stored in the ith page frame
int *getFixCounts (BM_BufferPool *const bm)
{
    PageNumber *fc = calloc(bm->numPages, sizeof(int)); //need free gamma
    buffer *queue = bm->mgmtData;
    statlist *statelisthead = queue->stathead;
    int i;
    for (i=0; i<bm->numPages; i++)
    {
        fc[i]=statelisthead->fpt->fixCount;
        statelisthead=statelisthead->next;
    }
    return fc;
}

//returns the number of pages that have been read from disk since a buffer pool has been initialized
int getNumReadIO (BM_BufferPool *const bm)
{
    buffer *queue = bm->mgmtData;
    return queue->numRead;
}
//returns the number of pages written to the page file since the buffer pool has been initialized
int getNumWriteIO (BM_BufferPool *const bm)
{
    buffer *queue = bm->mgmtData;
    return queue->numWrite;
}
/* -------------------------Statistics Interface functions ends----------------------*/
