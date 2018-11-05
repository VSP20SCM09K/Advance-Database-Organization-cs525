CS-525 HW3 Readme File
Malavika Sujir      - A20416469
Harsh Kaushikbhai Patel   - A20392023
Parthkumar Patel - A20416508

-->Instruction to Run :-

In the terminal go to assignment 3 directory and run the following commands:

1. Execute the make file 1 as below: 
		make -f makefile1

2. Execute the make file 2 as below: 
		make -f makefile2

3. Execute the make file 3 as below: 
		make -f makefile3
		

FUNCTION DESCRIPTIONS:
-----------------------

Table and manager:
a) initRecordManager:
	- Used to initialize record manager

b) shutdownRecordManager:
	- Used to shutdown Record Manager

c) createTable:
	- Used to create a new table.
	- The table name and the schema and free space information is included.

d) openTable:
	- Used to open table information. 
	- Initializes buffer pool and then pins a page
	- Reads schema from the file
	- Unpins the page.

e) closeTable:
	- Used to close a table. 
	- Pin page, mark dirty.
	- Unpin page and then shutdown the buffer pool.

f) deleteTable:
	- Used to delete the table.
	- If the destroyPageFIle is not RC_OK then the page will not be deleted.

g) getNumTuples:
	- Used to returns the number of tuples in the table.

Handling records in a table:
a) insertRecord:
	- It is used to insert a new record at the page and slot mentioned.	
	- The record is to be inserted in the free/available space.
	- Page is pinned and then the record is inserted.
	- After the record is inserted, the page is marked as dirty and written back to memory.
	- Later on the page is unpinned.

b) deleteRecord:
	- It is used to delete a record from the page and slot mentioned.	
	- Page is pinned and then the record is deleted.
	- Once the record is deleted number of records in a page is decreased by 1.
	- After the record is deleted, the page is marked as dirty.
	- Later on the page is unpinned.

c) updateRecord:
	- It is used to update an existing record from the page and slot mentioned.	
	- Page is pinned and then the record is updared.
	- After the record is updated, the page is marked as dirty.
	- Later on the page is unpinned.	

d) getRecord:
	- It is used to retrieve a record from a page and slot mentioned.
	- Page is pinned and then the record is retrieved.
	- The page is then unpinned.

Scans:
a) startScan:
	- It is used to scan the tuples based on a certain criteria (expr).
	- Start scan initializes the RM_ScanHandle data structure.

b) next:
	- The start scan function calls the next method.
	- It returns the next tuple from the rcord which satisfies the criteria mentioned in start scan.
	- If NULL is passed as the scan criteria then all the tuples are scanned and next returns RC_RM_NO_MORE_TUPLES

c) closeScan:
	- It is used to indicate to the record manager that all associated resources can be cleaned up.

Dealing with schemas:
a) getRecordSize: 
	- Checks if the schema is created; if not it throws an error
	- If schema exists then it returns the size in bytes of records for a given schema

b) createSchema:
	- Used to create a new schema

c) freeSchema:
	- This function is used to free the space associated with that particular schema in the memory.

Dealing with records and attribute values:
a) createRecord:
	- These function is used to create a new record 	
	- Initially, page and slot are set to -1 as it has not inserted into table/page/slot	

b) freeRecord:
	- Checks if the record is free.
	- If the record is free then it returns record free
	- If not free then it frees the record by removing the data from the record.

c) getAttr:
	- It is used to get the attribue value of a particular record.
	- The value requested can be a Integer, String or Float.

d) setAttr:
	- It is used to set the attribute value.
	-  A particular attribute in a record is set to the specified value and it can be Integer, String or Float type.


ADDITIONAL ERROR CODES:
------------------------------------------------------------------------------------------RC_MELLOC_MEM_ALLOC_FAILED
RC_SCHEMA_NOT_INIT
RC_PIN_PAGE_FAILED
RC_UNPIN_PAGE_FAILED
RC_INVLD_PAGE_NUM 
RC_IVALID_PAGE_SLOT_NUM 
RC_MARK_DIRTY_FAILED 
RC_BUFFER_SHUTDOWN_FAILED 
RC_NULL_IP_PARAM 
RC_FILE_DESTROY_FAILED 
RC_OPEN_TABLE_FAILED 


ADDITIONAL DATA STRUCTURES USED:
------------------------------------------------------------------------------------------

1. TableMgmt_info - stores informaton to manage the table.

    sizeOfRec - stores size of record.
    totalRecordInTable - total records in table.
    blkFctr - it stores the number of rcords that can be stored in a single page.
    firstFreeLoc - contains information about the first free location.

2. RM_SCAN_MGMT - stores information to manage information to scan data.

    recID - stores record id
    Expr *cond - it is the condition to update the records.
    int count - stores the no of records scanned


ADDITIONAL IMPLEMENTATION and TEST CASES (Conditional Updates using Scans):
------------------------------------------------------------------------------------------- It takes a condition(expression) based on which the tuples to be updated are selected and a pointer to a function which takes a record as input and returns the updated version of the record.

We have added test case in test_assign3_2.c to test the additional function for conditional updates using scans.

