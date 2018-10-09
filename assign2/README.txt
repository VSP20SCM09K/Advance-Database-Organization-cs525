CS-525 HW2 Readme File
Malavika Sujir      - A20416469
Harsh Kaushikbhai Patel   - A20392023
Parthkumar Patel - A20416508

-->Instruction to Run :-

In the terminal go to assignment 2 directory and run the following commands:



Step 1-  $ make
Step 2-  $ ./test_assign2_1
Step 3-  $ ./test_assign2_2



About Test Cases:
The test cases (and their descriptions) that we added are as below .
1. testCreatingAndReadingDummyPages(): 
2. createDummyPages(): 
3. checkDummyPages(): 
4. testReadPage():
5. testFIFO():
6. testLRU():
7. test_of_LRU_K():

Explanation of the test case logic:




1. testCreatingAndReadingDummyPages(): 
   1. A Dummpy Page is created and read.
   2. Once this is done the destroyPageFile is used to destroy the file.
2. createDummyPages(): 
   1. A for loop is created so that the dummy pages are created
   2. The num is made as the check statement in the for loop so that it does not cross the value of num
3. checkDummyPages(): 
   1. The DummyPage is read page by page and then is unpinned.
   2. This function checks the Dummy Pages so created 
   3. At the beginning of this function a size is allocated. This aids in the process of checking.
4. testReadPage():
    1. It firsts pins the page that it has to read
    2. It reads the page , unpins it and then destroys it.
5. testFIFO():
    1. The page is pinned at first.
    2. The contents of the pool are checked and then unpinned
    3. After the last page is unpined, a force flush is done.
    4. The number of read and write inputs are checked.
    5. The buffer pool is then shut down and the test is completed
6. testLRU():
    1. The first five pages are read directly. After unpinning,the order of the LRU is changed and read.
    2. The pages are replaced and checked for.
    3. The number of inputs - that is the read and the write are checked for and the test thus ends
7. test_of_LRU_K():
    1. The first five pages are read directly. 
    2. Again read the pages, but this time changing the functions order.
    3. The pages are replaced and checked if it it happens in the function order.
    4. The number of read and write inputs are checked.
The dberror.c file is used when there is an error. Used to provide an error message.

