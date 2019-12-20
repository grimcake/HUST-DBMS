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

	// 单表查询
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

	// 用于在系统表和系统列中查询设置的条件
	// 查询的值是表名，即relations，不管在系统表还是系统列中，
	// 偏移都是0，所以共用这个条件
	Con syscon;
	syscon.attrType = chars;
	syscon.bLhsIsAttr = 1;
	syscon.LattrOffset = 0;
	syscon.LattrLength = strlen(*relations) + 1;
	syscon.compOp = EQual;
	syscon.bRhsIsAttr = 0;
	syscon.Rvalue = *relations;

	if (strcmp((*selAttrs)->attrName, "*") == 0)
	{
		// 查询所有属性
		// 扫描系统表文件，获取relation表名所在的记录
		if (RM_OpenFile("SYSTABLES", &fileHandle))
		{
			return SQL_SYNTAX;
		}
		OpenScan(&fileScan, &fileHandle, 1, &syscon);
		if (GetNextRec(&fileScan, &rec))
		{
			return SQL_SYNTAX;
		}
		resHead->col_num = (int)*(&rec.pData + 21);
		CloseScan(&fileScan);

		// 扫描系统列文件，获取每个字段有关信息
		// 查询结果显示的方式第一行是属性名
		if (RM_OpenFile("SYSCOLUMNS", &fileHandle))
		{
			return SQL_SYNTAX;
		}
		OpenScan(&fileScan, &fileHandle, 1, &syscon);
		for (int i = 0; i < resHead->col_num; i++)
		{
			if (GetNextRec(&fileScan, &rec) != SUCCESS)
			{
				return SQL_SYNTAX;
			}
			Column* col = (Column*)rec.pData;
			memcpy(&resHead->type[i], &col->attrtype, sizeof(int));
			memcpy(&resHead->length[i], &col->attrlength, sizeof(int));
			memcpy(&resHead->fields[i], &col->attrname, 21);
		}
		CloseScan(&fileScan);
		RM_CloseFile(&fileHandle);
	}
	else
	{
		resHead->col_num = nSelAttrs;
		syscon.attrType = chars;
		syscon.bLhsIsAttr = 1;
		syscon.LattrOffset = 21; // 以属性名为查询条件，在系统列文件结构中的偏移
		syscon.LattrLength = 21;
		syscon.compOp = EQual;
		syscon.bRhsIsAttr = 0;

		if (RM_OpenFile("SYSCLOUMNS", &fileHandle))
		{
			return SQL_SYNTAX;
		}
		for (int i = 0; i < resHead->col_num; i++)
		{
			syscon.Rvalue = selAttrs[i]->attrName;
			OpenScan(&fileScan, &fileHandle, 2, &syscon);
			if (GetNextRec(&fileScan, &rec))
			{
				return SQL_SYNTAX;
			}
			Column* col = (Column*)rec.pData;
			memcpy(&resHead->type[i], &col->attrtype, sizeof(int));
			memcpy(&resHead->length[i], &col->attrlength, sizeof(int));
			memcpy(&resHead->fields[i], &col->attrname, 21);
			CloseScan(&fileScan);
		}
	}

	// 根据条件查询

}
