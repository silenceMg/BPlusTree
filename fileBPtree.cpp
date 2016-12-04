#include"fileBPtree.h"

BPlusTree::BPlusTree()
{
	Bfile = fopen("Bfile", "rb+");     //��B+���ļ�  

}

BPlusTree :: ~BPlusTree()
{
	fclose(Bfile);
}



void BPlusTree::Build_BPlus_Tree()   //����һ�ÿ�B+��  
{
	ROOT = GetBPlusNode();
	BPlusNode r;
	r.Pointer[MAX_KEY] = 0;
	r.nkey = 0;
	r.isleaf = true;
	WriteBPlusNode(ROOT, r);
}



void    BPlusTree::Insert_BPlus_Tree(TRecord &record)        //��B+������ؼ���  
{
	BPlusNode r;//root
	ReadBPlusNode(ROOT, r);

	if (r.nkey == MAX_KEY)//if root is full
	{
		BPlusNode newroot;
		newroot.nkey = 0;
		newroot.isleaf = false;
		newroot.Pointer[0] = ROOT;//point to root

		Split_BPlus_Node(newroot, r, 0);//creat a new root node and split the old root node.
		WriteBPlusNode(ROOT, r);//write splited old root node back to where it were.

		ROOT = GetBPlusNode();//allocate new space

		WriteBPlusNode(ROOT, newroot);//write new root to a new place.

		//���Ѹ��ڵ�
	}
	insert_bplus_tree(ROOT, record);//if root is not full, insert node directly.
}



void BPlusTree::insert_bplus_tree(FILEP current, TRecord &record)
{
	BPlusNode x;
	ReadBPlusNode(current, x);

	int   i;//position of given key's lower_bound
	for (i = 0; i < x.nkey && x.key[i] < record.key; i++);
	if (i < x.nkey && x.isleaf && x.key[i] == record.key)  //��B+��Ҷ�ڵ��ҵ�����ͬ�ؼ���  
	{
		//�ؼ��ֲ����ظ�  
		return;
	}

	if (!x.isleaf)//�������Ҷ�ڵ�  
	{
		BPlusNode y;
		ReadBPlusNode(x.Pointer[i], y);//find child of x, if y is leaf node, we will handle it in Split_BPlus_Node().

		if (y.nkey == MAX_KEY)     //���x���ӽڵ�������������ӽڵ����  
		{
			Split_BPlus_Node(x, y, i);
			WriteBPlusNode(current, x);//update father node
			WriteBPlusNode(x.Pointer[i], y);
		}
		if (record.key <= x.key[i] || i == x.nkey)
		{
			insert_bplus_tree(x.Pointer[i], record);//insert into new left node.
		}
		else
		{
			insert_bplus_tree(x.Pointer[i + 1], record);//insert into new node on current node's right.
		}

	}
	else          //�����Ҷ�ڵ�,��ֱ�ӽ��ؼ��ֲ���key������  
	{
		//make room for new node.
		for (int j = x.nkey; j > i; j--)
		{
			x.key[j] = x.key[j - 1];
			x.Pointer[j] = x.Pointer[j - 1];
		}
		x.key[i] = record.key;
		x.nkey++;

		//����¼�ĵ�ַ����x.Pointer[i]  

		x.Pointer[i] = record.Raddress;

		WriteBPlusNode(current, x);

	}

}



void    BPlusTree::Split_BPlus_Node(BPlusNode &father, BPlusNode &current, const int childnum)            //��������B+���ڵ�  
{
	int half = MAX_KEY / 2;

	int i;

	for (i = father.nkey; i > childnum; i--)//move key and pointer right to make room for new node
	{
		father.key[i] = father.key[i - 1];
		father.Pointer[i + 1] = father.Pointer[i];
	}
	father.nkey++;//increase key nums

	BPlusNode t;

	FILEP address = GetBPlusNode();//require new space from native file system, in sqlite ,this space is a page

	father.key[childnum] = current.key[half];
	father.Pointer[childnum + 1] = address;//split

	for (i = half + 1; i < MAX_KEY; i++)//copy to new node
	{
		t.key[i - half - 1] = current.key[i];
		t.Pointer[i - half - 1] = current.Pointer[i];
	}

	t.nkey = MAX_KEY - half - 1;
	t.Pointer[t.nkey] = current.Pointer[MAX_KEY];//point to right leaf node?

	t.isleaf = current.isleaf;

	current.nkey = half;

	if (current.isleaf)   //�����ǰ�����ѽڵ���Ҷ��  
	{
		//save the key which has been pushed up
		current.nkey++;
		t.Pointer[MAX_KEY] = current.Pointer[MAX_KEY];
		current.Pointer[MAX_KEY] = address;
	}

	WriteBPlusNode(address, t);//write down new node.

}



void BPlusTree::Search_BPlus_Tree(TRecord &record, SearchResult &result) const        //��B+����ѯһ���ؼ���  
{
	int i;

	BPlusNode a;
	FILEP current = ROOT;

	do
	{
		ReadBPlusNode(current, a);

		for (i = 0; i < a.nkey && record.key > a.key[i]; i++);

		if (i < a.nkey && a.isleaf && record.key == a.key[i])       //��B+��Ҷ�ڵ��ҵ��˵�ֵ�Ĺؼ���  
		{
			result.Baddress = current;
			result.Raddress = a.Pointer[i];                       //���ظùؼ�������Ӧ�ļ�¼�ĵ�ַ  
			result.exist = true;

			return;
		}
		current = a.Pointer[i];

	} while (!a.isleaf);

	result.exist = false;
}


void BPlusTree::borrow(BPlusNode& cur, BPlusNode& curChild, BPlusNode& adjChild, int i, FILEP current)
{
	for (int j = curChild.nkey; j > 0; j--)
	{
		curChild.key[j] = curChild.key[j - 1];
		curChild.Pointer[j] = curChild.Pointer[j - 1];
	}

	curChild.key[0] = cur.key[i - 1];
	curChild.Pointer[0] = adjChild.Pointer[adjChild.nkey - 1];

	curChild.nkey++;

	adjChild.nkey--;

	cur.key[i - 1] = adjChild.key[adjChild.nkey - 1];
	cur.key[i] = curChild.key[curChild.nkey - 2];

	WriteBPlusNode(current, cur);
	WriteBPlusNode(cur.Pointer[i - 1], adjChild);
	WriteBPlusNode(cur.Pointer[i], curChild);
}

void BPlusTree::merge(BPlusNode& cur, BPlusNode& curChild, BPlusNode& adjChild, int i, FILEP current)
{
	int j = 0;
	for (; j < curChild.nkey; j++)
	{
		adjChild.key[adjChild.nkey + j] = curChild.key[j];
		adjChild.Pointer[adjChild.nkey + j] = curChild.Pointer[j];
	}
	adjChild.nkey += curChild.nkey;

	adjChild.Pointer[MAX_KEY] = curChild.Pointer[MAX_KEY];

	//�ͷ�child�ڵ�ռ�õĿռ�x.Pointer[i]  

	for (j = i - 1; j < cur.nkey - 1; j++)
	{
		cur.key[j] = cur.key[j + 1];
		cur.Pointer[j + 1] = cur.Pointer[j + 2];
	}
	cur.nkey--;

	cur.key[i - 1] = adjChild.key[adjChild.nkey - 2];

	WriteBPlusNode(current, cur);
	WriteBPlusNode(cur.Pointer[i - 1], adjChild);

	i--;
}

void BPlusTree::delete_BPlus_tree(FILEP current, TRecord &record)
{
	int i, j;

	BPlusNode x;
	ReadBPlusNode(current, x);


	for (i = 0; i < x.nkey && record.key > x.key[i]; i++);

	if (i < x.nkey && x.key[i] == record.key)  //�ڵ�ǰ�ڵ��ҵ��ؼ���  
	{

		if (!x.isleaf)     //���ڽڵ��ҵ��ؼ���  
		{
			BPlusNode child;
			ReadBPlusNode(x.Pointer[i], child);

			if (child.isleaf)     //���������Ҷ�ڵ�  
			{
				if (child.nkey > MAX_KEY / 2)      //���A  
				{
					x.key[i] = child.key[child.nkey - 2];
					child.nkey--;

					WriteBPlusNode(current, x);
					WriteBPlusNode(x.Pointer[i], child);

					return;
				}
				else    //�����ӽڵ�Ĺؼ�������������  
				{
					if (i > 0)      //�����ֵܽڵ�  
					{
						BPlusNode lbchild;
						ReadBPlusNode(x.Pointer[i - 1], lbchild);

						if (lbchild.nkey > MAX_KEY / 2)        //���B  
						{
							borrow(x, child, lbchild,i,current);
						}
						else    //���C  
						{
							merge(x, child, lbchild, i, current);
						}
					}
					else      //ֻ�����ֵܽڵ�  
					{
						BPlusNode rbchild;
						ReadBPlusNode(x.Pointer[i + 1], rbchild);

						if (rbchild.nkey > MAX_KEY / 2)        //���D  
						{
							borrow(x, child, rbchild, i, current);
						}
						else    //���E  
						{
							merge(x, child, rbchild, i, current);
						}
					}
				}
			}
			else      //���F  
			{

				//�ҵ�key��B+��Ҷ�ڵ�����ֵܹؼ���,������ؼ���ȡ��key��λ��  

				TRecord trecord;
				trecord.key = record.key;
				SearchResult result;
				Search_BPlus_Tree(trecord, result);

				BPlusNode last;

				ReadBPlusNode(result.Baddress, last);

				x.key[i] = last.key[last.nkey - 2];

				WriteBPlusNode(current, x);


				if (child.nkey > MAX_KEY / 2)        //���H  
				{
					//���ӽڵ�Ĺؼ��������ﵽ��������������
				}
				else          //�����ӽڵ�Ĺؼ�������������,���ֵܽڵ��ĳһ���ؼ�����������  
				{
					if (i > 0)  //x.key[i]�����ֵ�  
					{
						BPlusNode lbchild;
						ReadBPlusNode(x.Pointer[i - 1], lbchild);

						if (lbchild.nkey > MAX_KEY / 2)       //���I  
						{
							borrow(x, child, lbchild, i, current);
						}
						else        //���J  
						{
							merge(x, child, lbchild, i, current);
						}

					}
					else        //����x.key[i]ֻ�����ֵ�  
					{
						BPlusNode rbchild;
						ReadBPlusNode(x.Pointer[i + 1], rbchild);

						if (rbchild.nkey > MAX_KEY / 2)     //���K  
						{

							borrow(x, child, rbchild, i, current);

						}
						else        //���L  
						{
							merge(x, child, rbchild, i, current);

						}
					}
				}
			}

			delete_BPlus_tree(x.Pointer[i], record);

		}
		else  //���G  
		{
			for (j = i; j < x.nkey - 1; j++)
			{
				x.key[j] = x.key[j + 1];
				x.Pointer[j] = x.Pointer[j + 1];
			}
			x.nkey--;

			WriteBPlusNode(current, x);

			return;
		}

	}
	else        //�ڵ�ǰ�ڵ�û�ҵ��ؼ���     
	{
		if (!x.isleaf)    //û�ҵ��ؼ���,��ؼ��ֱ�Ȼ��������Pointer[i]Ϊ����������  
		{
			BPlusNode child;
			ReadBPlusNode(x.Pointer[i], child);

			if (!child.isleaf)      //����亢�ӽڵ����ڽڵ�  
			{
				if (child.nkey > MAX_KEY / 2)        //���H  
				{

				}
				else          //�����ӽڵ�Ĺؼ�������������,���ֵܽڵ��ĳһ���ؼ�����������  
				{
					if (i > 0)  //x.key[i]�����ֵ�  
					{
						BPlusNode lbchild;
						ReadBPlusNode(x.Pointer[i - 1], lbchild);

						if (lbchild.nkey > MAX_KEY / 2)       //���I  
						{
							borrow(x, child, lbchild, i, current);
						}
						else        //���J  
						{
							merge(x, child, lbchild, i, current);

						}

					}
					else        //����x.key[i]ֻ�����ֵ�  
					{
						BPlusNode rbchild;
						ReadBPlusNode(x.Pointer[i + 1], rbchild);

						if (rbchild.nkey > MAX_KEY / 2)     //���K  
						{

							borrow(x, child, rbchild, i, current);

						}
						else        //���L  
						{
							merge(x, child, rbchild, i, current);

						}

					}
				}
			}
			else  //�����亢�ӽڵ�����ڵ�  
			{
				if (child.nkey > MAX_KEY / 2)  //���M  
				{

				}
				else        //�����ӽڵ㲻������  
				{
					if (i > 0) //�����ֵ�  
					{
						BPlusNode lbchild;
						ReadBPlusNode(x.Pointer[i - 1], lbchild);

						if (lbchild.nkey > MAX_KEY / 2)       //���N  
						{
							borrow(x, child, lbchild, i, current);

						}
						else        //���O  
						{

							merge(x, child, lbchild, i, current);

						}

					}
					else        //����ֻ�����ֵ�  
					{
						BPlusNode rbchild;
						ReadBPlusNode(x.Pointer[i + 1], rbchild);

						if (rbchild.nkey > MAX_KEY / 2)       //���P  
						{
							borrow(x, child, rbchild, i, current);

						}
						else        //���Q  
						{
							merge(x, child, rbchild, i, current);
						}
					}
				}
			}
			delete_BPlus_tree(x.Pointer[i], record);
		}
	}
}



void BPlusTree::Delete_BPlus_Tree(TRecord &record)    //��B+��ɾ��һ���ؼ���  
{
	delete_BPlus_tree(ROOT, record);

	BPlusNode rootnode;
	ReadBPlusNode(ROOT, rootnode);

	if (!rootnode.isleaf && rootnode.nkey == 0)    //���ɾ���ؼ��ֺ���ڵ㲻��Ҷ�ڵ㣬���ҹؼ�������Ϊ0ʱ���ڵ�ҲӦ�ñ�ɾ��  
	{
		//�ͷ�ROOT�ڵ�ռ�õĿռ�  
		ROOT = rootnode.Pointer[0];         //���ڵ�����,B+���߶ȼ�1  

	}

}




void BPlusTree::EnumLeafKey()    //����ö��B+��Ҷ�ڵ�����йؼ���  
{
	BPlusNode head;

	ReadBPlusNode(ROOT, head);

	while (!head.isleaf)
	{
		ReadBPlusNode(head.Pointer[0], head);//find most left leaf node.
	}

	while (1)
	{
		for (int i = 0; i < head.nkey; i++)
			printf("%d\n", head.key[i]);

		if (head.Pointer[MAX_KEY] == 0)
			break;

		ReadBPlusNode(head.Pointer[MAX_KEY], head);
	}

}




inline FILEP BPlusTree::GetBPlusNode()  const //�ڴ����Ϸ���һ��B+���ڵ�ռ�,add to file's tail
{
	fseek(Bfile, 0, SEEK_END);

	return  ftell(Bfile);
}

inline void BPlusTree::ReadBPlusNode(const FILEP address, BPlusNode   &r) const //��ȡaddress��ַ�ϵ�һ��B+���ڵ�  
{
	fseek(Bfile, address, SEEK_SET);
	fread((char*)(&r), sizeof(BPlusNode), 1, Bfile);

}


inline void BPlusTree::WriteBPlusNode(const FILEP address, const BPlusNode &r) //��һ��B+���ڵ�д��address��ַ  
{
	fseek(Bfile, address, SEEK_SET);
	fwrite((char*)(&r), sizeof(BPlusNode), 1, Bfile);
}



int main()
{
	BPlusTree tree;

	tree.Build_BPlus_Tree();      //����  

	TRecord record;   SearchResult result;

	int time1 = clock();

	int i;
	for (i = 0; i < 4; i++)
	{
		record.key = i;

		tree.Insert_BPlus_Tree(record);
		//  printf("%d\n",i );  
	}

	for (i = 3; i > 0; i--)
	{
		record.key = i;
		tree.Delete_BPlus_Tree(record);
		tree.Search_BPlus_Tree(record, result);
		if (result.exist)
		{
			break;
			printf("%d\n", i);
		}
 
	}

	cout << clock() - time1 << endl;
	system("pause");
	tree.EnumLeafKey();

	tree.~BPlusTree();
	system("pause");
	return 0;
}