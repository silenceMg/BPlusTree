#include"fileBPtree.h"

BPlusTree::BPlusTree()
{
	Bfile = fopen("Bfile", "rb+");     //打开B+树文件  

}

BPlusTree :: ~BPlusTree()
{
	fclose(Bfile);
}



void BPlusTree::Build_BPlus_Tree()   //建立一棵空B+树  
{
	ROOT = GetBPlusNode();
	BPlusNode r;
	r.Pointer[MAX_KEY] = 0;
	r.nkey = 0;
	r.isleaf = true;
	WriteBPlusNode(ROOT, r);
}



void    BPlusTree::Insert_BPlus_Tree(TRecord &record)        //向B+树插入关键字  
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

		//分裂根节点
	}
	insert_bplus_tree(ROOT, record);//if root is not full, insert node directly.
}



void BPlusTree::insert_bplus_tree(FILEP current, TRecord &record)
{
	BPlusNode x;
	ReadBPlusNode(current, x);

	int   i;//position of given key's lower_bound
	for (i = 0; i < x.nkey && x.key[i] < record.key; i++);
	if (i < x.nkey && x.isleaf && x.key[i] == record.key)  //在B+树叶节点找到了相同关键字  
	{
		//关键字插入重复  
		return;
	}

	if (!x.isleaf)//如果不是叶节点  
	{
		BPlusNode y;
		ReadBPlusNode(x.Pointer[i], y);//find child of x, if y is leaf node, we will handle it in Split_BPlus_Node().

		if (y.nkey == MAX_KEY)     //如果x的子节点已满，则这个子节点分裂  
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
	else          //如果是叶节点,则直接将关键字插入key数组中  
	{
		//make room for new node.
		for (int j = x.nkey; j > i; j--)
		{
			x.key[j] = x.key[j - 1];
			x.Pointer[j] = x.Pointer[j - 1];
		}
		x.key[i] = record.key;
		x.nkey++;

		//将记录的地址赋给x.Pointer[i]  

		x.Pointer[i] = record.Raddress;

		WriteBPlusNode(current, x);

	}

}



void    BPlusTree::Split_BPlus_Node(BPlusNode &father, BPlusNode &current, const int childnum)            //分裂满的B+树节点  
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

	if (current.isleaf)   //如果当前被分裂节点是叶子  
	{
		//save the key which has been pushed up
		current.nkey++;
		t.Pointer[MAX_KEY] = current.Pointer[MAX_KEY];
		current.Pointer[MAX_KEY] = address;
	}

	WriteBPlusNode(address, t);//write down new node.

}



void BPlusTree::Search_BPlus_Tree(TRecord &record, SearchResult &result) const        //在B+树查询一个关键字  
{
	int i;

	BPlusNode a;
	FILEP current = ROOT;

	do
	{
		ReadBPlusNode(current, a);

		for (i = 0; i < a.nkey && record.key > a.key[i]; i++);

		if (i < a.nkey && a.isleaf && record.key == a.key[i])       //在B+树叶节点找到了等值的关键字  
		{
			result.Baddress = current;
			result.Raddress = a.Pointer[i];                       //返回该关键字所对应的记录的地址  
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

	//释放child节点占用的空间x.Pointer[i]  

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

	if (i < x.nkey && x.key[i] == record.key)  //在当前节点找到关键字  
	{

		if (!x.isleaf)     //在内节点找到关键字  
		{
			BPlusNode child;
			ReadBPlusNode(x.Pointer[i], child);

			if (child.isleaf)     //如果孩子是叶节点  
			{
				if (child.nkey > MAX_KEY / 2)      //情况A  
				{
					x.key[i] = child.key[child.nkey - 2];
					child.nkey--;

					WriteBPlusNode(current, x);
					WriteBPlusNode(x.Pointer[i], child);

					return;
				}
				else    //否则孩子节点的关键字数量不过半  
				{
					if (i > 0)      //有左兄弟节点  
					{
						BPlusNode lbchild;
						ReadBPlusNode(x.Pointer[i - 1], lbchild);

						if (lbchild.nkey > MAX_KEY / 2)        //情况B  
						{
							borrow(x, child, lbchild,i,current);
						}
						else    //情况C  
						{
							merge(x, child, lbchild, i, current);
						}
					}
					else      //只有右兄弟节点  
					{
						BPlusNode rbchild;
						ReadBPlusNode(x.Pointer[i + 1], rbchild);

						if (rbchild.nkey > MAX_KEY / 2)        //情况D  
						{
							borrow(x, child, rbchild, i, current);
						}
						else    //情况E  
						{
							merge(x, child, rbchild, i, current);
						}
					}
				}
			}
			else      //情况F  
			{

				//找到key在B+树叶节点的左兄弟关键字,将这个关键字取代key的位置  

				TRecord trecord;
				trecord.key = record.key;
				SearchResult result;
				Search_BPlus_Tree(trecord, result);

				BPlusNode last;

				ReadBPlusNode(result.Baddress, last);

				x.key[i] = last.key[last.nkey - 2];

				WriteBPlusNode(current, x);


				if (child.nkey > MAX_KEY / 2)        //情况H  
				{
					//孩子节点的关键字数量达到半满，不做处理
				}
				else          //否则孩子节点的关键字数量不过半,则将兄弟节点的某一个关键字移至孩子  
				{
					if (i > 0)  //x.key[i]有左兄弟  
					{
						BPlusNode lbchild;
						ReadBPlusNode(x.Pointer[i - 1], lbchild);

						if (lbchild.nkey > MAX_KEY / 2)       //情况I  
						{
							borrow(x, child, lbchild, i, current);
						}
						else        //情况J  
						{
							merge(x, child, lbchild, i, current);
						}

					}
					else        //否则x.key[i]只有右兄弟  
					{
						BPlusNode rbchild;
						ReadBPlusNode(x.Pointer[i + 1], rbchild);

						if (rbchild.nkey > MAX_KEY / 2)     //情况K  
						{

							borrow(x, child, rbchild, i, current);

						}
						else        //情况L  
						{
							merge(x, child, rbchild, i, current);

						}
					}
				}
			}

			delete_BPlus_tree(x.Pointer[i], record);

		}
		else  //情况G  
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
	else        //在当前节点没找到关键字     
	{
		if (!x.isleaf)    //没找到关键字,则关键字必然包含在以Pointer[i]为根的子树中  
		{
			BPlusNode child;
			ReadBPlusNode(x.Pointer[i], child);

			if (!child.isleaf)      //如果其孩子节点是内节点  
			{
				if (child.nkey > MAX_KEY / 2)        //情况H  
				{

				}
				else          //否则孩子节点的关键字数量不过半,则将兄弟节点的某一个关键字移至孩子  
				{
					if (i > 0)  //x.key[i]有左兄弟  
					{
						BPlusNode lbchild;
						ReadBPlusNode(x.Pointer[i - 1], lbchild);

						if (lbchild.nkey > MAX_KEY / 2)       //情况I  
						{
							borrow(x, child, lbchild, i, current);
						}
						else        //情况J  
						{
							merge(x, child, lbchild, i, current);

						}

					}
					else        //否则x.key[i]只有右兄弟  
					{
						BPlusNode rbchild;
						ReadBPlusNode(x.Pointer[i + 1], rbchild);

						if (rbchild.nkey > MAX_KEY / 2)     //情况K  
						{

							borrow(x, child, rbchild, i, current);

						}
						else        //情况L  
						{
							merge(x, child, rbchild, i, current);

						}

					}
				}
			}
			else  //否则其孩子节点是外节点  
			{
				if (child.nkey > MAX_KEY / 2)  //情况M  
				{

				}
				else        //否则孩子节点不到半满  
				{
					if (i > 0) //有左兄弟  
					{
						BPlusNode lbchild;
						ReadBPlusNode(x.Pointer[i - 1], lbchild);

						if (lbchild.nkey > MAX_KEY / 2)       //情况N  
						{
							borrow(x, child, lbchild, i, current);

						}
						else        //情况O  
						{

							merge(x, child, lbchild, i, current);

						}

					}
					else        //否则只有右兄弟  
					{
						BPlusNode rbchild;
						ReadBPlusNode(x.Pointer[i + 1], rbchild);

						if (rbchild.nkey > MAX_KEY / 2)       //情况P  
						{
							borrow(x, child, rbchild, i, current);

						}
						else        //情况Q  
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



void BPlusTree::Delete_BPlus_Tree(TRecord &record)    //在B+中删除一个关键字  
{
	delete_BPlus_tree(ROOT, record);

	BPlusNode rootnode;
	ReadBPlusNode(ROOT, rootnode);

	if (!rootnode.isleaf && rootnode.nkey == 0)    //如果删除关键字后根节点不是叶节点，并且关键字数量为0时根节点也应该被删除  
	{
		//释放ROOT节点占用的空间  
		ROOT = rootnode.Pointer[0];         //根节点下移,B+树高度减1  

	}

}




void BPlusTree::EnumLeafKey()    //依次枚举B+树叶节点的所有关键字  
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




inline FILEP BPlusTree::GetBPlusNode()  const //在磁盘上分配一块B+树节点空间,add to file's tail
{
	fseek(Bfile, 0, SEEK_END);

	return  ftell(Bfile);
}

inline void BPlusTree::ReadBPlusNode(const FILEP address, BPlusNode   &r) const //读取address地址上的一块B+树节点  
{
	fseek(Bfile, address, SEEK_SET);
	fread((char*)(&r), sizeof(BPlusNode), 1, Bfile);

}


inline void BPlusTree::WriteBPlusNode(const FILEP address, const BPlusNode &r) //将一个B+树节点写入address地址  
{
	fseek(Bfile, address, SEEK_SET);
	fwrite((char*)(&r), sizeof(BPlusNode), 1, Bfile);
}



int main()
{
	BPlusTree tree;

	tree.Build_BPlus_Tree();      //建树  

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