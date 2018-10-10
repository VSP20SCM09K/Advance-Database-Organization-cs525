#include "buffer_mgr.h"
#include "dberror.h"
#include "storage_mgr.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define RC_QUEUE_IS_EMPTY 5;
#define RC_NO_FREE_BUFFER_ERROR 6;

SM_FileHandle *filehandler;
Queue *queue;
int readIO;
int writeIO;


RC pinPageLRU(BM_BufferPool * const bm, BM_PageHandle * const page,const PageNumber pageNum);  //pins the page with page number with LRU replacement strategy
RC pinPageFIFO(BM_BufferPool *const bm, BM_PageHandle *const page,const PageNumber pageNum); // pins the page with page number with FIFO replacement strategy


//creating a queue for frame list
void createQueue(BM_BufferPool *const bm)
{
	pageInfo *newPage[bm->numPages];
	int lastPage = (bm->numPages) - 1;
	int n;
	for (n = 0; n <= lastPage; n++) {
		newPage[n] = (pageInfo*) malloc(sizeof(pageInfo));
	}
	for (n = 0; n <= lastPage; n++) {
		newPage[n]->frameNum = n;
		newPage[n]->isDirty = 0;
		newPage[n]->fixCount = 0;
		newPage[n]->pageNum = -1;
		newPage[n]->bufferData = (char*) calloc(PAGE_SIZE, sizeof(char));
	}
	int i;
	for (i = 0; i <= lastPage; i++) {
		int n = i;
		if (n == 0)
		{
			newPage[n]->prevPageInfo = NULL;
			newPage[n]->nextPageInfo = newPage[n + 1];
		}

		else if (n == lastPage) {
			newPage[n]->nextPageInfo = NULL;
			newPage[n]->prevPageInfo = newPage[n - 1];
		}
		else {

			newPage[n]->nextPageInfo = newPage[n + 1];
			newPage[n]->prevPageInfo = newPage[n - 1];
		}
	}
	queue->head = newPage[0];
	queue->tail = newPage[lastPage];
	queue->filledframes = 0;
	queue->totalNumOfFrames = bm->numPages;
}

//emptying the queue
RC emptyQueue() {return (queue->filledframes == 0);}

//creating the new list for page
pageInfo* createNewList(const PageNumber pageNum) 
{
	pageInfo* newpinfo = (pageInfo*) malloc(sizeof(pageInfo));
	char *c = (char*) calloc(PAGE_SIZE, sizeof(char));

	newpinfo->pageNum = pageNum;
	newpinfo->isDirty = 0;
	newpinfo->frameNum = 0;
	newpinfo->fixCount = 1;
	newpinfo->bufferData = c;
	newpinfo->prevPageInfo=NULL;
	newpinfo->nextPageInfo = NULL;
	
	return newpinfo;
}
//dequeing the framelist
RC deQueue() 
{
	if (emptyQueue())
	{
		return RC_QUEUE_IS_EMPTY;
	}

	pageInfo *p = queue->head;
	for (int i = 0; i < queue->filledframes; i++)
	{
		if (i == (queue->filledframes-1))
		{
			queue->tail = p;
		} 
		else
			p = p->nextPageInfo;
	}
	

	int tail_pnum; 
	int pageDelete=0;
	pageInfo *pinfo = queue->tail;
	for (int i = 0; i < queue->totalNumOfFrames; i++)
	{

		if ((pinfo->fixCount) == 0)
		{

			if (pinfo->pageNum == queue->tail->pageNum)
			{
				pageDelete=pinfo->pageNum;
				queue->tail = (queue->tail->prevPageInfo);
				queue->tail->nextPageInfo = NULL;
 
			}
			else
			{
				pageDelete=pinfo->pageNum;
				pinfo->prevPageInfo->nextPageInfo = pinfo->nextPageInfo;
				pinfo->nextPageInfo->prevPageInfo = pinfo->prevPageInfo;
			}

		}
		else 
		{
			tail_pnum=pinfo->pageNum;
			//printf("\n next node%d", pinfo->pageNum);
			pinfo = pinfo->prevPageInfo;

		}
	}

	if (tail_pnum == queue->tail->pageNum)
	{	
		return 0;		//Add error
	}

	if (pinfo->isDirty == 1) 
	{
		writeBlock(pinfo->pageNum, filehandler, pinfo->bufferData);	
		writeIO++;
	}

	queue->filledframes--;
	return pageDelete;
}

//enqueing the page frame
RC Enqueue(BM_PageHandle * const page, const PageNumber pageNum,BM_BufferPool * const bm) 
{

	int pageDelete=-1;
	if (queue->filledframes == queue->totalNumOfFrames ) { //If frames are full remove a page
		pageDelete=deQueue();
	}

	pageInfo* pinfo = createNewList(pageNum);
	//pinfo->bufferData=bm->mgmtData;

	if (emptyQueue()) {

		readBlock(pinfo->pageNum,filehandler,pinfo->bufferData);
		page->data = pinfo->bufferData;
		readIO++;

		pinfo->frameNum = queue->head->frameNum;
		pinfo->nextPageInfo = queue->head;
		queue->head->prevPageInfo = pinfo;
		pinfo->pageNum = pageNum;
		page->pageNum= pageNum;
		queue->head = pinfo;
		

	} else {  
		readBlock(pageNum, filehandler, pinfo->bufferData);
		if(pageDelete==-1)
			pinfo->frameNum = queue->head->frameNum+1;
		else
			pinfo->frameNum=pageDelete;
		page->data = pinfo->bufferData;
		readIO++;
		pinfo->nextPageInfo = queue->head;
		queue->head->prevPageInfo = pinfo;
		queue->head = pinfo;
		page->pageNum= pageNum;
		

	}
	queue->filledframes++;

	return RC_OK; 
}

//pins the page with page number with LRU replacement strategy
RC pinPageLRU(BM_BufferPool * const bm, BM_PageHandle * const page,const PageNumber pageNum)
{

	int pageFound = 0;
	pageInfo *pinfo = queue->head;
	int i;
	//find the node with given page number
	for ( i= 0; i < bm->numPages; i++)
	{
		if (pageFound == 0) 
		{
			if (pinfo->pageNum == pageNum)
			{
				pageFound = 1;
				break;
			}
			else
				pinfo = pinfo->nextPageInfo;
		}
	}

	if (pageFound == 0)
		Enqueue(page,pageNum,bm);

	//provide the client with the data and details of page and if pinned then increase the fix count
	if (pageFound == 1)
	{
		pinfo->fixCount++;
		page->data = pinfo->bufferData;
		page->pageNum=pageNum;
		
		if (pinfo == queue->head)
		{
			pinfo->nextPageInfo = queue->head;
			queue->head->prevPageInfo = pinfo;
			queue->head = pinfo;
		}
		// To make the given node the head of the list
		if (pinfo != queue->head)
		{
			pinfo->prevPageInfo->nextPageInfo = pinfo->nextPageInfo;
			if (pinfo->nextPageInfo)
			{
				pinfo->nextPageInfo->prevPageInfo = pinfo->prevPageInfo;

				if (pinfo == queue->tail)
				{
					queue->tail = pinfo->prevPageInfo;
					queue->tail->nextPageInfo = NULL;
				}
				
				pinfo->nextPageInfo = queue->head;
				pinfo->prevPageInfo = NULL;
				pinfo->nextPageInfo->prevPageInfo = pinfo;
				queue->head = pinfo;
			}
		}
	}
	
	return RC_OK;
}

//pins the page with page number
RC pinPage(BM_BufferPool * const bm, BM_PageHandle * const page,const PageNumber pageNum)
{
	int res;
	if(bm->strategy==RS_FIFO)
		res=pinPageFIFO(bm,page,pageNum);
	else
		res=pinPageLRU(bm,page,pageNum);
	return res;
}

//
void updateBM_BufferPool(BM_BufferPool *const bm, const char *const pageFileName, const int numPages, ReplacementStrategy strategy)
{

	 char* buffersize = (char *)calloc(numPages,sizeof(char)*PAGE_SIZE);

	   bm->pageFile = (char *)pageFileName;
	   bm->numPages = numPages;
	   bm->strategy = strategy;
	   bm->mgmtData = buffersize;
	
}

//creates a new buffer pool with numPages page frames using the page replacement strategy
RC initBufferPool(BM_BufferPool *const bm, const char *const pageFileName, const int numPages, ReplacementStrategy strategy, void *stratData)
{
       readIO = 0;
	   writeIO = 0;
	   filehandler = (SM_FileHandle *)malloc(sizeof(SM_FileHandle));
	   queue = (Queue *)malloc(sizeof(Queue));

	  updateBM_BufferPool(bm,pageFileName,numPages,strategy);

	  openPageFile(bm->pageFile,filehandler);

	  createQueue(bm);

	   return RC_OK;

}

//destroys a buffer pool
RC shutdownBufferPool(BM_BufferPool *const bm)
{
	int i, returncode = -1;
	pageInfo *pinfo=NULL,*temp=NULL;

	pinfo=queue->head;
	for(i=0; i< queue->filledframes ; i++)
	{
		if(pinfo->fixCount==0 && pinfo->isDirty == 1)
		{
			writeBlock(pinfo->pageNum,filehandler,pinfo->bufferData);
			writeIO++;
			pinfo->isDirty=0;
			}
			pinfo=pinfo->nextPageInfo;		
	}
	closePageFile(filehandler);
	
	return RC_OK;
}

//causes all dirty pages (with fix count 0) from the buffer pool to be written to disk.
RC forceFlushPool(BM_BufferPool *const bm)
{
	int i;
	pageInfo *temp1;
	temp1 = queue->head;
	for(i=0; i< queue->totalNumOfFrames; i++)
	{
		if((temp1->isDirty==1) && (temp1->fixCount==0))
		{
			writeBlock(temp1->pageNum,filehandler,temp1->bufferData);
			writeIO++;
			temp1->isDirty=0;
			

		}

		temp1=temp1->nextPageInfo;
	}
	return RC_OK;
}

//unpins the page 
RC unpinPage (BM_BufferPool *const bm, BM_PageHandle *const page)
{
	pageInfo *temp;
	int i;
	temp = queue->head;
	
	for(i=0; i < bm->numPages; i++){
		if(temp->pageNum==page->pageNum)
			break;
		temp=temp->nextPageInfo;
	}
	
	if(i == bm->numPages)
		return RC_READ_NON_EXISTING_PAGE;		
	else
		temp->fixCount=temp->fixCount-1;
	return RC_OK;
}

//write the current content of the page back to the page file on disk
RC forcePage (BM_BufferPool *const bm, BM_PageHandle *const page)
{
	pageInfo *temp;
	int i;
	temp = queue->head;
	
	for(i=0; i < bm->numPages; i++){
		if(temp->pageNum==page->pageNum)
			break;
		temp=temp->nextPageInfo;
	}
	
	int flag;

	if(i == bm->numPages)
		return 1;          //give error code
	if((flag=writeBlock(temp->pageNum,filehandler,temp->bufferData))==0)
		writeIO++;
	else
		return RC_WRITE_FAILED;

	return RC_OK;
}

//marks a page as a dirtyx`
RC markDirty (BM_BufferPool *const bm, BM_PageHandle *const page)
{
	pageInfo *temp;
	int i;
	temp = queue->head;
	
	for(i=0; i < bm->numPages; i++){
		if(temp->pageNum==page->pageNum)
			break;
		if(temp->nextPageInfo!=NULL)
		temp=temp->nextPageInfo;
	}
	
	if(i == bm->numPages)
	return RC_READ_NON_EXISTING_PAGE;
	temp->isDirty=1;
	return RC_OK;
}

//------------Statistics Functions-----------------

//returns an array of PageNumbers (of size numPages) where the ith element is the number of the page stored in the ith page frame
PageNumber *getFrameContents (BM_BufferPool *const bm)
{
	PageNumber (*pages)[bm->numPages];
	int i;
	pages=calloc(bm->numPages,sizeof(PageNumber));
	pageInfo *temp;
	for(i=0; i< bm->numPages;i++)
	{	
       for(temp=queue->head ; temp!=NULL; temp=temp->nextPageInfo)
       {
	           if(temp->frameNum ==i)
	           {
		       		(*pages)[i] = temp->pageNum;
			   		break;
				}
			}
		}
	return *pages;
}

//returns an array of bools (of size numPages) where the ith element is TRUE if the page stored in the ith page frame is dirty.
bool *getDirtyFlags (BM_BufferPool *const bm)
{
	bool (*isDirty)[bm->numPages];
	int i;
	isDirty=calloc(bm->numPages,sizeof(PageNumber));
	pageInfo *temp;
	
	for(i=0; i< bm->numPages ;i++)
	{
		for(temp=queue->head ; temp!=NULL; temp=temp->nextPageInfo)
		{
           if(temp->frameNum ==i)
           {
				if(temp->isDirty==1)
					(*isDirty)[i]=TRUE;
				else
					(*isDirty)[i]=FALSE;
				break;
			}
		}
	}
	return *isDirty;

}

//returns an array of ints (of size numPages) where the ith element is the fix count of the page stored in the ith page frame
int *getFixCounts (BM_BufferPool *const bm)
{
	int (*fixCounts)[bm->numPages];
	int i;
	fixCounts=calloc(bm->numPages,sizeof(PageNumber));
	pageInfo *temp;

	for(i=0; i< bm->numPages;i++){	
       for(temp=queue->head ; temp!=NULL; temp=temp->nextPageInfo){
	           if(temp->frameNum ==i){
		       (*fixCounts)[i] = temp->fixCount;
			   break;
				}
			}
		}
	return *fixCounts;
}

//returns the number of pages that have been read from disk since a buffer pool has been initialized
int getNumReadIO (BM_BufferPool *const bm)
{
	return readIO;
}

//returns the number of pages written to the page file since the buffer pool has been initialized
int getNumWriteIO (BM_BufferPool *const bm)
{
	return writeIO;
}

//pins the page with page number with FIFO replacement strategy
RC pinPageFIFO(BM_BufferPool *const bm, BM_PageHandle *const page,const PageNumber pageNum)
{
		int pageFound = 0,numPages;
		numPages = bm->numPages;
			pageInfo *list=NULL,*temp=NULL,*temp1=NULL;
			list = queue->head;

			//find the node with given page number
			for (int i = 0; i < numPages; i++)
			{
				if (pageFound == 0) 
				{
					if (list->pageNum == pageNum)
					{
						pageFound = 1;
						break;
					}
					else
					list = list->nextPageInfo;
				}
			}

			//provide the client with the data and details of page and if pinned then increase the fix count
			if (pageFound == 1 )
			{
				list->fixCount++;
				page->data = list->bufferData;
				page->pageNum = pageNum;

				//free(list);
				return RC_OK;
			}
			//free(list);
				temp = queue->head;
				int returncode = -1;
				
			//frames in the memory are less than the total available frames, find out the first free frame from the head
			while (queue->filledframes < queue->totalNumOfFrames)
			{
				if(temp->pageNum == -1)
				{
					temp->fixCount = 1;
					temp->isDirty = 0;
					temp->pageNum = pageNum;
					page->pageNum= pageNum;
					
					queue->filledframes = queue->filledframes + 1 ;
					
					readBlock(temp->pageNum,filehandler,temp->bufferData);
					
					page->data = temp->bufferData;
					readIO++;
					returncode = 0;
					break;
 				}
				else
					temp = temp->nextPageInfo;
			}
			
			if(returncode == 0)
				return RC_OK;
		
		
		    // To create a new node
			pageInfo *addnode = (pageInfo *) malloc (sizeof(pageInfo));
			addnode->fixCount = 1;
			addnode->isDirty = 0;
			addnode->pageNum = pageNum;
			addnode->bufferData = NULL;
			addnode->nextPageInfo = NULL;
			page->pageNum= pageNum;
			addnode->prevPageInfo = queue->tail;
			temp = queue->head;

			int i;
			
			for(i=0; i<numPages ;i++)
			{
				if((temp->fixCount)== 0)
					break;
	        	else
					temp = temp->nextPageInfo;
			}

			if(i==numPages)
			{
				return RC_NO_FREE_BUFFER_ERROR;
			}

			temp1=temp;

			// To make the given node the head of the list
			if(temp == queue->head)
			{

				queue->head = queue->head->nextPageInfo;
				queue->head->prevPageInfo = NULL;

			}
			else if(temp == queue->tail)
			{
				queue->tail = temp->prevPageInfo;
				addnode->prevPageInfo=queue->tail;
			}
			else
			{
				temp->prevPageInfo->nextPageInfo = temp->nextPageInfo;
				temp->nextPageInfo->prevPageInfo=temp->prevPageInfo;
			}

			// If the frame to be replaced is dirty, write it back to the disk.
			if(temp1->isDirty == 1)
			{
				writeBlock(temp1->pageNum,filehandler,temp1->bufferData);
			 	writeIO++;
			}

			addnode->bufferData = temp1->bufferData;
			addnode->frameNum = temp1->frameNum;
			
			readBlock(pageNum,filehandler,addnode->bufferData);
			page->data = addnode->bufferData;
			readIO++;
			
			queue->tail->nextPageInfo = addnode;
			queue->tail=addnode;
            
            return RC_OK;

}