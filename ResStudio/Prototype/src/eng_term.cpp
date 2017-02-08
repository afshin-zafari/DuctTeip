#include "engine.hpp"
/*---------------------------------------------------------------*/
void engine::receivedTerminateCancel(int from){}
/*---------------------------------------------------------------*/
void engine::sendTerminateCancel(){}
inline bool engine::IsOdd (int a){return ( (a %2 ) ==1);}
inline bool engine::IsEven(int a){return ( (a %2 ) ==0);}
/*=================== Nodes Tree ===========================
                         0
              1                        2
       3            5            4             6
    7     9     11     13     8     10     12     14
  15 17 19 21 23  25 27  29 16 18 20  22 24  26 28  30
  ============================================================
*/
int engine::getParentNodeInTree(int node){
  int parent = -1;
  if ( node <2 ) {
    PRINT_IF(TERMINATE_FLAG)("parent of node:%d is %d\n",node,node-1);
    return node-1;
  }
  parent = node /2;
  if ( parent%2 != 0 && node%2 ==0 )
    parent --;
  else if ( parent%2 == 0 && node%2 !=0 )
    parent --;
  PRINT_IF(TERMINATE_FLAG)("parent of :%d is :%d\n",node,parent);
  return parent;
}
/*---------------------------------------------------------------------------------*/
void engine::getChildrenNodesInTree(int node,int *nodes,int *count){
  int N = net_comm->get_host_count();
  int left=2 *node + node%2;
  int right = 2 *(node+1) + node%2;
  if ( node ==0 ) {
    left  = 1;
    right = 2;
  }
  *count =0;
  nodes[0]=nodes[1]=-1;
  if (left <N ) {
    nodes[0] = left;
    *count=1;
  }
  if ( right <N ) {
    nodes[1]=right;
    *count=2;
  }
  PRINT_IF(TERMINATE_FLAG)("children of %d is %d,[0]=%d,[1]=%d\n",node,*count,nodes[0],nodes[1]);
}
/*---------------------------------------------------------------------------------*/
bool engine::amILeafInTree(){
  const int TREE_BASE=2; // binary tree
  int n,nodes[TREE_BASE];
  getChildrenNodesInTree(me,nodes,&n);
  PRINT_IF(TERMINATE_FLAG)("is leaf %d?%d\n ",me,n==0);
  if ( n==0)
    return true;
  return false;
}
/*---------------------------------------------------------------------------------*/
void engine::sendTerminateOK(){
  PRINT_IF(TERMINATE_FLAG)("send_term_ok called by node %d, term_ok:%d\n",me,term_ok);
  int parent = getParentNodeInTree(me);
  if (amILeafInTree() && term_ok ==TERMINATE_INIT){
    mailbox->send((byte*)&term_ok,sizeof(term_ok),MailBox::TerminateOKTag,parent);
    term_ok = SENT_TO_PARENT;
    PRINT_IF(TERMINATE_FLAG)("TERM_INIT->SENT 2 PARENT,node %d is leaf, sent TERM_OK to parent %d,term_ok is %d\n",
			     me,parent,term_ok);
  }
  else{
    if ( term_ok == ALL_CHILDREN_OK ) {
      if ( parent >=0) { // non-root node
	mailbox->send((byte*)&term_ok,sizeof(term_ok),MailBox::TerminateOKTag,parent);
	term_ok = SENT_TO_PARENT;
	PRINT_IF(TERMINATE_FLAG)("ALL_OK->SENT2PARENT,parent %d,term_ok:%d\n",parent,term_ok);
      }
      else{ // only root node
	const int TREE_BASE=2; // binary tree
	int n,nodes[TREE_BASE];
	bool wait = false;
	getChildrenNodesInTree(me,nodes,&n);
	if ( n >=1)
	  mailbox->send((byte*)&term_ok,sizeof(term_ok),MailBox::TerminateOKTag,nodes[0],wait);
	if ( n >=2)
	  mailbox->send((byte*)&term_ok,sizeof(term_ok),MailBox::TerminateOKTag,nodes[1],wait);
	term_ok = TERMINATE_OK;
	PRINT_IF(TERMINATE_FLAG)("ALL_OK->TERM_OK,node %d,child[0]:%d,child[1]:%d,term_ok:%d\n",
				 me,nodes[0],nodes[1],term_ok);
      }
    }
  }
}
/*---------------------------------------------------------------------------------*/
void engine::receivedTerminateOK(int from){
  const int TREE_BASE=2; // binary tree
  int n,nodes[TREE_BASE];
  int parent = getParentNodeInTree(me);
  getChildrenNodesInTree(me,nodes,&n);
  PRINT_IF(TERMINATE_FLAG)("node:%d recv TERM msg from:%d,my term_ok:%d\n",me,from,term_ok);

  if (term_ok == TERMINATE_INIT ){
    if ( from == nodes[0] || from == nodes[1]) {
      term_ok = ONE_CHILD_OK;
      PRINT_IF(TERMINATE_FLAG)("TERM_INIT->ONE CHILD OK,node :%d,term_ok:%d\n",me,term_ok);
      if (n == 1) {
	term_ok = ALL_CHILDREN_OK;
	PRINT_IF(TERMINATE_FLAG)("ONE_CHILD->ALL OK,node :%d has only one child,term_ok:%d\n"
				 ,me,term_ok);
      }
    }
  }
  else if (term_ok == ONE_CHILD_OK) {
    if ( from == nodes[0] || from == nodes[1]) {
      term_ok = ALL_CHILDREN_OK;
      PRINT_IF(TERMINATE_FLAG)("ONE OK->ALL OK,node :%d, term_ok:%d\n",me,term_ok);
    }
  }
  else if (term_ok == SENT_TO_PARENT && from == parent) {
    bool wait = false;
    if (n >=1)
      mailbox->send((byte*)&term_ok,sizeof(term_ok),MailBox::TerminateOKTag,nodes[0],wait);
    if (n >= 2)
      mailbox->send((byte*)&term_ok,sizeof(term_ok),MailBox::TerminateOKTag,nodes[1],wait);
    term_ok = TERMINATE_OK;
    PRINT_IF(TERMINATE_FLAG)("SENT 2 PARENT-> TERM OK, node %d,n:%d,child[0]:%d,child[1]:%d,term_ok:%d\n",
			     me,n,nodes[0],nodes[1],term_ok);
  }
}
