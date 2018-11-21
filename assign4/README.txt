CS-525 HW4 Readme File
Malavika Sujir      - A20416469
Harsh Kaushikbhai Patel   - A20392023
Parthkumar Patel - A20416508

-->Instruction to Run :-

In the terminal go to assignment 4 directory and run the following commands:

1. make
2. ./test_assign4_1
3. ./test_assign4_2
4. ./test_assign4_3
		

FUNCTION DESCRIPTIONS:
-----------------------

1) initIndexManager (void *mgmtData)

	Initialize index manager.


2) shutdownIndexManager ()

	Close index manager and free resources.


3) createBtree (char *idxId, DataType keyType, int n)

	Apply resources and initialize a B+ tree.


4) openBtree (BTreeHandle **tree, char *idxId)

	Open an existing B+ tree.


5) closeBtree (BTreeHandle *tree)

	Close a B+ tree and free resources.


6) deleteBtree (char *idxId)

	Delete a B+ tree from disk.


7) getNumNodes (BTreeHandle *tree, int *result)

	Get number of nodes from a B+ tree.


8) getNumEntries (BTreeHandle *tree, int *result)

	Get number of entries from a B+ tree.


9) getKeyType (BTreeHandle *tree, DataType *result)

	Get Key Type of a B+ tree.


10) findKey (BTreeHandle *tree, Value *key, RID *result)

	Find a key in B+ tree and return the RID result.


11) insertKey (BTreeHandle *tree, Value *key, RID rid)

	Insert a key into a B+ tree.


12) deleteKey (BTreeHandle *tree, Value *key)

	Delete a key from a B+ tree.


13) openTreeScan (BTreeHandle *tree, BT_ScanHandle **handle)

	Initialize the scan handle and prepare for scan.


14) nextEntry (BT_ScanHandle *handle, RID *result)

	Return next entry during scanning.


15) closeTreeScan (BT_ScanHandle *handle)

	Stop scanning and free resources.


16) printTree (BTreeHandle *tree)

	return the printing of the tree in the format. This is a debug function.


17) DFS(RM_BtreeNode *bTreeNode)

	Do a pre-order tree walk and assign positions to tree nodes.


18) walk(RM_BtreeNode *bTreeNode, char *result)

	Do a pre-order tree walk and generate the printing string.


19) createNewNod()

	return a newly created B+ tree node.


20) insertParent(RM_BtreeNode *left, RM_BtreeNode *right, int key)

	Insert key into non-leaf B+ tree node recursively.


21) deleteNode(RM_BtreeNode *bTreeNode, int index)

	Delete the node recursively if needed.



ADDITIONAL IMPLEMENTATION and TEST CASES (Conditional Updates using Scans):
------------------------------------------------------------------------------------------



The test cases are as follows:
1. testInsertAndFind_String
a. checks if the values have been entered into the B tree
b. Checks for the number of enteries and also validates with the RID

2. testDelete_Float
a. checks for the value to delete
b. Uses the RID to check for the value
c. Once the value is found, it is deleted
d. After the delete, again a search operation is done.
e. This is to ensure that the deleted item is not found again.

3. testInsertAndFind_Float
a. Checks for the number of nodes and the number of enteries in the B tree
b. Iterates over the loop(500) and checks if the RID was found
c. In step 2 we basically do a search to find the key

4. testDelete_String
a. check for entry against the RID(among the inserted enteries as well)
b. The RID of the entery that was deleted should not be found.
c. For the ones that are not deleted, when we find the right entry we check it against the RID - for validation

5. createPermutation
a. This is used to create different permutations of values
b. A temporary variable, an iterator and a variable called res is used
c. Value of res is returned

6. createValues
a. different values are created. 
b. A comversion is done from the string to a  value
c. The result is stored in res

7. freeValues
a. The value is just freed using the function free.
