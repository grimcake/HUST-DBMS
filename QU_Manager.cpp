#include "StdAfx.h"
#include "QU_Manager.h"
#include "RM_Manager.h"

void Init_Result(SelResult * res){
	res->next_res = NULL;
}

void Destory_Result(SelResult * res){
	for(int i = 0;i<res->row_num;i++){
		for(int j = 0;j<res->col_num;j++){
			delete[] res->res[i][j];
		}
		delete[] res->res[i];
	}
	if(res->next_res != NULL){
		Destory_Result(res->next_res);
	}
}

RC Query(char * sql,SelResult * res){
	RC rc;
	sqlstr *sqlStr;
	sqlStr = get_sqlstr();
	rc = parse(sql, sqlStr);

	if (rc)
	{
		return rc;
	}
	SelResult *selRes;
	rc = Select(sqlStr->sstr.sel.nSelAttrs, sqlStr->sstr.sel.selAttrs, sqlStr->sstr.sel.nRelations, sqlStr->sstr.sel.relations, sqlStr->sstr.sel.nConditions, sqlStr->sstr.sel.conditions, selRes);
	if (rc)
	{
		return rc;
	}

	return SUCCESS;
}

RC Select(int nSelAttrs, RelAttr **selAttrs, int nRelations, char **relations, int nConditions, Condition *conditions, SelResult * res)
{
	RC rc;
	RM_FileHandle table;
	RM_FileScan scan;
	RM_Record rec;
	table.bOpen = false;

	if (rc = RM_OpenFile("SYSTABLES", &table))
	{
		return SQL_SYNTAX;
	}

	// µ•±Ì≤È—Ø
	if (nRelations == 1)
	{
		SingleSelect(nSelAttrs, selAttrs, nRelations, relations, nConditions, conditions, res);
	}
	return SUCCESS;
}
RC SingleSelect(int nSelAttrs, RelAttr **selAttrs, int nRelations, char **relations, int nConditions, Condition *conditions, SelResult * res)
{
	SelResult *resHead = res;
	RM_FileHandle fileHandle;
	RM_FileScan fileScan;
	RM_Record rec;
	Con cons[2];
	cons[0].attrType = chars;
	cons[0].bLhsIsAttr = 1;
	cons[0].LattrOffset = 0;
	cons[0].LattrLength = strlen(*relations) + 1;
	cons[0].compOp = EQual;
	cons[0].bRhsIsAttr = 0;
	cons[0].Rvalue = *relations;
	if (RM_OpenFile("SYSCOLUMNS", &fileHandle))
	{
		return SQL_SYNTAX;
	}
}
