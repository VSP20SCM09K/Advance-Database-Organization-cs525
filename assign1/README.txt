CS-525 HW1 Readme File
Malavika Sujir      - A20416469
Harsh Kaushikbhai Patel   - A20392023
Parthkumar Patel - A20416508

-->Instruction to Run :-

In the terminal go to assignment 1 directory and run the following commands:



Step 1-  $ make
Step 2-  $ ./test_assign1_1



About Test Cases:
We added three functions. These are new_test_cases(),read_not_there() and PageTest() in the file test_assign1_1.c. The description of these test cases are as follows.

1. new_test_cases(): Tests the content of a single page.
2. read_not_there(): Test reads something that isnt there
3. PageTest(): Tests the content of the second page

Explanation of the test case logic:
1. new_test_cases():
1.1 The first block is read. If there are no bytes then it is confirmed that the block has nothing in it.
Write function is executed.
1.2 The next block is read. Content is checked for and then written
This process goes on until the page size is reached.
1.3 When it reaches the last block, again it should encounter 0 bytes. This is checked for.

2. read_not_there():
2.1 A new page file is created and opened. 
2.2 The file is read until it encounters the last page.
2.3 After this is done, it destroys the page file that was created.
2.4 Thereby it checks if something is not there or if the content is legitimate

3. PageTest():
3.1 Every page is read from the beginning until the end.
3.2 It is first read and then checked for.(if the charecter is what we had actually written)
3.3 A read and a write is executed if the content of the page is legitimate.(the current block is read and written)
3.4 The capacity in terms of the number of pages is ensured by this as well
3.5 The pagefile is then destroyed.





Some of the functions in the storage manager file are as follows:
1.readBlock()
2.readFirstBlock() 
3.readCurrentBlock()
4.readPreviousBlock()
5.readNextBlock()
6.readLastBlock()
7.writeBlock ()
8.writeCurrentBlock()
9.appendEmptyBlock()
10.ensureCapacity ()
11.createPageFile()
12.openPageFile()
13.closePageFile()
14.destroyPageFile()
15.getBlockPos()


Each of the functions have the following description.

1. The functions below modify the files in the ways as mentioned below.

a)openPageFile():
       1.Checks if the input file is present.
       2.If the file is present then the file is opened, if not the error message is thrown
b)createPageFile():
       1.Creates a new page file of 1 page and fills it with '\0' bytes.
c)destroyPageFile():
       1.Destroys the already created page file.
d)closePageFile():
       1.Closes the already open page file.

2.Read Functions
a) readFirstBlock():
      1.Checks if the entered file already exists.
      2.If the file exists;reads the first block of the file.
      3.If the file does not exist it throws an error.

b) getBlockPos():
      1.Returns the pointer of the block

c) readBlock():
      1.Checks if the entered file already exists.
      2.If the file exists it reads the entered block of the file.
      3.If the file does not exist it throws an error.

d) readPreviousBlock():
      1.Checks if the entered file already exists.
      2.If the file exists it reads the previous block to which the pointer in the data structure is pointing.
      3.If the file does not exist throws error.

e) readNextBlock():
      1.Checks if the entered file already exists.
      2.If the file exists it reads the next block of data to which the pointer is pointing.
      3.If the file does not exist an error is thrown

f) readCurrentBlock():
      1.Checks if the entered file already exists.
      2.If the file exists it reads the block of data to which the pointer in the data structure is pointing.
      3.If the file does not exist it throws an error.

g) readLastBlock():
      1.Checks if the entered file already exists.
      2.If the file exists it reads the last block.
      3.If the file does not exists it throws an error.

3. Write Functions

a) writeBlock():
      1.Checks if the entered file already exists. If it dosent it throws an error.
      2.If the file exists it writes data in the page number so given.                               
b)writeCurrentBlock():      
      1.Checks if the entered file already exists else it throws an error.
      2.If the file exists it writes the data where the pointer is pointing.

c)ensureCapacity():
       1.Checks for the number of pages in the file.
       2.If the number of pages is less than the specified pages, then it increase the the number of pages to the specified number.
       3.It uses appendEmptyBlock function to add the number of pages with '/0' bytes data.

d)appendEmptyBlock():
      - Checks if the entered file already exists else throws an error.
      - If the file is found to already exist,then an empty block is appended at the end


The dberror.c file is used when there is an error. Used to provide an error message.

