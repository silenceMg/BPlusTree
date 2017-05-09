#pragma once
/*
运行前需在程序目录建立名为Bfile的文件，否则崩溃
*/

#include<iostream>  
#include<time.h>  
//#include<cstdio> 
using namespace std;


#define MAX_KEY 5   //B+树的阶,必须为大于3奇数  

typedef  uint32_t KEYTYPE;
typedef  uint32_t  FILEP;

//B+树节点的数据结构  
typedef struct
{
	KEYTYPE   key[MAX_KEY];      //关键字域  
	FILEP Pointer[MAX_KEY + 1];    //指针域  
	int       nkey;              //关键字数  
	bool  isleaf;                //是否为叶节点 叶节点:true 否则为false  

}BPlusNode;


//插入关键字的数据结构  
typedef struct
{
	KEYTYPE   key;              //该记录的关键字  
	FILEP Raddress;         //该关键字对应记录的地址  

}TRecord;


//保存查询结果的数据结构  
typedef struct
{
	bool  exist;
	FILEP Baddress;   //保存包含该记录的B+树节点地址  
	FILEP Raddress;   //该关键字的所指向的记录地址  

}SearchResult;



class BPlusTree
{
	FILEP ROOT;       //树根在文件内的偏移地址  
	FILE  *Bfile;     //B+树文件的指针  
	FILE  *Rfile;     //记录文件的指针  

public:

	FILEP GetBPlusNode() const;
	void ReadBPlusNode(const FILEP, BPlusNode&) const;
	void WriteBPlusNode(const FILEP, const BPlusNode&);

	void Build_BPlus_Tree();

	void Insert_BPlus_Tree(TRecord&);
	void insert_bplus_tree(FILEP, TRecord&);

	void Split_BPlus_Node(BPlusNode&, BPlusNode&, const int);

	void Search_BPlus_Tree(TRecord&, SearchResult&) const;


	void Delete_BPlus_Tree(TRecord&);
	void delete_BPlus_tree(FILEP, TRecord&);

	void EnumLeafKey();

	BPlusTree();
	~BPlusTree();

};
