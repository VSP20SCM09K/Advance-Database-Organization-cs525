#include "storage_mgr.h"
#include "buffer_mgr_stat.h"
#include "buffer_mgr.h"
#include "dberror.h"
#include "test_helper.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


char *testName;

// check whether two  contents of the buffer pool is the same as the expected content 

#define ASSERT_EQUALS_POOL(expected,bm,message)             \
  do {                  \
    char *real;               \
    char *_exp = (char *) (expected);                                   \
    real = sprintPoolContent(bm);         \
    if (strcmp((_exp),real) != 0)         \
      {                 \
  printf("[%s-%s-L%i-%s] FAILED: expected <%s> but was <%s>: %s\n",TEST_INFO, _exp, real, message); \
  free(real);             \
  exit(1);              \
      }                 \
    printf("[%s-%s-L%i-%s] OK: expected <%s> and was <%s>: %s\n",TEST_INFO, _exp, real, message); \
    free(real);               \
  } while(0)

// The testing methods that were used are as below:
static void testCreatingAndReadingDummyPages (void);
static void createDummyPages(BM_BufferPool *bp, int num);
static void checkDummyPages(BM_BufferPool *bp, int n);
static void testReadPage (void);
static void testFIFO (void);
static void testLRU (void);
static void test_of_LRU_K (void);


// main method
int 
main (void) 
{
printf("tcar");
  initStorageManager();
  testName = "";

    testCreatingAndReadingDummyPages();
    testReadPage();
    testFIFO();
    testLRU();
    test_of_LRU_K();
}

// create n pages with content "Page X" and read them back to check whether the content is right
void
testCreatingAndReadingDummyPages (void)
{
  BM_BufferPool *bp = MAKE_POOL();
  testName = "Creating and Reading Back Dummy Pages";

  CHECK(createPageFile("test_buffer.bin"));

    createDummyPages(bp, 22);
    checkDummyPages(bp, 20);

  createDummyPages(bp, 10000);
  checkDummyPages(bp, 10000);

  CHECK(destroyPageFile("test_buffer.bin"));

  free(bp);
  TEST_DONE();
}


void 
createDummyPages(BM_BufferPool *bp, int num)
{
  int i;
  BM_PageHandle *h = MAKE_PAGE_HANDLE();

  CHECK(initBufferPool(bp, "test_buffer.bin", 3, RS_FIFO, NULL));
  
  for (i = 0; i < num; i++)
    {
      CHECK(pinPage(bp, h, i));
      sprintf(h->data, "%s-%i", "Page", h->pageNum);
      CHECK(markDirty(bp, h));
      CHECK(unpinPage(bp,h));
    }

  CHECK(shutdownBufferPool(bp));

  free(h);
}

void 
checkDummyPages(BM_BufferPool *bp, int n)
{
  int i;
  BM_PageHandle *h = MAKE_PAGE_HANDLE();
  char *e = malloc(sizeof(char) * 512);

  CHECK(initBufferPool(bp, "test_buffer.bin", 3, RS_FIFO, NULL));

  for (i = 0; i < n; i++)
    {
      CHECK(pinPage(bp, h, i));

      sprintf(e, "%s-%i", "Page", h->pageNum);
      ASSERT_EQUALS_STRING(e, h->data, "Reading the dummy page back");// we read the dummy page

      CHECK(unpinPage(bp,h));
    }

  CHECK(shutdownBufferPool(bp));

  free(e);
  free(h);
}

void
testReadPage ()
{
  BM_BufferPool *bp = MAKE_POOL();
  BM_PageHandle *h = MAKE_PAGE_HANDLE();
  testName = "Reading a page";

  CHECK(createPageFile("test_buffer.bin"));
  CHECK(initBufferPool(bp, "test_buffer.bin", 3, RS_FIFO, NULL));
  
  CHECK(pinPage(bp, h, 0));
  CHECK(pinPage(bp, h, 0));

  CHECK(markDirty(bp, h));

  CHECK(unpinPage(bp,h));
  CHECK(unpinPage(bp,h));

  CHECK(forcePage(bp, h));

  CHECK(shutdownBufferPool(bp));
  CHECK(destroyPageFile("test_buffer.bin"));

  free(bp);
  free(h);

  TEST_DONE();
}

void
testFIFO ()//test for the first in first out
{
  // expected results
  const char *poolContents[] = { 
    "[0 0],[-1 0],[-1 0]" , 
    "[0 0],[1 0],[-1 0]", 
    "[0 0],[1 0],[2 0]", 
    "[3 0],[1 0],[2 0]", 
    "[3 0],[4 0],[2 0]",
    "[3 0],[4 1],[2 0]",
    "[3 0],[4 1],[5x0]",
    "[6x0],[4 1],[5x0]",
    "[6x0],[4 1],[0x0]",
    "[6x0],[4 0],[0x0]",
    "[6 0],[4 0],[0 0]"
  };
  const int requests[] = {0,1,2,3,4,4,5,6,0};
  const int numLinRequests = 5;
  const int numChangeRequests = 3;

  int i;
  BM_PageHandle *h = MAKE_PAGE_HANDLE();
  BM_BufferPool *bp = MAKE_POOL();
  
  testName = "Testing FIFO page replacement";

  CHECK(createPageFile("test_buffer.bin"));

  createDummyPages(bp, 100);

  CHECK(initBufferPool(bp, "test_buffer.bin", 3, RS_FIFO, NULL));

  // Reading Pages
  for(i = 0; i < numLinRequests; i++)
    {
      pinPage(bp, h, requests[i]);
      unpinPage(bp, h);
      ASSERT_EQUALS_POOL(poolContents[i], bp, "check pool content");// pool content checking is done
    }

  // Using one page and testing 
  i = numLinRequests;
  pinPage(bp, h, requests[i]);
  //ASSERT_EQUALS_POOL(poolContents[i],bp,"pool content after pin page");
  ASSERT_EQUALS_POOL(poolContents[i],bp,"pool content after pin page");

  
  for(i = numLinRequests + 1; i < numLinRequests + numChangeRequests + 1; i++)
    {
      pinPage(bp, h, requests[i]);
      markDirty(bp, h);
      unpinPage(bp, h);
      ASSERT_EQUALS_POOL(poolContents[i], bp, "check pool content");
    }

  // flush buffer pool 
  i = numLinRequests + numChangeRequests + 1;
  h->pageNum = 4;
  unpinPage(bp, h);
  ASSERT_EQUALS_POOL(poolContents[i],bp,"The Last Page is unpinned");
  
  i++;
  forceFlushPool(bp);
  ASSERT_EQUALS_POOL(poolContents[i],bp,"After flushing the pool contents are");

  // check number of write IOs
  ASSERT_EQUALS_INT(3, getNumWriteIO(bp), "check number of write I/Os");
  ASSERT_EQUALS_INT(8, getNumReadIO(bp), "check number of read I/Os");

  CHECK(shutdownBufferPool(bp));
  CHECK(destroyPageFile("test_buffer.bin"));

  free(bp);
  free(h);
  TEST_DONE();
}

// test LRU 
void
testLRU (void)
{
  // expected results
  const char *poolContents[] = { 
    // read the first five pages and directly unpin them
    "[0 0],[-1 0],[-1 0],[-1 0],[-1 0]" , 
    "[0 0],[1 0],[-1 0],[-1 0],[-1 0]", 
    "[0 0],[1 0],[2 0],[-1 0],[-1 0]",
    "[0 0],[1 0],[2 0],[3 0],[-1 0]",
    "[0 0],[1 0],[2 0],[3 0],[4 0]",
    // use some of the page to create a fixed LRU order without changing the pool content
    "[0 0],[1 0],[2 0],[3 0],[4 0]",
    "[0 0],[1 0],[2 0],[3 0],[4 0]",
    "[0 0],[1 0],[2 0],[3 0],[4 0]",
    "[0 0],[1 0],[2 0],[3 0],[4 0]",
    "[0 0],[1 0],[2 0],[3 0],[4 0]",
    // check that pages get evicted in LRU order
    "[0 0],[1 0],[2 0],[5 0],[4 0]",
    "[0 0],[1 0],[2 0],[5 0],[6 0]",
    "[7 0],[1 0],[2 0],[5 0],[6 0]",
    "[7 0],[1 0],[8 0],[5 0],[6 0]",
    "[7 0],[9 0],[8 0],[5 0],[6 0]"
  };
  const int orderRequests[] = {3,4,0,2,1};
  const int numLRUOrderChange = 5;

  int i;
  int ss = 0;
  BM_BufferPool *bp = MAKE_POOL();
  BM_PageHandle *h = MAKE_PAGE_HANDLE();
  testName = "Testing LRU page replacement";

  CHECK(createPageFile("test_buffer.bin"));
  createDummyPages(bp, 100);
  CHECK(initBufferPool(bp, "test_buffer.bin", 5, RS_LRU, NULL));

  // reading first five pages linearly with direct unpinning assuming no modifications are made
  for(i = 0; i < 5; i++)
  {
      pinPage(bp, h, i);
      unpinPage(bp, h);
      ASSERT_EQUALS_POOL(poolContents[ss++], bp, "check pool content reading in pages");
  }

  // read pages to change the order of the LRU
  for(i = 0; i < numLRUOrderChange; i++)
  {
      pinPage(bp, h, orderRequests[i]);
      unpinPage(bp, h);
      ASSERT_EQUALS_POOL(poolContents[ss++], bp, "check pool content using pages");
  }

  // replace pages and check that it happens in LRU order
  for(i = 0; i < 5; i++)
  {
      pinPage(bp, h, 5 + i);
      unpinPage(bp, h);
      ASSERT_EQUALS_POOL(poolContents[ss++], bp, "check pool content using pages");
  }

  // check number of write Inputs
  ASSERT_EQUALS_INT(0, getNumWriteIO(bp), "check number of write I/Os");
  ASSERT_EQUALS_INT(10, getNumReadIO(bp), "check number of read I/Os");

  CHECK(shutdownBufferPool(bp));
  CHECK(destroyPageFile("test_buffer.bin"));

  free(bp);
  free(h);

  TEST_DONE();
}


void
test_of_LRU_K (void)
{
  
  const char *poolContents[] = { 
    // read first five pages and directly unpin them
    "[0 0],[-1 0],[-1 0],[-1 0],[-1 0]" , 
    "[0 0],[1 0],[-1 0],[-1 0],[-1 0]", 
    "[0 0],[1 0],[2 0],[-1 0],[-1 0]",
    "[0 0],[1 0],[2 0],[3 0],[-1 0]",
    "[0 0],[1 0],[2 0],[3 0],[4 0]",
    // use some of the page to create a fixed LRU_K order without changing pool content
    "[0 0],[1 0],[2 0],[3 0],[4 0]",
    "[0 0],[1 0],[2 0],[3 0],[4 0]",
    "[0 0],[1 0],[2 0],[3 0],[4 0]",
    "[0 0],[1 0],[2 0],[3 0],[4 0]",
    "[0 0],[1 0],[2 0],[3 0],[4 0]",
    // check that pages get evicted in LRU_K order
    "[0 0],[1 0],[2 0],[5 0],[4 0]",
    "[0 0],[1 0],[2 0],[5 0],[6 0]",
    "[7 0],[1 0],[2 0],[5 0],[6 0]",
    "[7 0],[1 0],[8 0],[5 0],[6 0]",
    "[7 0],[9 0],[8 0],[5 0],[6 0]"
  };
  const int order_of_Requests[] = {3,4,0,2,1};
  const int numLRU_KOrderChange = 5;

  int i;
  int snapshot = 0;
  BM_BufferPool *bm = MAKE_POOL();
  BM_PageHandle *h = MAKE_PAGE_HANDLE();
  testName = "Test of LRU_K page replacement";

  CHECK(createPageFile("test_buffer.bin"));
  createDummyPages(bm, 100);
  CHECK(initBufferPool(bm, "test_buffer.bin", 5, RS_LRU_K, NULL));

  // reading first five pages linearly with direct unpin and no modifications
  for(i = 0; i < 5; i++)
  {
      pinPage(bm, h, i);
      unpinPage(bm, h);
      ASSERT_EQUALS_POOL(poolContents[snapshot++], bm, "varify pool content reading in pages");
  }

  // read pages to change LRU_K order
  for(i = 0; i < numLRU_KOrderChange; i++)
  {
      pinPage(bm, h, order_of_Requests[i]);
      unpinPage(bm, h);
      ASSERT_EQUALS_POOL(poolContents[snapshot++], bm, "varify pool content using pages");
  }

  // replace pages then  check whether it happens in the function order
  for(i = 0; i < 5; i++)
  {
      pinPage(bm, h, 5 + i);
      unpinPage(bm, h);
      ASSERT_EQUALS_POOL(poolContents[snapshot++], bm, "varify pool content using pages");
  }

  
  ASSERT_EQUALS_INT(0, getNumWriteIO(bm), "varify number of write I/Os");
  ASSERT_EQUALS_INT(10, getNumReadIO(bm), "varify number of read I/Os");

  CHECK(shutdownBufferPool(bm));
  CHECK(destroyPageFile("test_buffer.bin"));

  free(bm);
  free(h);
  TEST_DONE();
}
