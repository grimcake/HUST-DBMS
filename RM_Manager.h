#ifndef RM_MANAGER_H_H
#define RM_MANAGER_H_H

#include "PF_Manager.h"
#include "str.h"
#include "BitMap.h"

typedef int SlotNum;

typedef struct {	
	PageNum pageNum;	//记录所在页的页号
	SlotNum slotNum;		//记录的插槽号
	bool bValid; 			//true表示为一个有效记录的标识符
}RID;

typedef struct{
	bool bValid;		 // False表示还未被读入记录
	RID  rid; 		 // 记录的标识符 
	char *pData; 		 //记录所存储的数据 
}RM_Record;

typedef struct {
	int nRecords;   // 当前文件中包含的记录数
	int recordSize;  // 每个记录的大小
	int recordsPerPage; // 每个页面可以装载的记录数量
	int firstRecordOffset; //每页第一个记录在数据区中的开始位置
}RM_FileSubHeader;


typedef struct
{
	int bLhsIsAttr,bRhsIsAttr;  //条件的左、右分别是属性（1）还是值（0）
	AttrType attrType;   //该条件中数据的类型
	int LattrLength,RattrLength;  //若是属性的话，表示属性的长度
	int LattrOffset,RattrOffset;  //若是属性的话，表示属性的偏移量
	CompOp compOp;  //比较操作符
	void *Lvalue,*Rvalue;  //若是值的话，指向对应的值
}Con;

typedef struct{//文件句柄
	bool bOpen;//句柄是否打开（是否正在被使用）
	//需要自定义其内部结构
	int recPerPage;  // 与RM_FileSubHeader中的recordsPerPage相等，在RM_OpenFile时赋值
	int bitMapLength;  // 页面位图和记录位图的字节大小
	int recOffset; // 等于recPerPage/8
	int recSize; // 一条记录的大小
	BitMap *pageBitMap;   // 页面位图，位于第0页
	BitMap *recordBitMap;   // 记录位图， 位于第1页
	PF_FileHandle pfFileHandle;
}RM_FileHandle;

typedef struct{
	bool  bOpen;		//扫描是否打开 
	RM_FileHandle  *pRMFileHandle;		//扫描的记录文件句柄
	int  conNum;		//扫描涉及的条件数量 
	Con  *conditions;	//扫描涉及的条件数组指针
    PF_PageHandle  PageHandle; //处理中的页面句柄
	PageNum  pn; 	//扫描即将处理的页面号
	SlotNum  sn;		//扫描即将处理的插槽号
}RM_FileScan;


// 获取一个符合扫描条件的记录。如果该方法成功，
// 返回值rec应包含记录副本及记录标识符。
// 如果没有发现满足扫描条件的记录，则返回RM_EOF。
RC GetNextRec(RM_FileScan *rmFileScan,RM_Record *rec);

// 打开一个文件扫描。
// 本函数利用从第二个参数开始的所有输入参数初始化一个由参数rmFileScan指向的文件扫描结构，
// 在使用中，用户应先调用此函数初始化文件扫描结构，
// 然后再调用GetNextRec函数来逐个返回文件中满足条件的记录。
// 如果条件数量conNum为0，则意味着检索文件中的所有记录。
// 如果条件不为空，则要对每条记录进行条件比较，只有满足所有条件的记录才被返回。
RC OpenScan(RM_FileScan *rmFileScan,RM_FileHandle *fileHandle,int conNum,Con *conditions);

// 关闭一个文件扫描，释放相应的资源。
RC CloseScan(RM_FileScan *rmFileScan);

// 更新指定文件中的记录，
// rec指向的记录结构中的rid字段为要更新的记录的标识符，
// pData字段指向新的记录内容。
RC UpdateRec (RM_FileHandle *fileHandle,const RM_Record *rec);

// 从指定文件中删除标识符为rid的记录。
RC DeleteRec (RM_FileHandle *fileHandle,const RID *rid);

// 插入一个新的记录到指定文件中，pData为指向新纪录内容的指针，
// 返回该记录的标识符rid。
RC InsertRec (RM_FileHandle *fileHandle, char *pData, RID *rid); 

// 获取指定文件中标识符为rid的记录内容到rec指向的记录结构中。
RC GetRec (RM_FileHandle *fileHandle, RID *rid, RM_Record *rec); 

// 关闭给定句柄对应的记录文件。
RC RM_CloseFile (RM_FileHandle *fileHandle);

// 根据文件名打开指定的记录文件，返回其文件句柄指针。
RC RM_OpenFile (char *fileName, RM_FileHandle *fileHandle);

// 创建一个名为fileName的记录文件，
// 该文件中每条记录的大小为recordSize
RC RM_CreateFile (char *fileName, int recordSize);

bool CmpVal(float left, float right, CompOp Op);
bool CmpStr(char *left, char *right, CompOp Op);



#endif