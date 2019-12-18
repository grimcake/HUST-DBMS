#include "stdafx.h"
#include "RM_Manager.h"
#include "PF_Manager.h"
#include "BitMap.h"
#include "str.h"

#include <assert.h>
#include <iostream>


RC OpenScan(RM_FileScan *rmFileScan,RM_FileHandle *fileHandle,int conNum,Con *conditions)//��ʼ��ɨ��
{
	RC rc;
	if (rmFileScan->bOpen == true)
	{
		return RM_FSOPEN;
	}
	// ����ļ�û������
	if (fileHandle->pfFileHandle.pFileSubHeader->nAllocatedPages <= 2)
	{
		rmFileScan->pn = rmFileScan->sn = 0;
		return SUCCESS;
	}
	// ��ʼ��
	rmFileScan->bOpen = true;
	rmFileScan->pRMFileHandle = fileHandle;
	rmFileScan->conNum = conNum;
	rmFileScan->conditions = conditions;
	// ����ɨ�����ҳ��
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
	// �ļ�������
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
		// Ѱ����һ�������ݵĲ��
		BitMap bm(rmFileScan->pRMFileHandle->recOffset, rmFileScan->PageHandle.pFrame->page.pData);
		while (bm.FirstBit(rmFileScan->sn, 1) != -1)
		{ 
			Con* condition;
			bool correct = true;
			// ������¼���ڵ�λ��
			int recOffset = rmFileScan->pRMFileHandle->recOffset + rmFileScan->sn*rmFileScan->pRMFileHandle->recSize;
			char* recAddr = rmFileScan->PageHandle.pFrame->page.pData + recOffset;

			int leftVal, rightVal;
			int leftF, rightF;
			char *leftStr, *rightStr;

			// �����ж�ÿһ������
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
					//�Ƚ������ַ���
					correct = CmpStr(leftStr, rightStr, condition->compOp);
					break;
				}
				if (!correct)
				{
					break;
				}
			}
			// �������еıȽ�����
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
			// ֮��û�з���ʹ�õ�ҳ��
			return RM_NOMORERECINMEM;
		}
		// ����һ�������ݵ�ҳ
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
	// ��ȡrid����ҳ
	PF_PageHandle pfPageHandle;
	RC rc;
	if (rc = GetThisPage(&fileHandle->pfFileHandle, rid->pageNum, &pfPageHandle))
	{
		return rc;
	}
	// �ڸ�ҳλͼ���ж�һ��rid����ļ�¼�Ƿ���Ч
	BitMap bm(fileHandle->recOffset, &pfPageHandle.pFrame->page.pData[0]);
	if (bm.Test(rid->slotNum) == 0)
	{
		return RM_INVALIDRID;
	}

	// ��ȡrid���ڲ�۵ĵ�ַ
	rec->rid.bValid = true;
	rec->rid.pageNum = rid->pageNum;
	rec->rid.slotNum = rid->slotNum;
	// ���ֻ��pData����rid
	rec->pData = &pfPageHandle.pFrame->page.pData[fileHandle->recOffset+rec->rid.slotNum*fileHandle->recSize];
	


	UnpinPage(&pfPageHandle);
	return SUCCESS;
}

// ��Ϊ���������
// 1.�����ѷ����δ��ҳ�������¼
// 2.�������ѷ����δ��ҳ
// 2.1 ���ڿ���ҳ��������ҳ�������¼
// 2.2 �����ڿ���ҳ������FAIL
RC InsertRec (RM_FileHandle *fileHandle,char *pData, RID *rid)
{
	// �ж���ҳ���
	RC rc;
	PF_PageHandle pfPageHandle;
	if (rc = GetThisPage(&fileHandle->pfFileHandle, 1, &pfPageHandle))
	{
		return rc;
	}
	int insertPageNum = -1;  
	for (int i = 2; i < fileHandle->bitMapLength; i++)
	{
		// �ҵ���һ���ѷ�����δ����ҳ
		// TODO:��������Ż�һ�²����㷨
		if (fileHandle->pageBitMap->Test(i) == 1 && fileHandle->recordBitMap->Test(i) == 0)
		{
			insertPageNum = i;
			break;
		}
	}
	
	// 1.�����ѷ����δ��ҳ�������¼
	if (insertPageNum !=-1)
	{
		// �ҵ���ҳ��һ�����еĲ��
		PF_PageHandle tmpPage;
		GetThisPage(&fileHandle->pfFileHandle, insertPageNum, &tmpPage);
		// ��ʼ��һ��λͼ������λͼ�е�dataû��ֵ
		BitMap bm(fileHandle->recOffset, &tmpPage.pFrame->page.pData[0]);
		int insertPagePos = bm.FirstBit(0, 0);
		assert(insertPagePos != -1);
		memcpy(&(tmpPage.pFrame->page.pData[fileHandle->recOffset])+insertPagePos*sizeof(fileHandle->recSize), pData, fileHandle->recSize);
		// �޸ı�ҳλͼ
		bm.Set(insertPagePos);

		MarkDirty(&tmpPage);
		UnpinPage(&tmpPage);
	}
	// 2.�������ѷ����δ��ҳ
	else
	{
		// �ж��Ƿ���ڿ���ҳ
		if (fileHandle->pageBitMap->hasZero() == true)
		{
			// 2.1 ���ڿ���ҳ��������ҳ�������¼
			PF_PageHandle newPage;
			if (rc = AllocatePage(&fileHandle->pfFileHandle, &newPage))
			{
				return rc;
			}
			else
			{
				memcpy(&newPage.pFrame->page.pData[fileHandle->recOffset], pData, fileHandle->recSize);
				// �޸ı�ҳλͼ
				BitMap bm(fileHandle->recOffset, &newPage.pFrame->page.pData[0]);
				bm.Set(0);

				MarkDirty(&newPage);
				UnpinPage(&newPage);
			}
		}
		else
		{
			// 2.2 �����ڿ���ҳ 
			return FAIL;
		}
	}
	
	return SUCCESS;
}

// 1. �ҵ���¼���ڵ�ҳ�棬��ۺ�
// 2. �޸ı�ҳ���λͼ
// 3. ���ɾ�����ҳ��Ϊδ�����м�¼�����޸ļ�¼��Ϣ����ҳ��ҳ���״̬
// 4. ���ɾ�����ҳ�����м�¼�����޸���Ϣ����ҳ��ҳ���״̬����������Ϊ����
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

	// ���ļ�����ȡҳ����
	PF_FileHandle pfFileHandle;

	if ((rc = CreateFile(fileName)))
	{
		return rc;
	}
	if ((rc = openFile(fileName, &pfFileHandle)))
	{
		return rc;
	}
	// ҳ����Ϣ����ҳ
	
	

	// ����ҳ�������ɼ�¼��Ϣ����ҳ(��1ҳ)
	PF_PageHandle pfPageHandle;

	if ((rc = AllocatePage(&pfFileHandle, &pfPageHandle)))
	{
		return rc;
	}
	assert(pfPageHandle.pFrame->page.pageNum == 1);

	// ��pData[0]��ʼ�����һ�����ݽṹRM_FileSubHeader��¼������Ϣ�������г�ʼ��
	RM_FileSubHeader *rmFileSubHeader;

	rmFileSubHeader = (RM_FileSubHeader *)pfPageHandle.pFrame->page.pData;
	rmFileSubHeader->nRecords = 0;
	rmFileSubHeader->recordSize = recordSize;
	// ÿ��ҳ���pData[0]��ʼ��һ��recordPerPage/8��С��λͼ�����ڹ���ҳ���¼���ʹ�����
	// recordPerPage*recordSize+0.125*recordPerPage=PF_PAGE_SIZE
	rmFileSubHeader->recordsPerPage = (PF_PAGE_SIZE) / (recordSize+0.125); 
	// ��֤firstRecordOffsetΪ��������recordsPerPage����ȡ8�ı���
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
	
	// �ж��ļ������
	if (fileHandle->bOpen)
	{
		return RM_FHOPENNED;
	}
	if ((rc = openFile(fileName, &fileHandle->pfFileHandle)))
	{
		return rc;
	}
	fileHandle->bOpen = true;

	// ��fileHandle�����й���Ϣ�Ļ�ȡ
	// 1. ��ȡ��¼��Ϣ����ҳ
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
	// 2. ���ü�¼��Ϣ����ҳλͼ
	fileHandle->recordBitMap = new BitMap(fileHandle->bitMapLength, &pfPageHandle.pFrame->page.pData[sizeof(RM_FileSubHeader)]);

	// 3. ����ҳ����Ϣ����ҳλͼ
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

