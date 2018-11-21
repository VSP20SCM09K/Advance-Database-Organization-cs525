#include "storage_mgr.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

void initStorageManager (void){
    
}
// File Manupulation //
RC openPageFile (char *fName, SM_FileHandle *fHandle)// this function corresponds to opening a file
{
    FILE *pF = fopen(fName,"r+");
    if (!pF)
        return RC_FILE_NOT_FOUND;
    //write fHandle
    int tot_pg;
    fscanf(pF,"%d\n",&tot_pg);
    
    fHandle->fileName=fName;
    fHandle->totalNumPages=tot_pg; 
    fHandle->curPagePos=0;  
    fHandle->mgmtInfo=pF; 
    return RC_OK;
}

RC createPageFile (char *fName)
{
    FILE *pF = fopen(fName,"w");
    
    SM_PageHandle str = malloc(PAGE_SIZE); 
    if (str==NULL)
        return RC_WRITE_FAILED;
    memset(str,'\0',PAGE_SIZE); 
    
    fprintf(pF,"%d\n",1); // give the pages as 1
    fwrite(str, sizeof(char), PAGE_SIZE, pF);
    fclose(pF);
    
    free(str);
    str=NULL;
    return RC_OK;
}

RC closePageFile (SM_FileHandle *fHandle)
{
    int no =fclose(fHandle->mgmtInfo); 
    if (no)
        return RC_FILE_HANDLE_NOT_INIT;
    

    return RC_OK;
}

RC destroyPageFile (char *fName)
{
    int no =remove(fName); 
    if (no)
        return RC_FILE_NOT_FOUND;
    
    return RC_OK;
}

// functions for reading //

RC readBlock (int pNum, SM_FileHandle *fHandle, SM_PageHandle mPage)
{
    if (pNum>fHandle->totalNumPages || pNum<0)
        return RC_READ_NON_EXISTING_PAGE;
    if (fHandle->mgmtInfo==NULL)
        return RC_FILE_NOT_FOUND;
    
    fseek(fHandle->mgmtInfo, 5+pNum*PAGE_SIZE, SEEK_SET);
    fread(mPage, sizeof(char), PAGE_SIZE, fHandle->mgmtInfo); //return total number of elements
    fHandle->curPagePos=pNum;
    
    return RC_OK;
}

int getBlockPos (SM_FileHandle *fHandle){
    return fHandle->curPagePos;
}
RC readCurrentBlock (SM_FileHandle *fHandle, SM_PageHandle mPage){
    return readBlock(fHandle->curPagePos, fHandle, mPage);
}
RC readPreviousBlock (SM_FileHandle *fHandle, SM_PageHandle mPage){
    return readBlock(fHandle->curPagePos-1, fHandle, mPage);
}
RC readFirstBlock (SM_FileHandle *fHandle, SM_PageHandle mPage){
    return readBlock(0, fHandle, mPage);
}
RC readNextBlock (SM_FileHandle *fHandle, SM_PageHandle mPage){
    return readBlock(fHandle->curPagePos+1, fHandle, mPage);
}
RC readLastBlock (SM_FileHandle *fHandle, SM_PageHandle mPage){
    return readBlock(fHandle->totalNumPages-1, fHandle, mPage); 
}

// functions for writing//

RC writeBlock (int pNum, SM_FileHandle *fHandle, SM_PageHandle mPage)
{
    if (pNum > (fHandle->totalNumPages) || pNum<0)
        return RC_WRITE_FAILED;
    
    int no = fseek(fHandle->mgmtInfo, 5+pNum*PAGE_SIZE, SEEK_SET);
    if (no)
        return RC_WRITE_FAILED;
    
    fwrite(mPage, sizeof(char), PAGE_SIZE, fHandle->mgmtInfo);
    fHandle->curPagePos = pNum;
    return RC_OK;
}

RC writeCurrentBlock (SM_FileHandle *fHandle, SM_PageHandle mPage)
{
    return writeBlock(fHandle->curPagePos, fHandle, mPage);
}

RC ensureCapacity (int numberOfPages, SM_FileHandle *fHandle)

{
    if (fHandle->totalNumPages < numberOfPages)
    {
        int difference = numberOfPages - fHandle->totalNumPages;
        RC return_value;
        int i; 
        for (i=0; i < difference; i++)
        {
            return_value = appendEmptyBlock(fHandle);
            if (return_value!=RC_OK)
                return return_value;
        }
    }
    return RC_OK;
}



RC appendEmptyBlock (SM_FileHandle *fHandle)
{
    SM_PageHandle str = (char *) calloc(PAGE_SIZE,1); 
    
    RC return_value = writeBlock(fHandle->totalNumPages, fHandle, str);
    
    if (return_value!=RC_OK)
    {
        free(str);
        str=NULL;
        return return_value;
    }
    fHandle->curPagePos=fHandle->totalNumPages;
    fHandle->totalNumPages+=1;
    
    rewind(fHandle->mgmtInfo);//the file pointer is reset
    fprintf(fHandle->mgmtInfo,"%d\n",fHandle->totalNumPages);
    fseek(fHandle->mgmtInfo,5+(fHandle->curPagePos)*PAGE_SIZE,SEEK_SET); //the file pointer is recovered
    free(str);
    str=NULL;
    return RC_OK;
}

