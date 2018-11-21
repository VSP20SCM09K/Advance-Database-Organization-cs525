#include <string.h>
#include <stdlib.h>
#include "btree_mgr.h"
#include "record_mgr.h"
#include "buffer_mgr.h"
#include "storage_mgr.h"

typedef struct RM_BtreeNode {
    Value *keys;
    void **ptr;
    struct RM_BtreeNode *pptr;
    bool isLeaf;
    int KeCounts;
    int pos; 
    
} RM_BtreeNode;


typedef struct RM_BScan_mgmt{
    int tot_Scan;
    RM_BtreeNode *cur;
    int index;
}RM_BScan_mgmt;

typedef struct RM_bTree_mgmtData{
    BM_BufferPool *bp;
    int maxKeNum;
    int num;
    
}RM_bTree_mgmtData;


RM_BtreeNode *root = NULL;
int numNodValue = 0;
int globalPos = 0;
int sizeofNod = 0;
char *s_s = NULL;
char *s = NULL;

Value empty;



bool strNoLarger(char *c, char *c1){
    if (strlen(c)<strlen(c1))
        return true;
    if (strlen(c)>strlen(c1))
        return false;
    return (strcmp(c, c1)<=0);
}

RM_BtreeNode *createNewNod(RM_BtreeNode *thisNode)
{
    if (thisNode == NULL)
        globalPos = -1;
    globalPos = 0;
    RM_BtreeNode *bTreeNode;
    
    
    bTreeNode = (RM_BtreeNode *)malloc(sizeof(RM_BtreeNode));
    bTreeNode->ptr = malloc(sizeofNod * sizeof(void *));
    bTreeNode->keys = malloc((sizeofNod - 1) * sizeof(Value));
    
    thisNode = NULL;
    
    bTreeNode->pptr = NULL;
    bTreeNode->KeCounts = 0;
    bTreeNode->isLeaf = FALSE;
    
    numNodValue += 1;
    
    
    
    return bTreeNode;
}
RC deleteNode(RM_BtreeNode *bTreeNode, int index)
{    
    RM_BtreeNode *brother;
    int position, i, j;
    
    
    bTreeNode->KeCounts--;
    int NumKeys = bTreeNode->KeCounts;
    if (bTreeNode->isLeaf){
        
        free(bTreeNode->ptr[index]);
        bTreeNode->ptr[index] = NULL;
        
        for (i = index; i < NumKeys;i++){
            bTreeNode->keys[i] = bTreeNode->keys[i + 1];
            globalPos = bTreeNode->pos;
            bTreeNode->ptr[i] = bTreeNode->ptr[i + 1];
        }
        bTreeNode->keys[i] = empty;
        bTreeNode->ptr[i] = NULL;
    }
    else{
        
        for (i = index - 1;i < NumKeys;i++){
            bTreeNode->keys[i] = bTreeNode->keys[i + 1];
            globalPos = bTreeNode->pos;
            bTreeNode->ptr[i + 1] = bTreeNode->ptr[i + 2];
        }
        bTreeNode->keys[i] = empty;
        bTreeNode->ptr[i + 1] = NULL;
    }// we figure out that it isnt the leaf
    
    if (bTreeNode->isLeaf)
        i = sizeofNod / 2;
    else
        i = (sizeofNod - 1) / 2;
    if (NumKeys >= i)
        return RC_OK;
    
    if (bTreeNode == root){
        if (root->KeCounts > 0)
            return RC_OK;
       
        RM_BtreeNode *newRoot = NULL;
        if (!root->isLeaf){

            newRoot = root->ptr[0];
            newRoot->pptr = NULL;
        }
        
        free(root->keys);
        root->keys = NULL;
        free(root->ptr);
        root->ptr = NULL;
        free(root);
        root = NULL;
        numNodValue -= 1;
        root = newRoot;
        
        return RC_OK;
    }
    
    RM_BtreeNode *parentNode = bTreeNode->pptr;
    position = 0;
    
    while(position < parentNode->KeCounts && parentNode->ptr[position] != bTreeNode) position++;
    
    if (position == 0)//leftmost
        brother = parentNode->ptr[1];
    else
        brother = parentNode->ptr[position - 1];
    if (bTreeNode->isLeaf)
        i = sizeofNod - 1;
    else
        i = sizeofNod - 2;
    
    if (brother->KeCounts + NumKeys <= i){
        //merging
        if (position == 0) {
            RM_BtreeNode *temp;
            temp = bTreeNode;
            bTreeNode = brother;
            brother = temp;
            position = 1;
            NumKeys = bTreeNode->KeCounts;
        }
        i = brother->KeCounts;
        if (!bTreeNode->isLeaf){
            brother->keys[i] = parentNode->keys[position - 1];
            i += 1;
            NumKeys += 1;
        }
        for (j=0;j<NumKeys;j++){
            brother->keys[i] = bTreeNode->keys[j];
            globalPos = brother->pos;
            brother->ptr[i] = bTreeNode->ptr[j];
            bTreeNode->keys[j] = empty;
            bTreeNode->ptr[j] = NULL;
            i++;
        }
    
        brother->KeCounts += NumKeys;
        brother->ptr[sizeofNod - 1] = bTreeNode->ptr[sizeofNod - 1];
        
        numNodValue -= 1;
        free(bTreeNode->keys);
        bTreeNode->keys = NULL;
        free(bTreeNode->ptr);
        bTreeNode->ptr = NULL;
        free(bTreeNode);
        bTreeNode = NULL;
        
        return deleteNode(parentNode, position);
    }
    //get one from brother
    int brotherNumKeys;
    if (position != 0) {
        if (!bTreeNode->isLeaf)// get from the left
            bTreeNode->ptr[NumKeys + 1] = bTreeNode->ptr[NumKeys];
        //shift to the right by 1
        for (i = NumKeys; i > 0; i--){
            bTreeNode->keys[i] = bTreeNode->keys[i - 1];
            globalPos = bTreeNode->pos;
            bTreeNode->ptr[i] = bTreeNode->ptr[i - 1];
        }
       
        if (bTreeNode->isLeaf){
            brotherNumKeys = brother->KeCounts - 1;
            bTreeNode->keys[0] = brother->keys[brotherNumKeys];
            parentNode->keys[position - 1] = bTreeNode->keys[0];
        }
        else{
            brotherNumKeys = brother->KeCounts;
            bTreeNode->keys[0] = parentNode->keys[position - 1];
            parentNode->keys[position - 1] = brother->keys[brotherNumKeys - 1];
        }
        bTreeNode->ptr[0] = brother->ptr[brotherNumKeys];
        brother->keys[brotherNumKeys] = empty;
        brother->ptr[brotherNumKeys] = NULL;
    }
    else {
        int temp = brother->KeCounts;
        
        if (bTreeNode->isLeaf){
            bTreeNode->keys[NumKeys] = brother->keys[0];
            bTreeNode->ptr[NumKeys] = brother->ptr[0];
            parentNode->keys[0] = brother->keys[1];
        }
        else {
            bTreeNode->keys[NumKeys] = parentNode->keys[0];
            bTreeNode->ptr[NumKeys + 1] = brother->ptr[0];
            parentNode->keys[0] = brother->keys[0];
        }
        
        for (i = 0; i < temp; i++){
            brother->keys[i] = brother->keys[i + 1];
            globalPos = brother->KeCounts;
            brother->ptr[i] = brother->ptr[i + 1];
        }
        brother->ptr[brother->KeCounts] = NULL;
        brother->keys[brother->KeCounts] = empty;
    }
    
    bTreeNode->KeCounts++;
    brother->KeCounts--;
    return RC_OK;
}

// init and shutdown index manager

RC insertParent(RM_BtreeNode *left, RM_BtreeNode *right, Value key)
{
    RM_BtreeNode *pptr = left->pptr;
    int i = 0;
    int index = 0;
    
    
    if (pptr == NULL){
        //create parent
        RM_BtreeNode *NewRoot;
        
        NewRoot = createNewNod(NewRoot);
        NewRoot->keys[0] = key;
        NewRoot->KeCounts = 1;
        NewRoot->ptr[0] = left;
        NewRoot->ptr[1] = right;
        left->pptr = NewRoot;
        right->pptr = NewRoot;
        
        root = NewRoot;
        return RC_OK;
    }
    else{
        
        while ( index < pptr->KeCounts && pptr->ptr[index] != left) index++;
        globalPos = pptr->pos;
        if (pptr->KeCounts < sizeofNod - 1){
            
            for (int i = pptr->KeCounts;i > index;i--){
                pptr->keys[i] = pptr->keys[i-1];
                globalPos = pptr->pos;
                pptr->ptr[i+1] = pptr->ptr[i];
            }
            pptr->keys[index] = key;
            pptr->ptr[index+1] = right;
            pptr->KeCounts += 1;
            
            return RC_OK;
        }
    }
    //we split the node since there is no empty space
    i = 0;
    int middleLoc;
    RM_BtreeNode **tempNode, *newNode;
    Value *tempKeys;
    
    tempNode = malloc((sizeofNod + 1) * sizeof(RM_BtreeNode *));
    tempKeys = malloc(sizeofNod * sizeof(Value));
    
    for (i=0; i < sizeofNod + 1;i++){
        if (i < index + 1)
            tempNode[i] = pptr->ptr[i];
        else if (i == index + 1)
            tempNode[i] = right;
        else{
            globalPos = pptr->pos;
            tempNode[i] = pptr->ptr[i-1];
        }
    }
    for (i = 0;i < sizeofNod;i++){
        if (i < index)
            tempKeys[i] = pptr->keys[i];
        else if (i == index)
            tempKeys[i] = key;
        else{
            globalPos = tempKeys[i].v.intV;
            tempKeys[i] = pptr->keys[i-1];
        }
    }
    
    if (sizeofNod % 2 == 0)
        middleLoc = sizeofNod / 2;
    else
        middleLoc = sizeofNod / 2 + 1;
    pptr->KeCounts = middleLoc - 1;
    for (i = 0;i < middleLoc - 1;i++){
        pptr->ptr[i] = tempNode[i];
        pptr->keys[i] = tempKeys[i];
    }
    pptr->ptr[i] = tempNode[i];
    newNode = createNewNod(newNode);
    newNode->KeCounts = sizeofNod - middleLoc;
    for (i=middleLoc;i <= sizeofNod;i++){
        newNode->ptr[i - middleLoc] = tempNode[i];
        globalPos = newNode->pos;
        newNode->keys[i - middleLoc] = tempKeys[i];
    }
    newNode->pptr = pptr->pptr;
    
    Value t;
    t = tempKeys[middleLoc - 1];
    free(tempNode);
    tempNode = NULL;
    free(tempKeys);
    tempKeys = NULL;
    
    return insertParent(pptr, newNode, t);
}
RC shutdownIndexManager ()
{
    return RC_OK;
}
RC createBtree (char *idxId, DataType keyType, int n)
{
    if(idxId == NULL) return RC_IM_KEY_NOT_FOUND;
    RC rc;
    
    rc = createPageFile(idxId);
    if (rc != RC_OK)
        return rc;
    SM_FileHandle fhandle;
    rc = openPageFile(idxId, &fhandle);
    if (rc != RC_OK)
        return rc;
    
    SM_PageHandle pageData = (SM_PageHandle)malloc(sizeof(PAGE_SIZE));
    memcpy(pageData, &keyType, sizeof(int));
    pageData += sizeof(int);
    memcpy(pageData, &n, sizeof(int));
    pageData -= sizeof(int);
    
    rc = writeCurrentBlock(&fhandle, pageData);
    if (rc != RC_OK)
        return rc;
    rc = closePageFile(&fhandle);
    if (rc != RC_OK)
        return rc;
    
    free(pageData);
    pageData = NULL;
    
    return RC_OK;
}

RC initIndexManager (void *mgmtData)
{
    root = NULL;
    numNodValue = 0;
    sizeofNod = 0;
    empty.dt = DT_INT;
    empty.v.intV = 0;
    return RC_OK;
}




RC openBtree (BTreeHandle **tree, char *idxId)
{
    if(idxId == NULL) return RC_IM_KEY_NOT_FOUND;
    RC rc;
    BM_BufferPool *bm = MAKE_POOL();
    BM_PageHandle *page = MAKE_PAGE_HANDLE();
    *tree = (BTreeHandle *) malloc (sizeof(BTreeHandle));
    
    rc = initBufferPool(bm, idxId, 10, RS_CLOCK, NULL);
    if (rc != RC_OK)
        return rc;
    rc = pinPage(bm, page, 0);
    if (rc != RC_OK)
        return rc;
    
    int type;
    memcpy(&type, page->data, sizeof(int));
    (*tree)->keyType = (DataType)type;
    page->data += sizeof(int);
    int n;
    memcpy(&n, page->data, sizeof(int));
    page->data -= sizeof(int);
    
    RM_bTree_mgmtData *bTreeMgmt = (RM_bTree_mgmtData *) malloc (sizeof(RM_bTree_mgmtData));
    bTreeMgmt->num = 0;
    bTreeMgmt->maxKeNum = n;
    bTreeMgmt->bp = bm;
    (*tree)->mgmtData = bTreeMgmt;
    
    free(page);
    page = NULL;
    
    return RC_OK;
}

RC closeBtree (BTreeHandle *tree)
{
    if(tree == NULL) return RC_IM_KEY_NOT_FOUND;
    RC rc;
    
    tree->idxId = NULL;
    RM_bTree_mgmtData *bTreeMgmt = (RM_bTree_mgmtData *)tree->mgmtData;
    
    rc = shutdownBufferPool(bTreeMgmt->bp);
    if (rc != RC_OK)
        return rc;
    
    free(bTreeMgmt);
    bTreeMgmt = NULL;
    free(tree);
    tree = NULL;
    free(root);
    root = NULL;
    
    return RC_OK;
}

RC deleteBtree (char *idxId)
{
    if(idxId == NULL) return RC_IM_KEY_NOT_FOUND;
    
    RC rc = destroyPageFile(idxId);
    if (rc != RC_OK)
        return rc;
    
    return RC_OK;
}
RC getNumEntries (BTreeHandle *tree, int *result)
{
    if(tree == NULL) return RC_IM_KEY_NOT_FOUND;
    
    (*result) = ((RM_bTree_mgmtData *)tree->mgmtData)->num;
    
    return RC_OK;
}
// functions below correspond to gaining acess of the B tree
RC getNumNodes (BTreeHandle *tree, int *result)
{
    if(tree == NULL) return RC_IM_KEY_NOT_FOUND;
    
    (*result) = numNodValue;
    
    return RC_OK;
}

RC getKeyType (BTreeHandle *tree, DataType *result)
{
    if(tree == NULL) return RC_IM_KEY_NOT_FOUND;
    
    (*result) = tree->keyType;
    
    return RC_OK;
}
RC findKey (BTreeHandle *tree, Value *key, RID *result)
{
    
    if ((tree == NULL)||(key == NULL)||(root == NULL)) return RC_IM_KEY_NOT_FOUND;
    
    RM_BtreeNode *leaf;
    int i = 0;
    
    //findleaf
    leaf = root;
    while (!leaf->isLeaf){
        
        s = serializeValue(&leaf->keys[i]);
        s_s = serializeValue(key);
        while ((i < leaf->KeCounts) && strNoLarger(s, s_s)){
            free(s);
            s = NULL;
            i++;
            if (i < leaf->KeCounts) s = serializeValue(&leaf->keys[i]);
        }
        free(s);
        s = NULL;
        free(s_s);
        s_s = NULL;
        
        leaf = (RM_BtreeNode *)leaf->ptr[i];
        i = 0;
    }
    
    s = serializeValue(&leaf->keys[i]);
    s_s = serializeValue(key);
    while ((i < leaf->KeCounts) && (strcmp(s, s_s)!=0)){
        free(s);
        s = NULL;
        i++;
        if (i < leaf->KeCounts) s = serializeValue(&leaf->keys[i]);
    }
    free(s);
    s = NULL;
    free(s_s);
    s_s = NULL;
    
    if (i >= leaf->KeCounts)
        return RC_IM_KEY_NOT_FOUND;
    else{
        result->page = ((RID *)leaf->ptr[i])->page;
        result->slot = ((RID *)leaf->ptr[i])->slot;
        return RC_OK;
    }
}

// index access

RC insertKey (BTreeHandle *tree, Value *key, RID rid)
{
    if ((tree == NULL)||(key == NULL)) return RC_IM_KEY_NOT_FOUND;
    
    RM_BtreeNode *leaf;
    int i = 0;
    
    //findleaf
    leaf = root;
    if (leaf != NULL)
    {
        while (!leaf->isLeaf)
        {
            
            s = serializeValue(&leaf->keys[i]);
            s_s = serializeValue(key);
            while ((i < leaf->KeCounts) && strNoLarger(s, s_s)){
                free(s);
                s = NULL;
                i++;
                if (i < leaf->KeCounts) s = serializeValue(&leaf->keys[i]);
            }
            free(s);
            s = NULL;
            free(s_s);
            s_s = NULL;
            
            leaf = (RM_BtreeNode *)leaf->ptr[i];
            i = 0;
        }
    }
    
    RM_bTree_mgmtData *bTreeMgmt = (RM_bTree_mgmtData *)tree->mgmtData;
    bTreeMgmt->num += 1;
    
    if (!leaf)
    {
        sizeofNod = bTreeMgmt->maxKeNum + 1;
        
        root = createNewNod(root);
        RID *rec = (RID *) malloc (sizeof(RID));
        rec->page = rid.page;
        rec->slot = rid.slot;
        root->ptr[0] = rec;
        root->keys[0] = *key;
        root->ptr[sizeofNod - 1] = NULL;
        root->isLeaf = true;
        root->KeCounts++;
    }
    else
    {
        int index = 0;
        
        s = serializeValue(&leaf->keys[index]);
        s_s = serializeValue(key);
        while ((index < leaf->KeCounts) && strNoLarger(s, s_s)){
            free(s);
            s = NULL;
            index++;
            if (index < leaf->KeCounts) s = serializeValue(&leaf->keys[index]);
        }
        free(s);
        s = NULL;
        free(s_s);
        s_s = NULL;
        
        
        if (leaf->KeCounts < sizeofNod - 1)
        {
            //empty slot
            for (int i = leaf->KeCounts;i > index;i--){
                leaf->keys[i] = leaf->keys[i-1];
                globalPos = leaf->pos;
                leaf->ptr[i] = leaf->ptr[i-1];
            }
            RID *rec = (RID *) malloc (sizeof(RID));
            rec->page = rid.page;
            rec->slot = rid.slot;
            leaf->keys[index] = *key;
            leaf->ptr[index] = rec;
            leaf->KeCounts += 1;
        }
        else
        {
            RM_BtreeNode *newLeafNod;
            RID **NodeRID;
            Value *NodeKeys;
            int middleLoc = 0;
            NodeRID = malloc(sizeofNod * sizeof(RID *));
            NodeKeys = malloc(sizeofNod * sizeof(Value));
            //full node
            for (i=0;i < sizeofNod;i++)
            {
                if (i == index){
                    RID *newValue = (RID *) malloc (sizeof(RID));
                    newValue->page = rid.page;
                    newValue->slot = rid.slot;
                    NodeRID[i] = newValue;
                    NodeKeys[i] = *key;
                }
                else if (i < index){
                    NodeRID[i] = leaf->ptr[i];
                    globalPos = NodeRID[i]->page;
                    NodeKeys[i] = leaf->keys[i];
                    middleLoc = sizeofNod % 2 == 0;
                }
                else{//i > index
                    middleLoc = globalPos;
                    NodeRID[i] = leaf->ptr[i-1];
                    globalPos = NodeRID[i]->page;
                    NodeKeys[i] = leaf->keys[i-1];
                }
            }
            
            middleLoc = sizeofNod / 2 + 1;
            //old leaf
            for (i=0;i < middleLoc;i++)
            {
                leaf->ptr[i] = NodeRID[i];
                leaf->keys[i] = NodeKeys[i];
            }
            //new leaf
            newLeafNod = createNewNod(newLeafNod);
            newLeafNod->isLeaf = true;
            newLeafNod->pptr = leaf->pptr;
            newLeafNod->KeCounts = sizeofNod - middleLoc;
            for (i=middleLoc;i < sizeofNod;i++){
                newLeafNod->ptr[i - middleLoc] = NodeRID[i];
                newLeafNod->keys[i - middleLoc] = NodeKeys[i];
            }
            //add to link list
            newLeafNod->ptr[sizeofNod - 1] = leaf->ptr[sizeofNod - 1];
            leaf->KeCounts = middleLoc;
            leaf->ptr[sizeofNod - 1] = newLeafNod;
            
            free(NodeRID);
            NodeRID = NULL;
            free(NodeKeys);
            NodeKeys = NULL;
            
            RC rc = insertParent(leaf, newLeafNod, newLeafNod->keys[0]);
            if (rc != RC_OK)
                return rc;
        }
    }
    
    tree->mgmtData = bTreeMgmt;
    return RC_OK;
}
RC deleteKey (BTreeHandle *tree, Value *ke)
{
    if ((tree == NULL)||(ke == NULL)) return RC_IM_KEY_NOT_FOUND;
    RC rc;
    RM_bTree_mgmtData *bTreeMgmt = (RM_bTree_mgmtData *)tree->mgmtData;
    bTreeMgmt->num -= 1;
    RM_BtreeNode *leaf;
    int i = 0;
  
    leaf = root;
    if (leaf != NULL)
    {
        while (!leaf->isLeaf)
        {
           
            s = serializeValue(&leaf->keys[i]);
            s_s = serializeValue(ke);
            while ((i < leaf->KeCounts) && strNoLarger(s, s_s)){
                free(s);
                s = NULL;
                i++;
                if (i < leaf->KeCounts) s = serializeValue(&leaf->keys[i]);
            }
            free(s);
            s = NULL;
            free(s_s);
            s_s = NULL;
            
            leaf = (RM_BtreeNode *)leaf->ptr[i];
            i = 0;
        }
        
        s = serializeValue(&leaf->keys[i]);
        s_s = serializeValue(ke);
        while ((i < leaf->KeCounts) && (strcmp(s, s_s)!=0)){
            free(s);
            s = NULL;
            i++;
            if (i < leaf->KeCounts) s = serializeValue(&leaf->keys[i]);
        }
        free(s);
        s = NULL;
        free(s_s);
        s_s = NULL;
       
        if (i < leaf->KeCounts)
        {
            rc = deleteNode(leaf, i);
            if (rc != RC_OK)
                return rc;
        }
    }
    
    tree->mgmtData = bTreeMgmt;
    return RC_OK;
}

RC openTreeScan (BTreeHandle *tree, BT_ScanHandle **handle)
{
    if(tree == NULL) return RC_IM_KEY_NOT_FOUND;
    
    *handle = (BT_ScanHandle *) malloc (sizeof(BT_ScanHandle));
    (*handle)->tree = tree;
    RM_BScan_mgmt *scanMgmt = (RM_BScan_mgmt *) malloc (sizeof(RM_BScan_mgmt));
    scanMgmt->cur = NULL;
    scanMgmt->index = 0;
    scanMgmt->tot_Scan = 0;
    (*handle)->mgmtData = scanMgmt;
    
    return RC_OK;
}

RC nextEntry (BT_ScanHandle *handle, RID *result)
{
    if(handle == NULL) return RC_IM_KEY_NOT_FOUND;
    RC rc;
    RM_BScan_mgmt *scanMgmt = (RM_BScan_mgmt *) handle->mgmtData;
    int totalRes = 0;
    
    rc = getNumEntries(handle->tree, &totalRes);
    if (rc != RC_OK)
        return rc;
    if(scanMgmt->tot_Scan >= totalRes){
        return RC_IM_NO_MORE_ENTRIES;
    }
    
    RM_BtreeNode *leaf = root;
    if(scanMgmt->tot_Scan == 0){
        while (!leaf->isLeaf)
            leaf = leaf->ptr[0];
        scanMgmt->cur = leaf;
    }
    
    if(scanMgmt->index == scanMgmt->cur->KeCounts){
        scanMgmt->cur = (RM_BtreeNode *)scanMgmt->cur->ptr[((RM_bTree_mgmtData *)handle->tree->mgmtData)->maxKeNum];
        scanMgmt->index = 0;
    }
    
    RID *ridRes = (RID *) malloc (sizeof(RID));
    ridRes = (RID *)scanMgmt->cur->ptr[scanMgmt->index];
    scanMgmt->index++;
    scanMgmt->tot_Scan++;
    handle->mgmtData = scanMgmt;
    
    result->page = ridRes->page;
    result->slot = ridRes->slot;
    
    return RC_OK;
}

RC closeTreeScan (BT_ScanHandle *handle)
{
    RM_BScan_mgmt *mgmt = handle->mgmtData;
    free(mgmt);
    mgmt = NULL;
    free(handle);
    handle = NULL;
    
    return RC_OK;
}

// debug and test functions

int DFS(RM_BtreeNode *bTreeNode)
{
    bTreeNode->pos = globalPos++;
    if (bTreeNode->isLeaf)
        return 0;
    for (int i=0;i<=bTreeNode->KeCounts;i++)
    {
        DFS(bTreeNode->ptr[i]);
    }
    return 0;
}

int walk(RM_BtreeNode *bTreeNode, char *result)
{
    char *line = (char *)malloc(100);
    
    char *t = (char *) malloc(10);
    int i = 0;
    strcpy(line, "(");
    sprintf(t, "%d", bTreeNode->pos);
    strcat(line, t);
    strcat(line, ")[");
    if (bTreeNode->isLeaf){
        
        i = 0;
        for (i=0;i<bTreeNode->KeCounts;i++){
            
            sprintf(t, "%d.%d,", ((RID *)bTreeNode->ptr[i])->page, ((RID *)bTreeNode->ptr[i])->slot);
            strcat(line, t);
            
            s = serializeValue(&bTreeNode->keys[i]);
            strcat(line, s);
            free(s);
            s = NULL;
            strcat(line, ",");
        }
        if (bTreeNode->ptr[sizeofNod - 1] != NULL){
            
            i = ((RM_BtreeNode *)bTreeNode->ptr[sizeofNod - 1])->pos;
            sprintf(t, "%d", i);
            strcat(line, t);
        }
        else{
            line[strlen(line)-1] = 0;
        }
        strcat(line, "]\n");
    }
    else{
        
        i = 0;
        for (i=0;i<bTreeNode->KeCounts;i++){
            sprintf(t, "%d,", ((RM_BtreeNode *)bTreeNode->ptr[i])->pos);
            strcat(line, t);
            
            s = serializeValue(&bTreeNode->keys[i]);
            strcat(line, s);
            free(s);
            s = NULL;
            strcat(line, ",");
        }
        if (((RM_BtreeNode *) bTreeNode->ptr[i]) != NULL){
            sprintf(t, "%d", ((RM_BtreeNode *)bTreeNode->ptr[i])->pos);
            strcat(line, t);
        }
        else{
            line[strlen(line)-1] = 0;//not a leaf
        }
        strcat(line, "]\n");
    }
    strcat(result,line);
    free(line);
    line = NULL;
    free(t);
    t = NULL;
    if (!bTreeNode->isLeaf){
        for (int i=0;i<=bTreeNode->KeCounts;i++)
        {
            walk(bTreeNode->ptr[i],result);
        }
    }
    return 0;
}

char *printTree (BTreeHandle *tree)
{
    if (root == NULL)
        return NULL;
    globalPos = 0;
    int lenth = DFS(root);
    lenth = 1000;
    char *result = malloc(lenth*sizeof(char));
    walk(root, result);
    return result;
}