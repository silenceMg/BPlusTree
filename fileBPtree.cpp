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


void    BPlusTree::delete_BPlus_tree(FILEP current, TRecord &record)
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
				{     //1、找到关键字，当前节点是内节点，孩子是叶子节点，孩子节点半满
					  //直接删除
					x.key[i] = child.key[child.nkey - 2];
					child.nkey--;

					WriteBPlusNode(current, x);
					WriteBPlusNode(x.Pointer[i], child);
                    //删除完就return了
					return;
				}
				else    //否则孩子节点的关键字数量不过半  
				{
					if (i > 0)      //有左兄弟节点  
					{
						BPlusNode lbchild;
						ReadBPlusNode(x.Pointer[i - 1], lbchild);
						//2、找到关键字，当前节点是内节点，孩子是叶子节点，孩子节点不半满，左孩子半满
						//向左孩子借record									
						if (lbchild.nkey > MAX_KEY / 2)        //情况B  
						{
							//右移键值和指针
							for (j = child.nkey; j > 0; j--)
							{
								child.key[j] = child.key[j - 1];
								child.Pointer[j] = child.Pointer[j - 1];
							}
                            //下放父节点键值和指针
							child.key[0] = x.key[i - 1];
							child.Pointer[0] = lbchild.Pointer[lbchild.nkey - 1];

							child.nkey++;

							lbchild.nkey--;
                            //更新父节点键值和指针
							x.key[i - 1] = lbchild.key[lbchild.nkey - 1];
							x.key[i] = child.key[child.nkey - 2];
                            //写回
							WriteBPlusNode(current, x);
							WriteBPlusNode(x.Pointer[i - 1], lbchild);
							WriteBPlusNode(x.Pointer[i], child);

						}
						else    //情况C  
						{
							//3、找到关键字，当前节点是内节点，孩子是叶子节点，孩子节点不半满，左孩子也不半满
							//向左孩子合并,child页加入freelist
								  //拷贝
							for (j = 0; j < child.nkey; j++)
							{
								lbchild.key[lbchild.nkey + j] = child.key[j];
								lbchild.Pointer[lbchild.nkey + j] = child.Pointer[j];
							}
							lbchild.nkey += child.nkey;

							lbchild.Pointer[MAX_KEY] = child.Pointer[MAX_KEY];


							//更新当前内节点
							for (j = i - 1; j < x.nkey - 1; j++)
							{
								x.key[j] = x.key[j + 1];
								x.Pointer[j + 1] = x.Pointer[j + 2];
							}
							x.nkey--;
							//i-1指向新的lbchild右端(末端)
							x.key[i - 1] = lbchild.key[lbchild.nkey - 2];

							WriteBPlusNode(current, x);
							WriteBPlusNode(x.Pointer[i - 1], lbchild);
							//游标减1
							i--;

						}


					}
					else      //只有右兄弟节点  
					{
						
						BPlusNode rbchild;
						ReadBPlusNode(x.Pointer[i + 1], rbchild);
                        
						if (rbchild.nkey > MAX_KEY / 2)        //情况D  
						{
							//4、找到关键字，当前节点是内节点，孩子是叶子节点，孩子节点不半满，只有右兄弟节点，右兄弟节点半满
						    //向右孩子借record
							x.key[i] = rbchild.key[0];
							child.key[child.nkey] = rbchild.key[0];
							child.Pointer[child.nkey] = rbchild.Pointer[0];
							child.nkey++;

							for (j = 0; j < rbchild.nkey - 1; j++)
							{
								rbchild.key[j] = rbchild.key[j + 1];
								rbchild.Pointer[j] = rbchild.Pointer[j + 1];
							}

							rbchild.nkey--;

							WriteBPlusNode(current, x);
							WriteBPlusNode(x.Pointer[i], child);
							WriteBPlusNode(x.Pointer[i + 1], rbchild);

						}
						else    //情况E  
						{
							//5、找到关键字，当前节点是内节点，孩子是叶子节点，孩子节点不半满，只有右兄弟节点，右兄弟节点不半满
						    //右兄弟节点向左孩子节点合并
							for (j = 0; j < rbchild.nkey; j++)
							{
								child.key[child.nkey + j] = rbchild.key[j];
								child.Pointer[child.nkey + j] = rbchild.Pointer[j];
							}
							child.nkey += rbchild.nkey;

							child.Pointer[MAX_KEY] = rbchild.Pointer[MAX_KEY];

							//释放rbchild占用的空间x.Pointer[i+1]  

							for (j = i; j < x.nkey - 1; j++)
							{
								x.key[j] = x.key[j + 1];
								x.Pointer[j + 1] = x.Pointer[j + 2];
							}
							x.nkey--;

							WriteBPlusNode(current, x);
							WriteBPlusNode(x.Pointer[i], child);

						}

					}

				}

			}
			else      //情况F  
			{
                //6、找到关键字key，当前节点是内节点，孩子也是内节点
				//找到key在B+树叶节点的左兄弟关键字,将这个关键字取代key的位置  

				TRecord trecord;
				trecord.key = record.key;
				
				SearchResult result;
				Search_BPlus_Tree(trecord, result);
                //找到了叶子节点左兄弟节点
				BPlusNode last;
				ReadBPlusNode(result.Baddress, last);
                //更新当前节点后写回
				x.key[i] = last.key[last.nkey - 2];
				WriteBPlusNode(current, x);

                //接下来要保证当前节点的孩子节点半满，内部节点的借用以及合并和叶子节点的借用合并不一样
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
							//7、找到关键字key，当前节点是内节点，孩子也是内节点，孩子节点不半满，有左兄弟节点，左兄弟节点半满
							//向左兄弟节点借record
							
							//腾出1个单位的空间
							for (j = child.nkey; j > 0; j--)
							{
								child.key[j] = child.key[j - 1];
								child.Pointer[j + 1] = child.Pointer[j];
							}
							child.Pointer[1] = child.Pointer[0];
							
							child.key[0] = x.key[i - 1];
							child.Pointer[0] = lbchild.Pointer[lbchild.nkey];

							child.nkey++;

							x.key[i - 1] = lbchild.key[lbchild.nkey - 1];
							lbchild.nkey--;

							WriteBPlusNode(current, x);
							WriteBPlusNode(x.Pointer[i - 1], lbchild);
							WriteBPlusNode(x.Pointer[i], child);
						}
						else        //情况J  
						{
							//8、找到关键字key，当前节点是内节点，孩子也是内节点，孩子节点不半满，有左兄弟节点，左兄弟节点不半满
							//向左兄弟节点合并，把清空后的child节点加入freelist
							lbchild.key[lbchild.nkey] = x.key[i - 1];   //将孩子节点复制到其左兄弟的末尾  
							lbchild.nkey++;

							for (j = 0; j < child.nkey; j++)      //将child节点拷贝到lbchild节点的末尾,  
							{
								lbchild.key[lbchild.nkey + j] = child.key[j];
								lbchild.Pointer[lbchild.nkey + j] = child.Pointer[j];
							}
							lbchild.Pointer[lbchild.nkey + j] = child.Pointer[j];
							lbchild.nkey += child.nkey;        //已经将child拷贝到lbchild节点  


							//释放child节点的存储空间,x.Pointer[i]  


							//将找到关键字的孩子child与关键字左兄弟的孩子lbchild合并后,将该关键字前移,使当前节点的关键字减少一个  
							for (j = i - 1; j < x.nkey - 1; j++)
							{
								x.key[j] = x.key[j + 1];
								x.Pointer[j + 1] = x.Pointer[j + 2];
							}
							x.nkey--;

							WriteBPlusNode(current, x);
							WriteBPlusNode(x.Pointer[i - 1], lbchild);

							i--;

						}

					}
					else        //否则x.key[i]只有右兄弟  
					{
						
						BPlusNode rbchild;
						ReadBPlusNode(x.Pointer[i + 1], rbchild);

						if (rbchild.nkey > MAX_KEY / 2)     //情况K  
						{
							//9、找到关键字key，当前节点是内节点，孩子也是内节点，孩子节点不半满，有右兄弟节点，右兄弟节点半满
							//从右兄弟节点借record
	
							child.key[child.nkey] = x.key[i];
							child.nkey++;

							child.Pointer[child.nkey] = rbchild.Pointer[0];
							x.key[i] = rbchild.key[0];

							for (j = 0; j < rbchild.nkey - 1; j++)
							{
								rbchild.key[j] = rbchild.key[j + 1];
								rbchild.Pointer[j] = rbchild.Pointer[j + 1];
							}
							rbchild.Pointer[j] = rbchild.Pointer[j + 1];
							rbchild.nkey--;

							WriteBPlusNode(current, x);
							WriteBPlusNode(x.Pointer[i], child);
							WriteBPlusNode(x.Pointer[i + 1], rbchild);

						}
						else        //情况L  
						{
							//10、找到关键字key，当前节点是内节点，孩子也是内节点，孩子节点不半满，有右兄弟节点，右兄弟节点不半满
							//向左合并
							child.key[child.nkey] = x.key[i];
							child.nkey++;

							for (j = 0; j < rbchild.nkey; j++)     //将rbchild节点合并到child节点后  
							{
								child.key[child.nkey + j] = rbchild.key[j];
								child.Pointer[child.nkey + j] = rbchild.Pointer[j];
							}
							child.Pointer[child.nkey + j] = rbchild.Pointer[j];

							child.nkey += rbchild.nkey;

							//释放rbchild节点所占用的空间,x,Pointer[i+1]  

							for (j = i; j < x.nkey - 1; j++)    //当前将关键字之后的关键字左移一位,使该节点的关键字数量减一  
							{
								x.key[j] = x.key[j + 1];
								x.Pointer[j + 1] = x.Pointer[j + 2];
							}
							x.nkey--;

							WriteBPlusNode(current, x);
							WriteBPlusNode(x.Pointer[i], child);

						}

					}
				}

			}

			delete_BPlus_tree(x.Pointer[i], record);

		}
		else  //情况G  
		{
			//11、在当前节点找到关键字，当前节点是叶子节点，直接删除返回
			for (j = i; j < x.nkey - 1; j++)
			{
				x.key[j] = x.key[j + 1];
				x.Pointer[j] = x.Pointer[j + 1];
			}
			x.nkey--;

			WriteBPlusNode(current, x);
            //直接返回
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
			{   //递归下降，下降之前保证子节点是半满的
			
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
							for (j = child.nkey; j > 0; j--)
							{
								child.key[j] = child.key[j - 1];
								child.Pointer[j + 1] = child.Pointer[j];
							}
							child.Pointer[1] = child.Pointer[0];
							child.key[0] = x.key[i - 1];
							child.Pointer[0] = lbchild.Pointer[lbchild.nkey];

							child.nkey++;

							x.key[i - 1] = lbchild.key[lbchild.nkey - 1];
							lbchild.nkey--;

							WriteBPlusNode(current, x);
							WriteBPlusNode(x.Pointer[i - 1], lbchild);
							WriteBPlusNode(x.Pointer[i], child);
						}
						else        //情况J  
						{
							lbchild.key[lbchild.nkey] = x.key[i - 1];   //将孩子节点复制到其左兄弟的末尾  
							lbchild.nkey++;

							for (j = 0; j < child.nkey; j++)      //将child节点拷贝到lbchild节点的末尾,  
							{
								lbchild.key[lbchild.nkey + j] = child.key[j];
								lbchild.Pointer[lbchild.nkey + j] = child.Pointer[j];
							}
							lbchild.Pointer[lbchild.nkey + j] = child.Pointer[j];
							lbchild.nkey += child.nkey;        //已经将child拷贝到lbchild节点  


							//释放child节点的存储空间,x.Pointer[i]  


							//将找到关键字的孩子child与关键字左兄弟的孩子lbchild合并后,将该关键字前移,使当前节点的关键字减少一个  
							for (j = i - 1; j < x.nkey - 1; j++)
							{
								x.key[j] = x.key[j + 1];
								x.Pointer[j + 1] = x.Pointer[j + 2];
							}
							x.nkey--;

							WriteBPlusNode(current, x);
							WriteBPlusNode(x.Pointer[i - 1], lbchild);

							i--;

						}

					}
					else        //否则x.key[i]只有右兄弟  
					{
						BPlusNode rbchild;
						ReadBPlusNode(x.Pointer[i + 1], rbchild);

						if (rbchild.nkey > MAX_KEY / 2)     //情况K  
						{

							child.key[child.nkey] = x.key[i];
							child.nkey++;

							child.Pointer[child.nkey] = rbchild.Pointer[0];
							x.key[i] = rbchild.key[0];

							for (j = 0; j < rbchild.nkey - 1; j++)
							{
								rbchild.key[j] = rbchild.key[j + 1];
								rbchild.Pointer[j] = rbchild.Pointer[j + 1];
							}
							rbchild.Pointer[j] = rbchild.Pointer[j + 1];
							rbchild.nkey--;

							WriteBPlusNode(current, x);
							WriteBPlusNode(x.Pointer[i], child);
							WriteBPlusNode(x.Pointer[i + 1], rbchild);

						}
						else        //情况L  
						{
							child.key[child.nkey] = x.key[i];
							child.nkey++;

							for (j = 0; j < rbchild.nkey; j++)     //将rbchild节点合并到child节点后  
							{
								child.key[child.nkey + j] = rbchild.key[j];
								child.Pointer[child.nkey + j] = rbchild.Pointer[j];
							}
							child.Pointer[child.nkey + j] = rbchild.Pointer[j];

							child.nkey += rbchild.nkey;

							//释放rbchild节点所占用的空间,x,Pointer[i+1]  

							for (j = i; j < x.nkey - 1; j++)    //当前将关键字之后的关键字左移一位,使该节点的关键字数量减一  
							{
								x.key[j] = x.key[j + 1];
								x.Pointer[j + 1] = x.Pointer[j + 2];
							}
							x.nkey--;

							WriteBPlusNode(current, x);
							WriteBPlusNode(x.Pointer[i], child);

						}

					}
				}
			}
			else  //否则其孩子节点是外节点  
			{
				//12、当前节点是内节点，当前节点中没找到关键字，孩子节点是叶子节点
				//则关键字必然包含在Pointer[i]指向的叶子节点中，保证当前节点的孩子节点半满，然后递归删除
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
							for (j = child.nkey; j > 0; j--)
							{
								child.key[j] = child.key[j - 1];
								child.Pointer[j] = child.Pointer[j - 1];
							}
							child.key[0] = x.key[i - 1];
							child.Pointer[0] = lbchild.Pointer[lbchild.nkey - 1];
							child.nkey++;
							lbchild.nkey--;

							x.key[i - 1] = lbchild.key[lbchild.nkey - 1];

							WriteBPlusNode(x.Pointer[i - 1], lbchild);
							WriteBPlusNode(x.Pointer[i], child);
							WriteBPlusNode(current, x);

						}
						else        //情况O  
						{

							for (j = 0; j < child.nkey; j++)        //与左兄弟孩子节点合并  
							{
								lbchild.key[lbchild.nkey + j] = child.key[j];
								lbchild.Pointer[lbchild.nkey + j] = child.Pointer[j];
							}
							lbchild.nkey += child.nkey;

							lbchild.Pointer[MAX_KEY] = child.Pointer[MAX_KEY];

							//释放child占用的空间x.Pointer[i]  

							for (j = i - 1; j < x.nkey - 1; j++)
							{
								x.key[j] = x.key[j + 1];
								x.Pointer[j + 1] = x.Pointer[j + 2];
							}

							x.nkey--;

							WriteBPlusNode(x.Pointer[i - 1], lbchild);
							WriteBPlusNode(current, x);

							i--;

						}

					}
					else        //否则只有右兄弟  
					{
						BPlusNode rbchild;
						ReadBPlusNode(x.Pointer[i + 1], rbchild);

						if (rbchild.nkey > MAX_KEY / 2)       //情况P  
						{
							x.key[i] = rbchild.key[0];
							child.key[child.nkey] = rbchild.key[0];
							child.Pointer[child.nkey] = rbchild.Pointer[0];
							child.nkey++;

							for (j = 0; j < rbchild.nkey - 1; j++)
							{
								rbchild.key[j] = rbchild.key[j + 1];
								rbchild.Pointer[j] = rbchild.Pointer[j + 1];
							}
							rbchild.nkey--;

							WriteBPlusNode(current, x);
							WriteBPlusNode(x.Pointer[i + 1], rbchild);
							WriteBPlusNode(x.Pointer[i], child);

						}
						else        //情况Q  
						{
							for (j = 0; j < rbchild.nkey; j++)
							{
								child.key[child.nkey + j] = rbchild.key[j];
								child.Pointer[child.nkey + j] = rbchild.Pointer[j];
							}
							child.nkey += rbchild.nkey;
							child.Pointer[MAX_KEY] = rbchild.Pointer[MAX_KEY];

							//释放rbchild占用的空间x.Pointer[i+1]  

							for (j = i; j < x.nkey - 1; j++)
							{
								x.key[j] = x.key[j + 1];
								x.Pointer[j + 1] = x.Pointer[j + 2];
							}
							x.nkey--;

							WriteBPlusNode(current, x);
							WriteBPlusNode(x.Pointer[i], child);


						}

					}

				}

			}

			delete_BPlus_tree(x.Pointer[i], record);
		}
        //else树中没有这个值，所以直接返回就行

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
