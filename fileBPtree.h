#pragma once
/*
����ǰ���ڳ���Ŀ¼������ΪBfile���ļ����������
*/

#include<iostream>  
#include<time.h>  
//#include<cstdio> 
using namespace std;


#define MAX_KEY 5   //B+���Ľ�,����Ϊ����3����  

typedef  uint32_t KEYTYPE;
typedef  uint32_t  FILEP;

//B+���ڵ�����ݽṹ  
typedef struct
{
	KEYTYPE   key[MAX_KEY];      //�ؼ�����  
	FILEP Pointer[MAX_KEY + 1];    //ָ����  
	int       nkey;              //�ؼ�����  
	bool  isleaf;                //�Ƿ�ΪҶ�ڵ� Ҷ�ڵ�:true ����Ϊfalse  

}BPlusNode;


//����ؼ��ֵ����ݽṹ  
typedef struct
{
	KEYTYPE   key;              //�ü�¼�Ĺؼ���  
	FILEP Raddress;         //�ùؼ��ֶ�Ӧ��¼�ĵ�ַ  

}TRecord;


//�����ѯ��������ݽṹ  
typedef struct
{
	bool  exist;
	FILEP Baddress;   //��������ü�¼��B+���ڵ��ַ  
	FILEP Raddress;   //�ùؼ��ֵ���ָ��ļ�¼��ַ  

}SearchResult;



class BPlusTree
{
	FILEP ROOT;       //�������ļ��ڵ�ƫ�Ƶ�ַ  
	FILE  *Bfile;     //B+���ļ���ָ��  
	FILE  *Rfile;     //��¼�ļ���ָ��  

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
	void borrow(BPlusNode& cur, BPlusNode& curChild, BPlusNode& adjChild, int i, FILEP current);
	void merge(BPlusNode& cur, BPlusNode& curChild, BPlusNode& adjChild, int i, FILEP current);

	BPlusTree();
	~BPlusTree();

};