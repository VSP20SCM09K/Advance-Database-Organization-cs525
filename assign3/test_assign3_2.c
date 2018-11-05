#include <stdlib.h>
#include "dberror.h"
#include "expr.h"
#include "record_mgr.h"
#include "tables.h"
#include "test_helper.h"


#define ASSERT_EQUALS_RECORDS(_l,_r, sch, m)			\
  do {									\
    Record *_lR = _l;                                                   \
    Record *_rR = _r;                                                   \
    ASSERT_TRUE(memcmp(_lR->data,_rR->data,getRecordSize(sch)) == 0, m); \
    int i;								\
    for(i = 0; i < sch->numAttr; i++)				\
      {									\
        Value *lVal, *rVal;                                             \
		char *lSer, *rSer; \
        getAttr(_lR, sch, i, &lVal);                                  \
        getAttr(_rR, sch, i, &rVal);                                  \
		lSer = serializeValue(lVal); \
		rSer = serializeValue(rVal); \
        ASSERT_EQUALS_STRING(lSer, rSer, "Same attribute");	\
		free(lVal); \
		free(rVal); \
		free(lSer); \
		free(rSer); \
      }									\
  } while(0)

#define ASSERT_EQUALS_RECORD_IN(_l,_r, rSize, sch, m)		\
  do {									\
    int j;								\
    boolean found = false;						\
    for(j = 0; j < rSize; j++)						\
      if (memcmp(_l->data,_r[j]->data,getRecordSize(sch)) == 0)	\
	found = true;							\
    ASSERT_TRUE(0, m);						\
  } while(0)

#define OP_TRUE(le, ri, op, m)		\
  do {							\
    Value *rlt = (Value *) malloc(sizeof(Value));	\
    op(le, ri, rlt);				\
    bool b = rlt->v.boolV;				\
    free(rlt);					\
    ASSERT_TRUE(b,m);				\
   } while (0)

// test methods
static void testUpdateScan(void);
static void testRecords(void);

static void testUpdateTable(void);
static void testScans(void);
static void testScansTwo(void);
static void testInsertManyRecords(void);
static void testMultipleScans(void);

// ----- structure defined ------
typedef struct TestRecord {
    int one;
    char *two;
    int three;
} TestRecord;


Record *testRecord(Schema *sch, int one, char *two, int three);
Schema *testSchema (void);
Record *fromTestRecord (Schema *sch, TestRecord tr);

// ----name of the test used ----
char *testName;

// main method
int
main (void)
{
    testName = "";
    testUpdateScan();
    return 0;
}

void testUpdateScan(void)
{
    RM_TableData *tab = (RM_TableData *) malloc(sizeof(RM_TableData));
    TestRecord inserts[] = {
            {1, "aaaa", 3},
            {2, "bbbb", 2},
            {3, "cccc", 1},
            {4, "dddd", 3},
            {5, "eeee", 5},
            {6, "ffff", 1},
            {7, "gggg", 3},
            {8, "hhhh", 3},
            {9, "iiii", 2},
            {10, "jjjj", 5},
    };
    bool foundScan[] = {
            FALSE,
            FALSE,
            FALSE,
            FALSE,
            FALSE,
            FALSE,
            FALSE,
            FALSE,
            FALSE,
            FALSE
    };

    TestRecord updates[] = {
            {8888, "iiii", 8}
    };
    TestRecord updateRec[] = {
            {1, "aaaa", 3},
            {2, "bbbb", 2},
            {3, "cccc", 1},
            {4, "dddd", 3},
            {8888, "iiii", 8},
            {6, "ffff", 1},
            {7, "gggg", 3},
            {8, "hhhh", 3},
            {9, "iiii", 2},
            {8888, "iiii", 8},
    };
    int numInserts = 10, i;
    Record *r,*updateRecord;
    RID *ridss;
    Schema *sch;
    RM_ScanHandle *sc = (RM_ScanHandle *) malloc(sizeof(RM_ScanHandle));
    Expr *sel, *le, *ri, *fst, *se;
    int rc;

    testName = "test is for updating the scan";
    sch = testSchema();
    ridss = (RID *) malloc(sizeof(RID) * numInserts);

    TEST_CHECK(initRecordManager(NULL));
    TEST_CHECK(createTable("test_table_r",sch));
    TEST_CHECK(openTable(tab, "test_table_r"));

    // insert rows into table
    for(i = 0; i < numInserts; i++)
    {
        r = fromTestRecord(sch, inserts[i]);
        TEST_CHECK(insertRecord(tab,r));
        ridss[i] = r->id;
    }

    TEST_CHECK(closeTable(tab));
    TEST_CHECK(openTable(tab, "test_table_r"));

    
    MAKE_CONS(le, stringToValue("i2"));
    MAKE_ATTRREF(ri, 0);
    MAKE_BINOP_EXPR(sel, le, ri, OP_COMP_EQUAL);
    createRecord(&r, sch);
    TEST_CHECK(startScan(tab, sc, sel));
    while((rc = next(sc, r)) == RC_OK)
    {
        ASSERT_EQUALS_RECORDS(fromTestRecord(sch, inserts[1]), r, sch, "comparing the records");
    }
    if (rc != RC_RM_NO_MORE_TUPLES)
        TEST_CHECK(rc);
    TEST_CHECK(closeScan(sc));

    
    MAKE_CONS(le, stringToValue("sffff"));
    MAKE_ATTRREF(ri, 1);
    MAKE_BINOP_EXPR(sel, le, ri, OP_COMP_EQUAL);
    createRecord(&r, sch);
    TEST_CHECK(startScan(tab, sc, sel));
    while((rc = next(sc, r)) == RC_OK)
    {
        ASSERT_EQUALS_RECORDS(fromTestRecord(sch, inserts[5]), r, sch, "comparing the records");
        serializeRecord(r, sch);
    }
    if (rc != RC_RM_NO_MORE_TUPLES)
        TEST_CHECK(rc);
    TEST_CHECK(closeScan(sc));

    //---Choose only those records that evaulate to false--------
    MAKE_CONS(le, stringToValue("i4"));
    MAKE_ATTRREF(ri, 2);
    MAKE_BINOP_EXPR(fst, ri, le, OP_COMP_SMALLER);
    MAKE_UNOP_EXPR(se, fst, OP_BOOL_NOT);
    TEST_CHECK(startScan(tab, sc, se));
    updateRecord = fromTestRecord(sch, updates[0]);
    updateScan(tab,r,updateRecord,sc);
    TEST_CHECK(closeTable(tab));
    
    TEST_CHECK(shutdownRecordManager());

    TEST_CHECK(initRecordManager(NULL));
    
    TEST_CHECK(openTable(tab, "test_table_r"));

    MAKE_CONS(le, stringToValue("i7"));
    MAKE_ATTRREF(ri, 2);
    MAKE_BINOP_EXPR(fst, ri, le, OP_COMP_SMALLER);
    MAKE_UNOP_EXPR(se, fst, OP_BOOL_NOT);
    TEST_CHECK(startScan(tab, sc, se));

    while((rc = next(sc, r)) == RC_OK)
    {
        serializeRecord(r, sch);
        for(i = 0; i < numInserts; i++)
        {
            if (memcmp(fromTestRecord(sch, updateRec[i])->data,r->data,getRecordSize(sch)) == 0)
                foundScan[i] = TRUE;
        }
    }
    if (rc != RC_RM_NO_MORE_TUPLES)
        TEST_CHECK(rc);
    TEST_CHECK(closeScan(sc));

    ASSERT_TRUE(!foundScan[0], "Is not >7");
    ASSERT_TRUE(foundScan[4], ">7");
    ASSERT_TRUE(foundScan[9], ">7");

    // clean up
    TEST_CHECK(closeTable(tab));
    TEST_CHECK(deleteTable("test_table_r"));
    TEST_CHECK(shutdownRecordManager());

    freeRecord(r);
    free(tab);
    free(sc);
    freeExpr(sel);
    TEST_DONE();
}





Schema *
testSchema (void)
{
    Schema *rlt;
    char *names[] = { "a", "b", "c" };
    DataType dt[] = { DT_INT, DT_STRING, DT_INT };
    int sizes[] = { 0, 4, 0 };
    int key[] = {0};
    int i;
    char **cpNames = (char **) malloc(sizeof(char*) * 3);
    DataType *cpDt = (DataType *) malloc(sizeof(DataType) * 3);
    int *cpSizes = (int *) malloc(sizeof(int) * 3);
    int *cpKeys = (int *) malloc(sizeof(int));

    for(i = 0; i < 3; i++)
    {
        cpNames[i] = (char *) malloc(2);
        strcpy(cpNames[i], names[i]);
    }
    memcpy(cpDt, dt, sizeof(DataType) * 3);
    memcpy(cpSizes, sizes, sizeof(int) * 3);
    memcpy(cpKeys, key, sizeof(int));

    rlt = createSchema(3, cpNames, cpDt, cpSizes, 1, cpKeys);

    return rlt;
}

Record *
fromTestRecord (Schema *sch, TestRecord tr)
{
    return testRecord(sch, tr.one, tr.two, tr.three);
}

Record *
testRecord(Schema *sch, int one, char *two, int three)
{
    Record *rlt;
    Value *v;

    TEST_CHECK(createRecord(&rlt, sch));

    MAKE_VALUE(v, DT_INT, one);
    TEST_CHECK(setAttr(rlt, sch, 0, v));
    freeVal(v);

    MAKE_STRING_VALUE(v, two);
    TEST_CHECK(setAttr(rlt, sch, 1, v));
    freeVal(v);

    MAKE_VALUE(v, DT_INT, three);
    TEST_CHECK(setAttr(rlt, sch, 2, v));
    freeVal(v);

    return rlt;
}
