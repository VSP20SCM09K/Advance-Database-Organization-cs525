#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "storage_mgr.h"
#include "dberror.h"

FILE *file;

/* -----------manipulating  page file methods begins -------------------------*/


void initStorageManager(void) {
	printf("\n <---------------Initializing Storage Manager----------------->\n ");
	

}

//Creating Page file
RC createPageFile(char *fileName) {
	char *memoryBlock = malloc(PAGE_SIZE * sizeof(char)); //Initialized Memory Block with size PAGE_SIZE
	RC returncode;
	file = fopen(fileName, "w+"); //Opening the file in write mode having name as filename
	
	if (file != NULL)
	{
		memset(memoryBlock, '\0', PAGE_SIZE); //If file exists then Setting the allocated memory block by \0 using memset functions
		fwrite(memoryBlock, sizeof(char), PAGE_SIZE, file);	//Writing the allocated memory block in the file
		free(memoryBlock);		//Freeing the memoryBlock after writing
		fclose(file);			//Closing file after creating is done
		returncode = RC_OK; // If file is created then send returncode as RC_OK  
	}	
	else 
		returncode = RC_FILE_NOT_FOUND; // If file is not found then send return code as RC_FILE_NOT_FOUND

	return returncode;

}

//Opening Page file
RC openPageFile(char *fileName, SM_FileHandle *fHandle) {
	file = fopen(fileName, "r+");		//Opening the file in read mode

	if (file != NULL)
	{
		fseek(file, 0, SEEK_END);//To point the file pointer to the last location of the file fseek function is used

		int EndByte = ftell(file); 		//ftell function returns the last byte of file
		int TotalLength = EndByte + 1; // calculating the total length of the file
		int TotalNumberofPages = TotalLength / PAGE_SIZE; //Total number of pages in the file

		//Initializing the file attributes like fileName, total Number of Pages and current Page Position
		(*fHandle).fileName = fileName; 
		(*fHandle).totalNumPages = TotalNumberofPages;
		(*fHandle).curPagePos = 0;

		rewind(file); //rewind function sets the file pointer back to the start of the file
		return RC_OK;
	}	
	else 
		return RC_FILE_NOT_FOUND; // If file is not found then send return code as RC_FILE_NOT_FOUND

}

//Closing Page file
RC closePageFile(SM_FileHandle *fHandle) {
	RC isFileClosed;
	isFileClosed = fclose(file);//fclose closes the file successfully and returns 0
	if (isFileClosed != 0)
		return RC_FILE_NOT_FOUND;
	else
		return RC_OK;
}

//Destroying Page file
RC destroyPageFile(char *fileName) {

	if (remove(fileName) == 0)//remove will delete the file and return 0 if successful
		return RC_OK;
	else
		return RC_FILE_NOT_FOUND;
}

/* -----------manipulating  page file methods ends ------------------------ */







/* -----------------------------------------------methods to read blocks from disc begins ------------------------------------------------------------------------------*/

//Reading pageNum^th block from the file
RC readBlock(int pageNum, SM_FileHandle *fHandle, SM_PageHandle memPage) {
	RC returncode;
	RC read_blocksize;

	if (pageNum > (*fHandle).totalNumPages)	//If the page number is greater than the total no. of pages then throw an error
		returncode = RC_READ_NON_EXISTING_PAGE;

	else {
		fseek(file, pageNum * PAGE_SIZE, SEEK_SET); //To point the file pointer to the begining of the file fseek function is used
		read_blocksize = fread(memPage, sizeof(char), PAGE_SIZE, file);

		//If the size of the block returned by fread() is not within the limit of the Page size then throw an error
		if (read_blocksize < PAGE_SIZE || read_blocksize > PAGE_SIZE) {
			returncode = RC_READ_NON_EXISTING_PAGE;
		}

		(*fHandle).curPagePos = pageNum;//Update current page position to pageNum value
		returncode = RC_OK;
	}

	return returncode;
}

//Get current block position
int getBlockPos(SM_FileHandle *fHandle) {
	RC block_position;
	block_position = ((*fHandle).curPagePos);	//get current page position
	return block_position;
}

//Reading first block from the file
RC readFirstBlock(SM_FileHandle *fHandle, SM_PageHandle memPage) {
	RC returncode;
	if (fHandle != NULL)
	{
		if ((*fHandle).totalNumPages <= 0)
			returncode = RC_READ_NON_EXISTING_PAGE;
		else 
		{
			fseek(file, 0, SEEK_SET);
			RC read_first_block;
			read_first_block = fread(memPage, sizeof(char), PAGE_SIZE, file); //fread returns the first block of size PAGE_SIZE
			(*fHandle).curPagePos = 0; //First page of the file has index 0

			if (read_first_block < 0 || read_first_block > PAGE_SIZE) //If the read block is not within pagesize limits, throw an error
				returncode = RC_READ_NON_EXISTING_PAGE;

			returncode = RC_OK;
		}
	}
	else 
		returncode = RC_FILE_NOT_FOUND;

	return returncode;
}

//Reading previous block in file
RC readPreviousBlock(SM_FileHandle *fHandle, SM_PageHandle memPage) {
	RC returncode;

	if (fHandle != NULL)
	{

		RC previous_block_position;
		RC read_previous_block;

		previous_block_position = (*fHandle).curPagePos - 1; //storing the index of previous block i.e 1 less than current page position

		if (previous_block_position < 0)
			returncode = RC_READ_NON_EXISTING_PAGE;

		else 
		{
			fseek(file, (previous_block_position * PAGE_SIZE), SEEK_SET); //fseek will point the file pointer to the start of the previous block
			read_previous_block = fread(memPage, sizeof(char), PAGE_SIZE, file); //fread returns the read block
			(*fHandle).curPagePos = (*fHandle).curPagePos - 1; //Update the current page position reducing it by 1

			if (read_previous_block < 0 || read_previous_block > PAGE_SIZE) //If the previous block is not within pagesize limits, throw an error
				returncode = RC_READ_NON_EXISTING_PAGE;

			returncode = RC_OK;
		}
	}
	else 
		returncode = RC_FILE_NOT_FOUND;

	return returncode;
}

//Reads the current block which the file pointer points to
RC readCurrentBlock(SM_FileHandle *fHandle, SM_PageHandle memPage) {
	RC returncode;

	if (fHandle != NULL)	
	{
		RC current_block_position;
		RC read_current_block;

		current_block_position = getBlockPos(fHandle); //get the position of the current block to read
		fseek(file, (current_block_position * PAGE_SIZE), SEEK_SET); //fseek will point the file pointer to the start of the current block
		read_current_block = fread(memPage, sizeof(char), PAGE_SIZE, file); //fread returns the current block to be read
		(*fHandle).curPagePos = current_block_position;

		if (read_current_block < 0 || read_current_block > PAGE_SIZE) //If the current block is not within pagesize limits, throw an error
			returncode = RC_READ_NON_EXISTING_PAGE;

		returncode = RC_OK;
	}
	else //If the fhandle is null, throw an error
		returncode = RC_FILE_HANDLE_NOT_INIT;


	return returncode;
}

//Reading next block of the file
RC readNextBlock(SM_FileHandle *fHandle, SM_PageHandle memPage) {
	RC returncode;

	if (fHandle != NULL)
	{
		RC next_block_position;
		RC read_next_block;

		next_block_position = (*fHandle).curPagePos + 1; //get the position of next block 
		if (next_block_position < (*fHandle).totalNumPages) 
		{

			fseek(file, (next_block_position * PAGE_SIZE), SEEK_SET); //fseek points the file pointer to the next block
			read_next_block = fread(memPage, sizeof(char), PAGE_SIZE, file); //fread returns the next block to be read

			(*fHandle).curPagePos = next_block_position; //Update current page position to next block index

			if (read_next_block < 0 || read_next_block > PAGE_SIZE) //If the next block is not within pagesize limits, throw an error
				returncode = RC_READ_NON_EXISTING_PAGE;

			returncode = RC_OK;
		}
		else
			return RC_READ_NON_EXISTING_PAGE;

	}
	else
	 	returncode = RC_FILE_HANDLE_NOT_INIT;

	return returncode;
}


//Reading the last block of the file
RC readLastBlock(SM_FileHandle *fHandle, SM_PageHandle memPage) {
	RC returncode;

	if (fHandle != NULL)
	{
		RC find_last_block_position;
		RC read_last_block;

		find_last_block_position = (*fHandle).totalNumPages - 1; //storing the index of the Last block i.e 1 less than total no. of pages

		fseek(file, (find_last_block_position * PAGE_SIZE), SEEK_SET); //fseek points the file pointer to the beginning of the last block
		read_last_block = fread(memPage, sizeof(char), PAGE_SIZE, file); //fread returns the last block to be read


		(*fHandle).curPagePos = find_last_block_position; //Update current page position to the last block index

		if (read_last_block < 0 || read_last_block > PAGE_SIZE) //If the last block is not within pagesize limits, throw an error
			returncode = RC_READ_NON_EXISTING_PAGE;

		returncode = RC_OK;
	}
	else 
		returncode = RC_FILE_NOT_FOUND;

	return returncode;
}
/* --------------------------------methods to read blocks from disc ends----------------------------------------------------------------------------------------- */








/* --------------------------------methods to write blocks to page file begins----------------------------------------------------------------------------------------- */
//Writing block at page pageNum
RC writeBlock(int pageNum, SM_FileHandle *fHandle, SM_PageHandle memPage) {
	RC returncode;
	if (pageNum < 0 || pageNum > (*fHandle).totalNumPages) //If the pageNum is greater than totalNumber of pages, or less than zero, throw an error
		returncode = RC_WRITE_FAILED;

	else

	if (fHandle != NULL)
	{
		if (file != NULL) 
		{ 
			if (fseek(file, (PAGE_SIZE * pageNum), SEEK_SET) == 0) 
			{
				fwrite(memPage, sizeof(char), PAGE_SIZE, file);

				(*fHandle).curPagePos = pageNum; //Update current page position to the pageNum i.e. index of the block written

				fseek(file, 0, SEEK_END);
				(*fHandle).totalNumPages = ftell(file) / PAGE_SIZE; //update the value of totalNumPages which increases by 1

				returncode = RC_OK;
			}
			else 
				returncode = RC_WRITE_FAILED;
			
		} 
		else //If file is not found, throw an error
			returncode = RC_FILE_NOT_FOUND;
		
	}
	else
		returncode = RC_FILE_HANDLE_NOT_INIT;
 
	return returncode;

}

//Writing current block in the file
RC writeCurrentBlock(SM_FileHandle *fHandle, SM_PageHandle memPage) {
	RC current_position;
	RC write_current_block;

	current_position = getBlockPos(fHandle); //get current block position
	write_current_block = writeBlock(current_position, fHandle, memPage); //call the writeBlock function to write block at current position

	if (write_current_block == RC_OK)
		return RC_OK;

	else
		return RC_WRITE_FAILED;

}

//Append empty block to the file
RC appendEmptyBlock(SM_FileHandle *fHandle) {

	int returncode;
	if (file != NULL)
	{
		RC size = 0;

		char *newemptyblock;
		newemptyblock = (char *) calloc(PAGE_SIZE, sizeof(char)); //Create and initialize the new empty block

		fseek(file, 0, SEEK_END); //To point the file pointer to the end of the file fseek function is used
		size = fwrite(newemptyblock, 1, PAGE_SIZE, file); // it will write the emptyblock at the end of the file

		if (size == PAGE_SIZE)
		{
			(*fHandle).totalNumPages = ftell(file) / PAGE_SIZE; //update total no. of pages i.e. increase by 1
			(*fHandle).curPagePos = (*fHandle).totalNumPages - 1; //update current page position i.e. index is 1 less than totalNumPages
			returncode = RC_OK;
		}
		else 
			returncode = RC_WRITE_FAILED;

		free(newemptyblock);

	} 
	else 
		returncode = RC_FILE_NOT_FOUND;
	
	return returncode;
}

//Ensure capacity of the file is numberOfPages
RC ensureCapacity(int numberOfPages, SM_FileHandle *fHandle) {

	int PageNumber = (*fHandle).totalNumPages;
	int count;

	if (numberOfPages > PageNumber) //If the new capacity is greater than current capacity
	{ 
		int add_page = numberOfPages - PageNumber;
		for (count = 0; count < add_page; count++) //Add add_page no. of pages by calling the appendEmptyBlock function
			appendEmptyBlock(fHandle);

		return RC_OK;
	}
	else
		return RC_WRITE_FAILED;
}
/* --------------------------------methods to write blocks to page file ends----------------------------------------------------------------------------------------- */
