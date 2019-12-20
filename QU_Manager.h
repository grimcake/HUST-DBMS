#ifndef __QUERY_MANAGER_H_
#define __QUERY_MANAGER_H_
#include "str.h"

typedef struct SelResult{
	int col_num;
	int row_num;
	AttrType type[20];	//结果集各字段的数据类型
	int length[20];		//结果集各字段值的长度
	char fields[20][20];//最多二十个字段名，而且每个字段的长度不超过20
	char ** res[100];	//最多一百条记录
	SelResult * next_res;
}SelResult;

// 系统表文件中记录结构
typedef struct Table {
	char tablename[21]; // 表名
	int attrcount;      // 表中属性的数量
};

// 系统列文件中记录结构
typedef struct Column {
	char tablename[21]; // 表名
	char attrname[21];  // 属性名
	int attrtype;       // 属性类型
	int attrlength;     // 属性长度
	int attroffset;     // 属性在记录中的偏移量
	bool ix_flag;       // 该属性列上是否存在索引的标识，true为存在，false为不存在
	char indexname[21]; // 索引名称
};

void Init_Result(SelResult * res);
void Destory_Result(SelResult * res);

RC Query(char * sql,SelResult * res);

RC Select(int nSelAttrs,RelAttr **selAttrs,int nRelations,char **relations,int nConditions,Condition *conditions,SelResult * res);

RC SingleSelect(int nSelAttrs, RelAttr **selAttrs, int nRelations, char **relations, int nConditions, Condition *conditions, SelResult * res);
#endif