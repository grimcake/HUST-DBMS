#include "stdafx.h"
#include "RM_Manager.h"
#include "PF_Manager.h"
#include "BitMap.h"
#include "str.h"

#include <assert.h>
#include <iostream>


RC OpenScan(RM_FileScan *rmFileScan,RM_FileHandle *fileHandle,int conNum,Con *conditions)//初始化扫描
{
	RC rc;
	if (rmFileScan->bOpen == true)
	{
		return RM_FSOPEN;
	}
	// 如果文件没有数据
	if (fileHandle->pfFileHandle.pFileSubHeader->nAllocatedPages <= 2)
	{
		rmFileScan->pn = rmFileScan->sn = 0;
		return SUCCESS;
	}
	// 初始化
	rmFileScan->bOpen = true;
	rmFileScan->pRMFileHandle = fileHandle;
	rmFileScan->conNum = conNum;
	rmFileScan->conditions = conditions;
	// 设置扫描的首页面
	if (rc = GetThisPage(&fileHandle->pfFileHandle, 2, &rmFileScan->PageHandle))
	{
		return rc;
	}
	rmFileScan->pn = 2;
	rmFileScan->sn = 0;
	return SUCCESS;
}

RC GetNextRec(RM_FileScan *rmFileScan,RM_Record *rec)
{
	if (rmFileScan->bOpen == false)
	{
		return RM_FSCLOSED;
	}
	// 文件无内容
	if (rmFileScan->pn == 0)
	{
		return RM_NOMORERECINMEM;
	}
	while (true)
	{
		if (GetThisPage(&rmFileScan->pRMFileHandle->pfFileHandle, rmFileScan->pn, &rmFileScan->PageHandle))
		{
			return FAIL;
		}
		// 寻找下一个有内容的插槽
		BitMap bm(rmFileScan->pRMFileHandle->recOffset, rmFileScan->PageHandle.pFrame->page.pData);
		while (bm.FirstBit(rmFileScan->sn, 1) != -1)
		{ 
			Con* condition;
			bool correct = true;
			// 该条记录所在的位置
			int recOffset = rmFileScan->pRMFileHandle->recOffset + rmFileScan->sn*rmFileScan->pRMFileHandle->recSize;
			char* recAddr = rmFileScan->PageHandle.pFrame->page.pData + recOffset;

			int leftVal, rightVal;
			int leftF, rightF;
			char *leftStr, *rightStr;

			// 遍历判断每一个条件
			int conNumber;
			for (conNumber = 0; conNumber < rmFileScan->conNum; conNumber++)
			{
				condition = (Con*)(rmFileScan->conditions + conNumber);
				switch (condition->attrType)
				{
				case ints:
					leftVal = (condition->bLhsIsAttr == 1) ? *((int*)(recAddr + condition->LattrOffset)) : *((int*)condition->Lvalue);
					if (condition->compOp == NO_OP)
					{
						if (leftVal == 0)
						{
							correct = false;
							break;
						}
						else
						{
							break;
						}
					}
					rightVal = (condition->bRhsIsAttr == 1) ? *((int *)(recAddr + condition->RattrOffset)) : *((int *)condition->Rvalue);
					correct = CmpVal(leftVal, rightVal, condition->compOp);
					break;
				case floats:
					leftVal = (condition->bLhsIsAttr == 1) ? *((float*)(recAddr + condition->LattrOffset)) : *((float*)condition->Lvalue);
					if (condition->compOp == NO_OP)
					{
						if (leftVal == 0)
						{
							correct = false;
							break;
						}
						else
						{
							break;
						}
					}
					rightVal = (condition->bRhsIsAttr == 1) ? *((float *)(recAddr + condition->RattrOffset)) : *((float *)condition->Rvalue);
					correct = CmpVal(leftVal, rightVal, condition->compOp);
					break;
				case chars:
					if (condition->compOp == NO_OP)
					{
						correct = false;
						break;
					}
					leftStr = (condition->bLhsIsAttr == 1) ? (recAddr + condition->LattrOffset) : (char *)condition->Lvalue;
					rightStr = (condition->bRhsIsAttr == 1) ? (recAddr + recOffset + condition->RattrOffset) : (char *)condition->Rvalue;
					//比较两个字符串
					correct = CmpStr(leftStr, rightStr, condition->compOp);
					break;
				}
				if (!correct)
				{
					break;
				}
			}
			// 满足所有的比较条件
			if (conNumber == rmFileScan->conNum)
			{
				rec->bValid = true;
				rec->pData = recAddr;
				rec->rid.bValid = true;
				rec->rid.pageNum = rmFileScan->pn;
				rec->rid.slotNum = rmFileScan->sn;
				rmFileScan->sn++;
				return SUCCESS;
			}
			else
			{
				rmFileScan->sn++;
			}
		}
		UnpinPage(&rmFileScan->PageHandle);
		if (rmFileScan->pRMFileHandle->pageBitMap->FirstBit(rmFileScan->pn + 1, 1) == -1)
		{
			// 之后没有分配使用的页面
			return RM_NOMORERECINMEM;
		}
		// 找下一个有内容的页
		rmFileScan->pn = rmFileScan->pRMFileHandle->pageBitMap->FirstBit(rmFileScan->pn + 1, 1);
		if (rmFileScan->pn == -1)
		{
			return FAIL;
		}
		GetThisPage(&rmFileScan->pRMFileHandle->pfFileHandle, rmFileScan->pn, &rmFileScan->PageHandle);
		rmFileScan->sn = 0;
	}
	return FAIL;
}

RC GetRec (RM_FileHandle *fileHandle,RID *rid, RM_Record *rec) 
{
	rec->bValid = false;
	if (fileHandle->pageBitMap->Test(rid->pageNum) == 0 || rid->pageNum<2 || rid->slotNum>fileHandle->recPerPage-1)
	{
		return RM_INVALIDRID;
	}
	// 获取rid所在页
	PF_PageHandle pfPageHandle;
	RC rc;
	if (rc = GetThisPage(&fileHandle->pfFileHandle, rid->pageNum, &pfPageHandle))
	{
		return rc;
	}
	// 在该页位图中判断一下rid代表的记录是否有效
	BitMap bm(fileHandle->recOffset, &pfPageHandle.pFrame->page.pData[0]);
	if (bm.Test(rid->slotNum) == 0)
	{
		return RM_INVALIDRID;
	}

	// 获取rid所在插槽的地址
	rec->rid.bValid = true;
	rec->rid.pageNum = rid->pageNum;
	rec->rid.slotNum = rid->slotNum;
	// 插槽只存pData不存rid
	rec->pData = &pfPageHandle.pFrame->page.pData[fileHandle->recOffset+rec->rid.slotNum*fileHandle->recSize];
	


	UnpinPage(&pfPageHandle);
	return SUCCESS;
}

// 分为几种情况：
// 1.存在已分配的未满页：插入记录
// 2.不存在已分配的未满页
// 2.1 存在空闲页：分配新页：插入记录
// 2.2 不存在空闲页：返回FAIL
RC InsertRec (RM_FileHandle *fileHandle,char *pData, RID *rid)
{
	// 判断满页情况
	RC rc;
	PF_PageHandle pfPageHandle;
	if (rc = GetThisPage(&fileHandle->pfFileHandle, 1, &pfPageHandle))
	{
		return rc;
	}
	int insertPageNum = -1;  
	for (int i = 2; i < fileHandle->bitMapLength; i++)
	{
		// 找到第一个已分配且未满的页
		// TODO:这里可以优化一下查找算法
		if (fileHandle->pageBitMap->Test(i) == 1 && fileHandle->recordBitMap->Test(i) == 0)
		{
			insertPageNum = i;
			break;
		}
	}
	
	// 1.存在已分配的未满页：插入记录
	if (insertPageNum !=-1)
	{
		// 找到该页第一个空闲的插槽
		PF_PageHandle tmpPage;
		GetThisPage(&fileHandle->pfFileHandle, insertPageNum, &tmpPage);
		// 初始化一个位图，否则位图中的data没有值
		BitMap bm(fileHandle->recOffset, &tmpPage.pFrame->page.pData[0]);
		int insertPagePos = bm.FirstBit(0, 0);
		assert(insertPagePos != -1);
		memcpy(&(tmpPage.pFrame->page.pData[fileHandle->recOffset])+insertPagePos*sizeof(fileHandle->recSize), pData, fileHandle->recSize);
		// 修改本页位图
		bm.Set(insertPagePos);

		MarkDirty(&tmpPage);
		UnpinPage(&tmpPage);
	}
	// 2.不存在已分配的未满页
	else
	{
		// 判断是否存在空闲页
		if (fileHandle->pageBitMap->hasZero() == true)
		{
			// 2.1 存在空闲页：分配新页：插入记录
			PF_PageHandle newPage;
			if (rc = AllocatePage(&fileHandle->pfFileHandle, &newPage))
			{
				return rc;
			}
			else
			{
				memcpy(&newPage.pFrame->page.pData[fileHandle->recOffset], pData, fileHandle->recSize);
				// 修改本页位图
				BitMap bm(fileHandle->recOffset, &newPage.pFrame->page.pData[0]);
				bm.Set(0);

				MarkDirty(&newPage);
				UnpinPage(&newPage);
			}
		}
		else
		{
			// 2.2 不存在空闲页 
			return FAIL;
		}
	}
	
	return SUCCESS;
}

// 1. 找到记录所在的页面，插槽号
// 2. 修改本页面的位图
// 3. 如果删除后该页变为未满但有记录，则修改记录信息控制页该页面的状态
// 4. 如果删除后该页不再有记录，则修改信息控制页该页面的状态，并将其置为空闲
RC DeleteRec (RM_FileHandle *fileHandle,const RID *rid)
{
	RC rc;
	// 1.
	int pageNum = rid->pageNum;
	int slotNum = rid->slotNum;
	PF_PageHandle pfPageHandle;
	if (rc = GetThisPage(&fileHandle->pfFileHandle, pageNum, &pfPageHandle))
	{
		return rc;
	}
	BitMap bm(fileHandle->recOffset, pfPageHandle.pFrame->page.pData);
	bool isFull = bm.hasZero();
	
	// 2.
	bm.Reset(slotNum);
	assert(isFull);
	// 3.
	fileHandle->recordBitMap->Reset(pageNum);
	PF_PageHandle recPageHandle;
	if (rc = GetThisPage(&fileHandle->pfFileHandle, 1, &recPageHandle))
	{
		return rc;
	}
	BitMap recbm(fileHandle->bitMapLength, &recPageHandle.pFrame->page.pData[0] + sizeof(RM_FileSubHeader));
	recbm.Reset(pageNum);

	int isEmpty = bm.FirstBit(0, 1);
	if (isEmpty == -1)
	{
		// 4.
		DisposePage(&fileHandle->pfFileHandle, pageNum);
	}
	
	MarkDirty(&recPageHandle);
	UnpinPage(&recPageHandle);
	MarkDirty(&pfPageHandle);
	UnpinPage(&pfPageHandle);
	return SUCCESS;
}

RC UpdateRec (RM_FileHandle *fileHandle,const RM_Record *rec)
{
	RC rc;
	int pageNum = rec->rid.pageNum;
	int slotNum = rec->rid.slotNum;
	PF_PageHandle pfPageHandle;
	if (rc = GetThisPage(&fileHandle->pfFileHandle, pageNum, &pfPageHandle))
	{
		return rc;
	}
	memcpy(&pfPageHandle.pFrame->page.pData[fileHandle->recOffset] + rec->rid.slotNum*fileHandle->recSize, rec->pData, fileHandle->recSize);
	MarkDirty(&pfPageHandle);
	UnpinPage(&pfPageHandle);
	return SUCCESS;
}

RC RM_CreateFile (char *fileName, int recordSize)
{
	assert(fileName);
	RC rc;

	// 打开文件，获取页面句柄
	PF_FileHandle pfFileHandle;

	if ((rc = CreateFile(fileName)))
	{
		return rc;
	}
	if ((rc = openFile(fileName, &pfFileHandle)))
	{
		return rc;
	}
	// 页面信息管理页
	
	

	// 根据页面句柄生成记录信息控制页(第1页)
	PF_PageHandle pfPageHandle;

	if ((rc = AllocatePage(&pfFileHandle, &pfPageHandle)))
	{
		return rc;
	}
	assert(pfPageHandle.pFrame->page.pageNum == 1);

	// 从pData[0]开始，存放一个数据结构RM_FileSubHeader记录控制信息，并进行初始化
	RM_FileSubHeader *rmFileSubHeader;

	rmFileSubHeader = (RM_FileSubHeader *)pfPageHandle.pFrame->page.pData;
	rmFileSubHeader->nRecords = 0;
	rmFileSubHeader->recordSize = recordSize;
	// 每个页面从pData[0]开始有一个recordPerPage/8大小的位图，用于管理页面记录插槽使用情况
	// recordPerPage*recordSize+0.125*recordPerPage=PF_PAGE_SIZE
	rmFileSubHeader->recordsPerPage = (PF_PAGE_SIZE) / (recordSize+0.125); 
	// 保证firstRecordOffset为整数，即recordsPerPage向下取8的倍数
	rmFileSubHeader->recordsPerPage = ((int)(rmFileSubHeader->recordsPerPage / 8)) * 8;
	rmFileSubHeader->firstRecordOffset = rmFileSubHeader->recordsPerPage/8;
	if ((rc = MarkDirty(&pfPageHandle)) || (rc = UnpinPage(&pfPageHandle)) || (rc = CloseFile(&pfFileHandle)))
	{
		return rc;
	}

	return SUCCESS;
}
RC RM_OpenFile(char *fileName, RM_FileHandle *fileHandle)
{
	assert(fileHandle);
	RC rc;
	
	// 判断文件打开情况
	if (fileHandle->bOpen)
	{
		return RM_FHOPENNED;
	}
	if ((rc = openFile(fileName, &fileHandle->pfFileHandle)))
	{
		return rc;
	}
	fileHandle->bOpen = true;

	// 对fileHandle进行有关信息的获取
	// 1. 获取记录信息控制页
	PF_PageHandle pfPageHandle;
	if ((rc = GetThisPage(&fileHandle->pfFileHandle, 1, &pfPageHandle)))
	{
		CloseFile(&fileHandle->pfFileHandle);
		return rc;
	}
	fileHandle->recPerPage = ((RM_FileSubHeader*)&pfPageHandle.pFrame->page.pData[0])->recordsPerPage;
	fileHandle->recOffset = ((RM_FileSubHeader*)&pfPageHandle.pFrame->page.pData[0])->firstRecordOffset;
	fileHandle->recSize = ((RM_FileSubHeader*)&pfPageHandle.pFrame->page.pData[0])->recordSize;
	fileHandle->bitMapLength = PF_PAGE_SIZE - sizeof(RM_FileSubHeader);
	// 2. 设置记录信息控制页位图
	fileHandle->recordBitMap = new BitMap(fileHandle->bitMapLength, &pfPageHandle.pFrame->page.pData[sizeof(RM_FileSubHeader)]);

	// 3. 设置页面信息控制页位图
	fileHandle->pageBitMap = new BitMap(fileHandle->bitMapLength, fileHandle->pfFileHandle.pBitmap);

	UnpinPage(&pfPageHandle);

	return SUCCESS;
}

RC RM_CloseFile(RM_FileHandle *fileHandle)
{
	assert(fileHandle);
	RC rc;
	if (fileHandle->pfFileHandle.bopen == false)
	{
		return RM_FHCLOSED;
	}
	if (rc = CloseFile(&fileHandle->pfFileHandle))
	{
		return rc;
	}
	fileHandle->bOpen = false;

	if (fileHandle->pageBitMap)
	{
		delete fileHandle->pageBitMap;
	}
	if (fileHandle->recordBitMap)
	{
		delete fileHandle->recordBitMap;
	}
	return SUCCESS;
}


bool CmpStr(char *left, char *right, CompOp Op)
{
	int cmpResult = strcmp(left, right);
	switch (Op)
	{
	case EQual:
		return (cmpResult == 0) ? true : false;
	case LessT:
		return (cmpResult < 0) ? true : false;
	case GreatT:
		return (cmpResult > 0) ? true : false;
	case NEqual:
		return (cmpResult == 0) ? false : true;
	case LEqual:
		return (cmpResult == 0 || cmpResult < 0) ? true : false;
	case GEqual:
		return (cmpResult == 0 || cmpResult > 0) ? true : false;
	default:
		return false;
	}
}

bool CmpVal(float left, float right, CompOp Op)
{
	switch (Op)
	{
	case EQual:
		return (left == right);
	case LessT:
		return (left < right);
	case GreatT:
		return (left > right);
	case NEqual:
		return (left != right);
	case LEqual:
		return (left <= right);
	case GEqual:
		return (left >= right);
	default:
		return false;
	}
}

