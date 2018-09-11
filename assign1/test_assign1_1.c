#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "storage_mgr.h"
#include "dberror.h"
#include "test_helper.h"

// test name
char *testName;

/* test output files */
#define TESTPF "test_pagefile.bin"

/* prototypes for test functions */
static void testCreateOpenClose(void);
static void testSinglePageContent(void);
/* New test cases */
static void new_test_cases(void); 
static void read_not_there(void);
static void PageTest(void);

/* main function running all tests */
int
main (void)
{
  testName = "";
  
  initStorageManager();
  testCreateOpenClose();
  testSinglePageContent();
  new_test_cases();
  read_not_there();
  PageTest();
  


  return 0;
}


/* check a return code. If it is not RC_OK then output a message, error description, and exit */
/* Try to create, open, and close a page file */
void
testCreateOpenClose(void)
{
  SM_FileHandle fh;

  testName = "test create open and close methods";

  TEST_CHECK(createPageFile (TESTPF));
  
  TEST_CHECK(openPageFile (TESTPF, &fh));
  ASSERT_TRUE(strcmp(fh.fileName, TESTPF) == 0, "filename correct");
  ASSERT_TRUE((fh.totalNumPages == 1), "expect 1 page in new file");
  ASSERT_TRUE((fh.curPagePos == 0), "freshly opened file's page position should be 0");

  TEST_CHECK(closePageFile (&fh));
  TEST_CHECK(destroyPageFile (TESTPF));

  // after destruction trying to open the file should cause an error
  ASSERT_TRUE((openPageFile(TESTPF, &fh) != RC_OK), "opening non-existing file should return an error.");

  TEST_DONE();
}

/* Try to create, open, and close a page file */
void
testSinglePageContent(void)
{ 
  SM_FileHandle fh;
  SM_PageHandle ph;
  int i;

  testName = "test single page content";

  ph = (SM_PageHandle) malloc(PAGE_SIZE);

  // create a new page file
  TEST_CHECK(createPageFile (TESTPF));
  TEST_CHECK(openPageFile (TESTPF, &fh));
  printf("created and opened file\n");
  
  // read first page into handle
  TEST_CHECK(readFirstBlock (&fh, ph));
  // the page should be empty (zero bytes)
  for (i=0; i < PAGE_SIZE; i++)
    ASSERT_TRUE((ph[i] == 0), "expected zero byte in first page of freshly initialized page");
  printf("first block was empty\n");
    
  // change ph to be a string and write that one to disk
  for (i=0; i < PAGE_SIZE; i++)
    ph[i] = (i % 10) + '0';
  TEST_CHECK(writeBlock (0, &fh, ph));
  printf("writing first block\n");

  // read back the page containing the string and check that it is correct
  TEST_CHECK(readFirstBlock (&fh, ph));
  for (i=0; i < PAGE_SIZE; i++)
    ASSERT_TRUE((ph[i] == (i % 10) + '0'), "character in page read from disk is the one we expected.");
  printf("reading first block\n");

  // destroy new page file
  TEST_CHECK(destroyPageFile (TESTPF));  
  
  TEST_DONE();
}

void
new_test_cases(void)
{ int j;
  SM_PageHandle ph;
  SM_FileHandle fh;
  
  

  testName = "To Test the Content of a Single page";

  ph = (SM_PageHandle) malloc(PAGE_SIZE);


    TEST_CHECK(createPageFile (TESTPF));
    TEST_CHECK(openPageFile (TESTPF, &fh));
    printf("created and opened file\n");

    // First block or the page is read
    TEST_CHECK(readFirstBlock (&fh, ph));
    // Since we are dealing with the first page we are asssuming that it contains 0 bytes
    for (j=0; j < PAGE_SIZE; j++)
        ASSERT_TRUE((ph[j] == 0), "First Page so we expect that its going to contain 0 bytes");
    printf("confirmed that the block had nothing\n");

    // Write ph on the disk. NOTE: Assuming that ph is a string
    for (j=0; j < PAGE_SIZE; j++)
        ph[j] = (j % 10) + '0';
    TEST_CHECK(writeBlock (0, &fh, ph));
    printf("writing the first block\n");

    // we need to check if what was written was right or not. So we implement the read 
    TEST_CHECK(readBlock (0,&fh, ph));
    for (j=0; j < PAGE_SIZE; j++)
        ASSERT_TRUE((ph[j] == (j % 10) + '0'), "character in page read from disk is the one we expected");
    printf("reading first block\n");

    // Repeating the above step for the subsequent block
       for (j=0; j < PAGE_SIZE; j++)
           ph[j] = (j % 10) + '0';
       TEST_CHECK(writeBlock (1, &fh, ph));
       printf("writing second block\n");

       // Use the currentblock function and check if it is correct-by reading the block
       TEST_CHECK(readCurrentBlock (&fh, ph));
       for (j=0; j < PAGE_SIZE; j++)
           ASSERT_TRUE((ph[j] == (j % 10) + '0'), "character in the page read from disk is the one we expected");
       printf("Read Current:second block\n");


    // Increase the number of pages in the file by one. The last page should not contain anything.Follows the same logic as the first page during initialization
       TEST_CHECK(appendEmptyBlock (&fh));
       ASSERT_TRUE((fh.totalNumPages == 3), "after appending to a new file, total number of pages should be 3");


     // Previous block is read 
        TEST_CHECK(readPreviousBlock (&fh, ph));
        for (j=0; j < PAGE_SIZE; j++)
           ASSERT_TRUE((ph[j] == (j % 10) + '0'), "character in page read from disk is the one we expected");
          printf("reading Previous(second) block\n");


    // Next Block is read
    TEST_CHECK(readNextBlock (&fh, ph));
    // Page shouldnt contain any bytes since we are adding a new page
    for (j=0; j < PAGE_SIZE; j++)
        ASSERT_TRUE((ph[j] == 0), "expected zero byte in newly appened page");
    printf("reading Next(third) block- the appended one\n");


    // change ph to be a string and write that one to disk on current page
    for (j=0; j < PAGE_SIZE; j++)
        ph[j] = (j % 10) + '0';
    TEST_CHECK(writeCurrentBlock (&fh, ph));
    printf("writing to the block which was appended\n");


    // if the file happens to contain less than the numberOfPages pages; ensure that the max numberOfPages is 5
    TEST_CHECK(ensureCapacity (5, &fh));
    printf("%d\n",fh.totalNumPages);
    ASSERT_TRUE((fh.totalNumPages == 5), "expect 5 pages after ensure capacity");

    //To check if the numberofpages is not exceeding 5
    ASSERT_TRUE((fh.curPagePos == 4), "After appending page position should be 4.(pointing to the last page 5)");

    // now we get to the last page
    TEST_CHECK(readLastBlock (&fh, ph));
    // since again its the beginning of the page we start of with 0 bytes.
    for (j=0; j < PAGE_SIZE; j++)// for loop to ensure that the end is not reached.
        ASSERT_TRUE((ph[j] == 0), "Ensured that there were 0 bytes in the new page");
    printf("Since we are reading the last block it is expected to be empty\n");

  TEST_CHECK(destroyPageFile (TESTPF));
  free(ph);
  TEST_DONE();
}
void read_not_there(void){
    SM_FileHandle fh;
    SM_PageHandle ph;
    

    testName = "test reads something that is not there";

    ph = (SM_PageHandle) malloc(PAGE_SIZE);

    // create a new page file
    TEST_CHECK(createPageFile (TESTPF));
    TEST_CHECK(openPageFile (TESTPF, &fh));
    printf("\ncreated and opened file\n");
    ASSERT_TRUE((readBlock (fh.totalNumPages, &fh, ph) == RC_OK), "Trying to read what isnt there");

    // destroy new page file
    ASSERT_TRUE(destroyPageFile (TESTPF)==RC_OK, "File has been destroyed");


    TEST_DONE();   
}


void PageTest(void){
    SM_PageHandle ph;
	  SM_FileHandle fh;
	  
	  int i;

	  testName = "Testing the content of the second page";

	  ph = (SM_PageHandle) malloc(PAGE_SIZE);

	  // create a new page file
	  TEST_CHECK(createPageFile (TESTPF));
	  TEST_CHECK(openPageFile (TESTPF, &fh));
	  printf("created and opened file\n");

	  // write the first block
	  for (i=0; i < PAGE_SIZE; i++)
	       ph[i] = (i % 10) + '0';
	  TEST_CHECK(writeBlock (0, &fh, ph));
	  printf("first block written\n");

	  // write the second block
	  for (i=0; i < PAGE_SIZE; i++)
	      ph[i] = (i % 10) + '0';
	  TEST_CHECK(writeBlock (1, &fh, ph));
	  printf("second block written\n");

	  // read the previous page 
	   TEST_CHECK(readPreviousBlock (&fh, ph));
	   for (i=0; i < PAGE_SIZE; i++)
	     ASSERT_TRUE((ph[i] == (i % 10) + '0'), "The character in page read from disk is the one that we had expected.");
	   printf("reading previous block\n");


	   // read the next page in the file
	   TEST_CHECK(readNextBlock (&fh, ph));
	   for (i=0; i < PAGE_SIZE; i++)
	     ASSERT_TRUE((ph[i] == (i % 10) + '0'), "The character in page read from disk is the one that we had expected.");
	   printf("reading next block\n");


	 // read the last page in the file
	   TEST_CHECK(readLastBlock (&fh, ph));
	   for (i=0; i < PAGE_SIZE; i++)
	     ASSERT_TRUE((ph[i] == (i % 10) + '0'), "character in page read from disk is the one we expected.");
	   printf("reading last block\n");

	 // writing current block
	  TEST_CHECK(writeCurrentBlock (&fh, ph));
	   for (i=0; i < PAGE_SIZE; i++)
	  	 ASSERT_TRUE((ph[i] == (i % 10) + '0'), "character in page read from disk is the one we expected.");
	   printf("write current block\n");

	 // append an empty block to the file
	   TEST_CHECK(appendEmptyBlock (&fh));
	   printf("append to the empty block\n");

	   
	// Ensure that it dosent exceed 5 pages
	   TEST_CHECK(ensureCapacity (5, &fh));
	   	  ASSERT_TRUE((fh.totalNumPages==5),"checking if the  total no. of pages is 5");
	   printf("ensure capacity \n");


	// destroying the page file
	  TEST_CHECK(destroyPageFile (TESTPF));

	  TEST_DONE();
}    

