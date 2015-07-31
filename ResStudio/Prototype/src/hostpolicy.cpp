#include "hostpolicy.hpp"  
#include "procgrid.hpp"
#include "glb_context.hpp"

int IHostPolicy::getHost(Coordinate c ,int ndim) {
  if ( ndim == 1 )
    return (c.bx % PG->getProcessorCount() ) ;
  return PG->getProcessor(c.by,c.bx);
}
bool IHostPolicy::isInRow(int me , int r ) {
  return PG->isInRow(me,r);
}
bool IHostPolicy::isInCol(int me , int c ) {
  return PG->isInCol(me,c);
}
/*----------------------------------------------------------------------*/
bool ContextHostPolicy::isAllowed(IContext *ctx,ContextHeader *hdr){
  if (active_policy == PROC_GROUP_CYCLIC){
    if (groupCounter  != DONT_CARE_COUNTER ) {
      int lower,upper;
      getHostRange(&lower,&upper);
      int group_size = upper - lower +1;
      int level = glbCtx.getDepth();
      bool b = ((groupCounter % group_size ) + lower ) == me;
      //if(b) printf("lv=%d,[%d %d],  gc=%d,gs=%d,allowed?%d\n",level,lower,upper,groupCounter,group_size,b);
      return b;
    }
    int a = 0 ;
    int b = PG->getProcessorCount()-1;
    int offset = 0 ;
    int s;
    int p;
    for ( int level=0; level<glbCtx.getDepth();level++){
      s = (b - a +1 )  / groupCount[level];
      p = glbCtx.getLevelID(level) % groupCount[level];
      a = p * s;
      b = (p+1) *s -1 ;
      if (me < (offset + a) || me > (offset+b) )  {
	//printf("ContextHostPolicy: %d is NOT in [%d,%d]\n",me,offset+a,offset+b);
	return false;
      }
      offset  += a;
    }
    if (me >= (offset) && me <= (offset-a+b) ) {
      //printf("ContextHostPolicy: %d IS in [%d,%d]\n",me,offset+a,offset+b);
      return true;
    }
    //printf("ContextHostPolicy: %d is NOT in [%d,%d]\n",me,offset+a,offset+b);
  }
  return false;
}
/*--------------------------------------------------------------*/
void ContextHostPolicy::getHostRange(int *lower, int *upper){
  if (active_policy == PROC_GROUP_CYCLIC){
    int a = 0 ;
    int b = PG->getProcessorCount()-1;
    int offset = 0 ;
    int s;
    int p;
    for ( int level=0; level<glbCtx.getDepth();level++){
      s = (b - a +1 )  / groupCount[level];
      p = glbCtx.getLevelID(level) % groupCount[level];
      a = p * s;
      b = (p+1) *s -1 ;
      offset  += a;
    }
    *lower = offset;
    *upper = offset -a + b;
  }
}
/*===================================================================================*/
bool TaskReadPolicy::isAllowed(IContext *c,ContextHeader *hdr){
  if ( active_policy == ALL_GROUP_MEMBERS ) {
    return true;
  }
  if ( active_policy == ALL_READ_ALL ) {
    return true;
  }
  return false;
}
/*===================================================================================*/
bool TaskAddPolicy::isAllowed(IContext *c,ContextHeader *hdr){
  int gc = glbCtx.getContextHostPolicy()->getGroupCounter();
  if ( gc  == DONT_CARE_COUNTER && (active_policy != WRITE_DATA_OWNER)  )
    return true;
  if ( active_policy  == ROOT_ONLY ) {
    return ( me == 0 ) ;
  }
  if ( active_policy ==NOT_OWNER_CYCLIC || active_policy == WRITE_DATA_OWNER) {
    int r,c;
    IData *A;
    list<DataRange *> dr = hdr->getWriteRange();
    list<DataRange *>::iterator it;
    //printf("TaskAddPolicy: isAllowed(%d)?\n",me);
    for ( it = dr.begin(); it != dr.end(); ++it ) {
      for (  r=(*it)->row_from; r<= (*it)->row_to;r++){
	for (  c=(*it)->col_from; c<= (*it)->col_to;c++){
	  A=(*it)->d;
	  //printf("A(%d,%d), %s\n",r,c,(*A)(r,c)->getName().c_str());
	  if ( (*A)(r,c)->isOwnedBy(me) ) {
	    //printf("Yes\n");
	    return true;
	  }
	}
      }
    }
    //printf("No\n");
    if (active_policy == WRITE_DATA_OWNER) 
      return false;
    it = dr.begin();
    r=(*it)->row_from;
    c=(*it)->col_from;
    A=(*it)->d;
    int owner = (*A)(r,c)->getHost () ;

    ContextHostPolicy *chp = static_cast<ContextHostPolicy *>(glbCtx.getContextHostPolicy());
    int lower,upper;
    chp->getHostRange(&lower,&upper);
    if ( owner >= lower && owner <= upper && owner != me)
      return false;
    int group_size = upper - lower + 1;
    bool b = ((not_owner_count % group_size ) + lower ) == me;
    //    printf("Not Owner, [%d %d] g-size=%d, me=%d,nocnt=%d,allowed=%d\n",
    //	  lower,upper,group_size,me,not_owner_count, b);
    not_owner_count++;
    return b;
  }
  return false;
}
