#ifndef __QUERY_MANAGER_H_
#define __QUERY_MANAGER_H_
#include "str.h"

typedef struct SelResult{
	int col_num;
	int row_num;
	AttrType type[20];	//��������ֶε���������
	int length[20];		//��������ֶ�ֵ�ĳ���
	char fields[20][20];//����ʮ���ֶ���������ÿ���ֶεĳ��Ȳ�����20
	char ** res[100];	//���һ������¼
	SelResult * next_res;
}SelResult;

// ϵͳ���ļ��м�¼�ṹ
typedef struct Table {
	char tablename[21]; // ����
	int attrcount;      // �������Ե�����
};

// ϵͳ���ļ��м�¼�ṹ
typedef struct Column {
	char tablename[21]; // ����
	char attrname[21];  // ������
	int attrtype;       // ��������
	int attrlength;     // ���Գ���
	int attroffset;     // �����ڼ�¼�е�ƫ����
	bool ix_flag;       // �����������Ƿ���������ı�ʶ��trueΪ���ڣ�falseΪ������
	char indexname[21]; // ��������
};

void Init_Result(SelResult * res);
void Destory_Result(SelResult * res);

RC Query(char * sql,SelResult * res);

RC Select(int nSelAttrs,RelAttr **selAttrs,int nRelations,char **relations,int nConditions,Condition *conditions,SelResult * res);

RC SingleSelect(int nSelAttrs, RelAttr **selAttrs, int nRelations, char **relations, int nConditions, Condition *conditions, SelResult * res);
#endif