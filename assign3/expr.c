#include <string.h>
#include <stdlib.h>

#include "dberror.h"
#include "record_mgr.h"
#include "expr.h"
#include "tables.h"

// implementations
RC 
valueEquals (Value *le, Value *ri, Value *rlt)
{
  if(le->dt != ri->dt)
    THROW(RC_RM_COMPARE_VALUE_OF_DIFFERENT_DATATYPE, "The Comparision is for values of the same data type");

  rlt->dt = DT_BOOL;
  
  switch(le->dt) {
  case DT_INT:
    rlt->v.boolV = (le->v.intV == ri->v.intV);
    break;
  case DT_FLOAT:
    rlt->v.boolV = (le->v.floatV == ri->v.floatV);
    break;
  case DT_BOOL:
    rlt->v.boolV = (le->v.boolV == ri->v.boolV);
    break;
  case DT_STRING:
    rlt->v.boolV = (strcmp(le->v.stringV, ri->v.stringV) == 0);
    break;
  }

  return RC_OK;
}

RC 
valueSmaller (Value *le, Value *ri, Value *rlt)
{
  if(le->dt != ri->dt)
    THROW(RC_RM_COMPARE_VALUE_OF_DIFFERENT_DATATYPE, "The comparision is for values of same data types");

  rlt->dt = DT_BOOL;
  
  switch(le->dt) {
  case DT_INT:
    rlt->v.boolV = (le->v.intV < ri->v.intV);
    break;
  case DT_FLOAT:
    rlt->v.boolV = (le->v.floatV < ri->v.floatV);
    break;
  case DT_BOOL:
    rlt->v.boolV = (le->v.boolV < ri->v.boolV);
  case DT_STRING:
    rlt->v.boolV = (strcmp(le->v.stringV, ri->v.stringV) < 0);
    break;
  }

  return RC_OK;
}

RC 
boolNot (Value *input, Value *rlt)
{
  if (input->dt != DT_BOOL)
    THROW(RC_RM_BOOLEAN_EXPR_ARG_IS_NOT_BOOLEAN, "NOT needs only bool input");
  rlt->dt = DT_BOOL;
  rlt->v.boolV = !(input->v.boolV);

  return RC_OK;
}

RC
boolAnd (Value *le, Value *ri, Value *rlt)
{
  if (le->dt != DT_BOOL || ri->dt != DT_BOOL)
    THROW(RC_RM_BOOLEAN_EXPR_ARG_IS_NOT_BOOLEAN, "AND requires only bool inputs");
  rlt->v.boolV = (le->v.boolV && ri->v.boolV);

  return RC_OK;
}

RC
boolOr (Value *le, Value *ri, Value *rlt)
{
  if (le->dt != DT_BOOL || ri->dt != DT_BOOL)
    THROW(RC_RM_BOOLEAN_EXPR_ARG_IS_NOT_BOOLEAN, "OR requires only bool inputs");
  rlt->v.boolV = (le->v.boolV || ri->v.boolV);

  return RC_OK;
}

RC
evalExpr (Record *record, Schema *sch, Expr *expr, Value **rlt)
{
  Value *lIn;
  Value *rIn;
  MAKE_VALUE(*rlt, DT_INT, -1);

  switch(expr->type)
    {
    case EXPR_OP:
      {
      Operator *op = expr->expr.op;
      bool twoArgs = (op->type != OP_BOOL_NOT);
      
      
      CHECK(evalExpr(record, sch, op->args[0], &lIn));
      if (twoArgs)
	CHECK(evalExpr(record, sch, op->args[1], &rIn));

      switch(op->type) 
	{
	case OP_BOOL_NOT:
	  CHECK(boolNot(lIn, *rlt));
	  break;
	case OP_BOOL_AND:
	  CHECK(boolAnd(lIn, rIn, *rlt));
	  break;
	case OP_BOOL_OR:
	  CHECK(boolOr(lIn, rIn, *rlt));
	  break;
	case OP_COMP_EQUAL:
	  CHECK(valueEquals(lIn, rIn, *rlt));
	  break;
	case OP_COMP_SMALLER:
	  CHECK(valueSmaller(lIn, rIn, *rlt));
	  break;
	default:
	  break;
	}

      // cleanup
      freeVal(lIn);
      if (twoArgs)
	freeVal(rIn);
      }
      break;
    case EXPR_CONST:
      CPVAL(*rlt,expr->expr.cons);
      break;
    case EXPR_ATTRREF:
      free(*rlt);
      CHECK(getAttr(record, sch, expr->expr.attrRef, rlt));
      break;
    }

  return RC_OK;
}

RC
freeExpr (Expr *expr)
{
  switch(expr->type) 
    {
    case EXPR_OP:
      {
      Operator *op = expr->expr.op;
      switch(op->type) 
	{
	case OP_BOOL_NOT:
	  freeExpr(op->args[0]);
	  break;
	default:
	  freeExpr(op->args[0]);
	  freeExpr(op->args[1]);
	  break;
	}
      free(op->args);
      }
      break;
    case EXPR_CONST:
      freeVal(expr->expr.cons);
      break;
    case EXPR_ATTRREF:
      break;
    }
  free(expr);
  
  return RC_OK;
}

void 
freeVal (Value *val)
{
  if (val->dt == DT_STRING)
    free(val->v.stringV);
  free(val);
}

