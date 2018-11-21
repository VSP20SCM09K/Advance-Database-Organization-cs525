#include <string.h>
#include <stdlib.h>
#include "record_mgr.h"
#include "tables.h"
#include "buffer_mgr.h"
#include "storage_mgr.h"
#include "test_helper.h"
//#include "rm_serializer.c"


typedef struct TableData_information{
    int numberOfPages;//the number of pages of the file
    int numberOfRecords;//number of records in the table
    int numberOfRecordsPerPage;//total number of records in one page
    int numberOfTuplesInserted;//number of tuples that inserted into the file
    BM_BufferPool *bufferPool;//buffer pool of buffer manager
}TableData_information;

typedef struct RM_SCANDATA_MANAGEMENT{
    int totalScan;//number of tuple be scanned
    RID currentRID;//the RID of the record that scanned now
    Expr *cond;    //select condition of the record
}RM_SCANDATA_MANAGEMENT;

RM_TableData *rm_tabledata = NULL;



/* -----------Table and Record manager functions begins -------------------------*/

//Initializing a record manager
RC initRecordManager (void *mgmtData)
{
    rm_tabledata = (RM_TableData *) malloc (sizeof(RM_TableData));
    rm_tabledata->mgmtData = mgmtData;
    return RC_OK;
}

//Shutting down the record manager
RC shutdownRecordManager ()
{
    if (rm_tabledata->schema != NULL){
        free(rm_tabledata->schema); //free the memory associated with schema if it is not null
        rm_tabledata->schema = NULL;
    }//in case
    free(rm_tabledata->mgmtData); //free the data associated with tabledata
    rm_tabledata->mgmtData = NULL;
    free(rm_tabledata); //free tabledata
    rm_tabledata = NULL;
    
    return RC_OK;
}

//creating a table
RC createTable (char *name, Schema *schema)
{
    //passing name of the table. it will create a table with this file name
    //schema contains other information which needed to create a table

    if (name == NULL) return RC_FILE_NOT_FOUND;
    if (schema == NULL) return RC_RM_UNKOWN_DATATYPE;
    
    RC rc;
    rm_tabledata->name = name;
    rm_tabledata->schema = schema;
    TableData_information *tabledata_info;
    BM_BufferPool *bufferPool;
    
    tabledata_info = ((TableData_information *) malloc (sizeof(TableData_information)));
    tabledata_info->numberOfPages = 0;
    tabledata_info->numberOfRecords = 0;
    tabledata_info->numberOfRecordsPerPage = 0;
    tabledata_info->numberOfTuplesInserted = 0;
    
    //creating a page file with the name provided
    rc = createPageFile(rm_tabledata->name);
    if (rc != RC_OK)
    {
        return rc;
    }
    
    bufferPool = MAKE_POOL();
    bufferPool->mgmtData = malloc(sizeof(buffer));
    rc = openPageFile(rm_tabledata->name, &bufferPool->fH);
    if (rc != RC_OK)
    {
        return rc;
    }
    
    //copy number of pages,number of records,number of records per page, number of tuples inserted to assigned block
    char *first_page = (char *)malloc(PAGE_SIZE);
    memcpy(first_page, &tabledata_info->numberOfPages, sizeof(int));
    first_page += sizeof(int);
    memcpy(first_page, &tabledata_info->numberOfRecords, sizeof(int));
    first_page += sizeof(int);
    memcpy(first_page, &tabledata_info->numberOfRecordsPerPage, sizeof(int));
    first_page += sizeof(int);
    memcpy(first_page, &tabledata_info->numberOfTuplesInserted, sizeof(int));
    first_page -= 3*sizeof(int);
    strcat(first_page, serializeSchema(schema));
    
    //if block position is zero then use wrire current block else use write block function
    if (0 == getBlockPos(&bufferPool->fH))
    {
        rc = writeCurrentBlock(&bufferPool->fH, first_page);
    }
    else
    {
        rc = writeBlock(0, &bufferPool->fH, first_page);
    }
    if (rc != RC_OK)
    {
        return rc;
    }
    
    free(first_page);//free first page
    first_page = NULL;
    
    tabledata_info->numberOfPages = 0;
    tabledata_info->bufferPool = bufferPool;
    rm_tabledata->mgmtData = tabledata_info;
    return RC_OK;
}

//function to open a table
RC openTable (RM_TableData *rel, char *name)
{
    //input parameters contains all the information related to schema and name of the table 

    if (name == NULL) return RC_FILE_NOT_FOUND;

    if (rm_tabledata == NULL){ // if table data is null then send unknown data type error
        return RC_RM_UNKOWN_DATATYPE;
    }

    RC rc;
    //calling initBufferPool function to intialize a bufferpool to store the data of table
    rc = initBufferPool(((TableData_information *)rm_tabledata->mgmtData)->bufferPool, name, 10000, RS_CLOCK, NULL);
    if (rc != RC_OK)
    {
        return rc;
    }
    
    *rel = *rm_tabledata; //assigning a tabledata to schema
    return RC_OK;
}

//functoin to close table
RC closeTable (RM_TableData *rel)
{
     //input parameters contains all the information related to schema
    if (rm_tabledata == NULL) return RC_RM_UNKOWN_DATATYPE; // if table data is null then send unknown data type error
    RC rc;
    //calling shutdownBufferPool function to close a bufferpool 
    rc = shutdownBufferPool(((TableData_information *)rm_tabledata->mgmtData)->bufferPool);
    if (rc != RC_OK)
    {
        return rc;
    }
    
    rel->name = NULL;  //assignning null to schema name
    return RC_OK;
}
//function to delete table
RC deleteTable (char *name)
{
    //passing table name as input
    if (name == NULL) return RC_FILE_NOT_FOUND; //if table name is not found the return file not found error
    RC rc;
    //calling destory page file function to delete table information
    rc = destroyPageFile(name);
    if (rc != RC_OK)
    {
        return rc;
    }
    
    return RC_OK;
}
//function to get number of tuples
int getNumTuples (RM_TableData *rel)
{
    return ((TableData_information *)rm_tabledata->mgmtData)->numberOfRecords; //returns number of records are in table data
}
/* -----------Table and manager functions ends -------------------------*/

/* -----------handling a record functions begins -------------------------*/
//this function will insert the record data into file
RC insertrecorddataintofile (Record *record)
{
    if (record == NULL) return RC_RM_UNKOWN_DATATYPE;
    
    RC rc;
    int pageNumber = record->id.page;//getting page number of record
    int slot = record->id.slot; //getting the slot number of record
    BM_BufferPool *bufferPool = ((TableData_information *)rm_tabledata->mgmtData)->bufferPool;
    BM_PageHandle *page = (BM_PageHandle *)malloc(sizeof(BM_PageHandle));
    int offslot_size = getRecordSize(rm_tabledata->schema); //getting the record size
    
    page->data = (char *)malloc(PAGE_SIZE);
    
    //calling pinPage function to pins the page with page number
    rc = pinPage(bufferPool, page, pageNumber);
    if (rc != RC_OK)
        return rc;
    
    page->data += slot * (offslot_size);
    memcpy(page->data, record->data, offslot_size);// copy data of record to page
    page->data -= slot * (offslot_size);
    
    //calling markDirty function to mark page as a dirty
    rc = markDirty(bufferPool, page);
    if (rc != RC_OK)
        return rc;

    //calling unpinPage function to unpin a page
    rc = unpinPage(bufferPool, page);
    if (rc != RC_OK)
        return rc;

    //calling a forcePage function to write the current content of the page back to the page file on disk
    rc = forcePage(bufferPool, page);
    if (rc != RC_OK)
        return rc;
    
    return RC_OK;
}

//function to inserts the record
RC insertRecord (RM_TableData *rel, Record *record)
{
    //inserts the records passed in input parameter at avialable page and slot
    if (rel == NULL)
    {
        return -1;
    }
    
    if (record == NULL)
    {
        return -1;
    }
    RC rc;
    int offslot_size = getRecordSize(rm_tabledata->schema);
    TableData_information *tabledata_info = (TableData_information *)rm_tabledata->mgmtData;
    //to calculate the number of tuples per page.
    tabledata_info->numberOfRecordsPerPage = PAGE_SIZE/offslot_size;
    tabledata_info->numberOfRecords += 1;
    tabledata_info->numberOfTuplesInserted += 1;
    
    int available_slot = (tabledata_info->numberOfTuplesInserted)%(tabledata_info->numberOfRecordsPerPage); //decodding whether to send record into new file or free space at current file
    //record inserted into the new file if space is not available in current file
    if (available_slot == 1) tabledata_info->numberOfPages += 1;//incremating number of pages
    record->id.page = tabledata_info->numberOfPages; //updating record id
    //record inserted into avalile free space of the current file
    if (available_slot == 0) record->id.slot = tabledata_info->numberOfRecordsPerPage - 1;
    else record->id.slot = available_slot - 1; 
    
    rc = insertrecorddataintofile(record);//calling a function to record the insertred data into file
    if (rc != RC_OK)
    {
        return rc;
    }
    rm_tabledata->mgmtData = tabledata_info;
    *rel = *rm_tabledata;
    return RC_OK;
}
//function to delete record in table
RC deleteRecord (RM_TableData *rel, RID id)
{
    if (rel == NULL)
        return -1;

    int pageNumber = id.page; //getting the page number  of to be deleted record
    int slot = id.slot; //getting the slot of to be deleted slott
    
    RC rc;
    int offslot_size = getRecordSize(rm_tabledata->schema);
    TableData_information *tabledata_info = ((TableData_information *)rm_tabledata->mgmtData);
    BM_BufferPool *bufferPool = tabledata_info->bufferPool;
    BM_PageHandle *page = (BM_PageHandle *)malloc(sizeof(BM_PageHandle));
    page->data = (char *)malloc(PAGE_SIZE);
    
    //calling pinPage function to pins the page with page number
    rc = pinPage(bufferPool, page, pageNumber);
    if (rc != RC_OK)
        return rc;
    
    page->data += (offslot_size)*slot;
    memset(page->data, 0, offslot_size);// setting data of page values to null by 0 using memset functions
    page->data -= (offslot_size)*slot;
    //calling markDirty function to mark page as a dirty
    rc = markDirty(bufferPool, page);
    if (rc != RC_OK)
        return rc;

    //calling unpinPage function to unpin a page
    rc = unpinPage(bufferPool, page);
    if (rc != RC_OK)
        return rc;

    //calling a forcePage function to write the current content of the page back to the page file on disk
    rc = forcePage(bufferPool, page);
    if (rc != RC_OK)
        return rc;
        
    //updating number of records in table
    tabledata_info->numberOfRecords -= 1;
    
    rm_tabledata->mgmtData = tabledata_info;
    *rel = *rm_tabledata;
    return RC_OK;
}

//function to update the record in a a table
RC updateRecord (RM_TableData *rel, Record *record)
{
    //input parameters contains all the information related to schema and new record
    RC rc;
    if (rel == NULL)
        return -1;

    if (record == NULL)
        return -1;
    rc = insertrecorddataintofile(record);//updating the new record

    if (rc != RC_OK)
        return rc;
    
    *rel = *rm_tabledata;
    return RC_OK;
}

//functoin to get the record of particular page number and slot number mentioned in input parameter id
RC getRecord (RM_TableData *rel, RID id, Record *record)
{
    //input parameters contains all information of schema and page number and slot id to be read from file and a pointer to record data
    RC rc;
    if ((rm_tabledata == NULL)||(record == NULL))
        return RC_RM_UNKOWN_DATATYPE;
    int pageNumber = id.page; // record will be searched at this page number
    int slot = id.slot; // record will be searched at this slot
    record->id = id; //assigning a to get record id to schema
    int offslot_size = getRecordSize(rm_tabledata->schema); //getting the record size
    TableData_information *temp = (TableData_information *)rm_tabledata->mgmtData;
    
    BM_BufferPool *bufferPool = temp->bufferPool;
    BM_PageHandle *page = MAKE_PAGE_HANDLE();
    
    //calling pinPage function to pins the page with page number
    rc = pinPage(bufferPool, page, pageNumber);
    if (rc != RC_OK)
        return rc;
    
    page->data += (offslot_size)*slot;
    memcpy(record->data, page->data, offslot_size); //copy the page data to record data with the offslot size
    page->data -= (offslot_size)*slot;
    
    //calling unpinPage function to unpin a page
    rc = unpinPage(bufferPool, page);
    if (rc != RC_OK)
        return rc;
    
    return RC_OK;
}
/* -----------handling a record functions begins -------------------------*/

/* -----------scan functions begins -------------------------*/
// scan functions are used to retrieve all tuples from a table that fulfill a certain condition

//initializes the RM_ScanHandle data structure passed as an argument to startScan
RC startScan (RM_TableData *rel, RM_ScanHandle *scan, Expr *cond)//input parameters contains all the information related to schema and scan, and condition to scan the data
{
    if ((rel == NULL)||(scan == NULL)||(cond == NULL)) return RC_RM_UNKOWN_DATATYPE;
    RM_SCANDATA_MANAGEMENT *scan_management = ((RM_SCANDATA_MANAGEMENT *) malloc (sizeof(RM_SCANDATA_MANAGEMENT)));
    scan_management->totalScan = 0;
    scan_management->cond = cond;
    scan_management->currentRID.page = 1;// records starts from page 1
    scan_management->currentRID.slot = 0;// slot starts from 0
    scan->mgmtData = scan_management;
    scan->rel = rel;
    return RC_OK;
}

//next method should return the next tuple that fulfills the scan condition.
RC next (RM_ScanHandle *scan, Record *record)
{
    if ((scan == NULL)||(record == NULL)) return RC_RM_UNKOWN_DATATYPE;
    RC rc;
    RM_SCANDATA_MANAGEMENT *scan_management = (RM_SCANDATA_MANAGEMENT *)scan->mgmtData;
    //if the count number is all records number
    TableData_information *tabledata_info = (TableData_information *)rm_tabledata->mgmtData;
    
    Value *result = ((Value *) malloc (sizeof(Value)));
    
    result->v.boolV = FALSE;
    //This will will scan every record. after reading record it checks the condition and if condition is not satisfied then again the scan passed as parameter until the total record scan is less than the total number of tuple
    //next should return RC_RM_NO_MORE_TUPLES once the scan is completed and RC_OK otherwise (unless an error occurs of course)
    while(!result->v.boolV)
    {
        //check condition for no more tuple available in table
        if (scan_management->totalScan == tabledata_info->numberOfRecords) return RC_RM_NO_MORE_TUPLES;
        rc = getRecord (rm_tabledata, scan_management->currentRID, record); //get the record of current record id
        if (rc != RC_OK)
            return rc;
        
        rc = evalExpr (record, rm_tabledata->schema, scan_management->cond, &result);
        scan_management->currentRID.slot ++; //incremaenting the current slor
        if (scan_management->currentRID.slot == tabledata_info->numberOfRecordsPerPage)
        {
            scan_management->currentRID.page += 1;  //incrementing the page
            scan_management->currentRID.slot = 0;
        }
        scan_management->totalScan ++; //incrementing the total scan
        
    }
    scan->mgmtData = scan_management;
    return RC_OK;
}

//cloese scan will close the scan and reset all the information in RM_SCANDATA_MANAGEMENT
RC closeScan (RM_ScanHandle *scan)
{
    if (scan == NULL) return RC_RM_UNKOWN_DATATYPE;
    free(scan->mgmtData);
    scan->mgmtData = NULL;
    return RC_OK;
}

/* -----------scan functions ends -------------------------*/


/* -----------schema functions begins -------------------------*/

//this function will return the size of record based on length of each field and data type
int getRecordSize (Schema *schema)
{
    //if schema is not intialized then it will show that you need to create schema
    if (schema == NULL)
    {
        return RC_RM_SCHEMA_NOT_FOUND;
    }
    int sizeOfRecord = 0;
    for(int i = 0; i < schema->numAttr; i++)
    {
        //size of record will be set based on data type
        switch(schema->dataTypes[i])
        {
            case DT_FLOAT:
                sizeOfRecord += sizeof(float);
                break;
            case DT_BOOL:
                sizeOfRecord += sizeof(bool);
                break;
            case DT_INT:
                sizeOfRecord += sizeof(int);
                break;
            case DT_STRING:
                sizeOfRecord += schema->typeLength[i]+1;
                break;
            default:
                return RC_RM_UNKOWN_DATATYPE;
        }
    }
    return sizeOfRecord; // it will return the size of record
}

//function to define the parameter of new schmena based on the input parameters
static Schema *mallocSchema(int numAttr, int keySize)
{
    Schema *SCHEMA;
    
    // allocate memory to Schema
    SCHEMA = (Schema *)malloc(sizeof(Schema));
    SCHEMA->numAttr = numAttr;
    SCHEMA->attrNames = (char **)malloc(sizeof(char*) * numAttr);
    SCHEMA->typeLength = (int *)malloc(sizeof(int) * numAttr);
    SCHEMA->dataTypes = (DataType *)malloc(sizeof(DataType) * numAttr);
    SCHEMA->keyAttrs = (int *)malloc(sizeof(int) * keySize);
    SCHEMA->keySize = keySize;
    
    for(int i = 0; i < numAttr; i++)
    {
        SCHEMA->attrNames[i] = (char *) malloc(sizeof(char *));
    }
    
    return SCHEMA;
}

//this function will create schema with given input parameter such as attribute number,name,data type,length of attribute, number of keys and value of keys
Schema *createSchema (int numAttr, char **attrNames, DataType *dataTypes, int *typeLength, int keySize, int *keys)
{
    Schema *schema = mallocSchema(numAttr, keySize);
    //set the parameter of new schema based on the input parameters
    schema->numAttr = numAttr;
    schema->dataTypes = dataTypes;
    schema->typeLength = typeLength;
    schema->keySize = keySize;
    schema->keyAttrs = keys;
    for (int i = 0; i< numAttr; i++)
    {
        strcpy(schema->attrNames[i], attrNames[i]);
    }
    return schema;
}

//this function will free the memory which was allocated to schema
RC freeSchema (Schema *schema)
{
    if (schema == NULL)
    {
        return RC_RM_SCHEMA_NOT_FOUND;
    }
    //free every parameters
    free(schema->attrNames);
    schema->attrNames = NULL;
    free(schema->dataTypes);
    schema->dataTypes = NULL;
    free(schema->typeLength);
    schema->typeLength = NULL;
    free(schema->keyAttrs);
    schema->keyAttrs = NULL;
    free(schema);
    schema = NULL;
    
    return RC_OK;
}
/* -----------schema functions ends -------------------------*/

/* -----------Attribute functions begins -------------------------*/
//createRecord function is used to create a new record with all the null values
RC createRecord (Record **record, Schema *schema)
{
    //input parameters are pointer to newly created record and all attribues of schema
    
    if (schema == NULL) return RC_RM_SCHEMA_NOT_FOUND;
    *record = ((Record *) malloc (sizeof(Record)));//allocating memory for new record
    (*record)->data = (char *)malloc(getRecordSize(schema));
    
    return RC_OK;
}

//freeRecord function will free the memory related to record
RC freeRecord (Record *record)
{
    
    if (record == NULL) return -1;
    
    free(record->data);//free the record data
    record->data = NULL;
    free(record);//free the space of record
    record = NULL;
    
    return RC_OK;
}

//getAttr function returns the value of attribute pointed by atttrnum
RC getAttr (Record *record, Schema *schema, int attrNum, Value **value)//input parameter contains record data,schema attributes,attribute number which is to be retrived and values to be return
{
    if (record == NULL) return -1;
    if (schema == NULL) return -1;
    
    RC rc;
    char *record_data = record->data;
    int offsetattribute = 0;
    value[0] = ((Value *) malloc (sizeof(Value)));
    
    //getAttributeOffsetInRecord function returns offset of particular attribute number
    rc = attrOffset(schema, attrNum, &offsetattribute);
    if (rc != RC_OK){
        return rc;
    }
    
    record_data += offsetattribute;

    switch(schema->dataTypes[attrNum])
    {
        case DT_FLOAT:
            memcpy(&(value[0]->v.floatV), record_data, sizeof(float));//copy data from record data to value
            break;
        case DT_BOOL:
            memcpy(&(value[0]->v.boolV), record_data, sizeof(bool));//copy data from record data to value
            break;
        case DT_INT:
            memcpy(&(value[0]->v.intV), record_data, sizeof(int));//copy data from record data to value
            break;
        case DT_STRING:
            value[0]->v.stringV = (char *)malloc(schema->typeLength[attrNum] + 1);
            memcpy((value[0]->v.stringV), record_data, schema->typeLength[attrNum] + 1);//copy data from record data to value
            break;
        default:
            return RC_RM_UNKOWN_DATATYPE;
            
    }
    record_data -= offsetattribute;
    value[0]->dt = schema->dataTypes[attrNum];
    
    return RC_OK;
}
//setAttr functions will set value of particular attribute given in attrNum
RC setAttr (Record *record, Schema *schema, int attrNum, Value *value)//input parameters are pointers to the record data,schema attributes, attribute number whose value needs to be changed and new value of attribute
{
    RC rc;
    if (record == NULL) return -1;
    if (schema == NULL) return -1;
    
    if (attrNum < 0 || attrNum >= schema->numAttr) return -1;
    
    char *record_data = record->data;
    int offsetattribute = 0;
    rc = attrOffset(schema, attrNum, &offsetattribute);//getAttributeOffsetInRecord function returns offset of particular attribute number
    record_data += offsetattribute;
    schema->dataTypes[attrNum] = (value)->dt;
    switch((value)->dt){
        case DT_INT:
            memcpy(record_data, &((value)->v.intV), sizeof(int));//set data from value to record data
            break;
        case DT_STRING:
            memcpy(record_data, ((value)->v.stringV), schema->typeLength[attrNum] + 1);//set data from value to record data
            break;
        case DT_FLOAT:
            memcpy(record_data, &((value)->v.floatV), sizeof(float));//set data from value to record data
            break;
        case DT_BOOL:
            memcpy(record_data, &((value)->v.boolV), sizeof(bool));//set data from value to record data
            break;
        default:
            return RC_RM_UNKOWN_DATATYPE;
    }
    record_data -= offsetattribute;
    
    return RC_OK;
}

