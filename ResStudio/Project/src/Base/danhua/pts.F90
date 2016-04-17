!$$$ pts.f90 $$$
!
! get_pointer_pts(rg,pts)
! The result is a pointer to a range of the points. Can be used in 
! calculations without naming the result (no copying of data). 
! The result can be assigned to a pointer. Any change of the values of this
! pointer is changing the actual points. Might be useful also in this way. 
!
! File format: Points can be read from a file, if it follows these rules:
!              The first line must have the number of points, the dimension,
!              and, the number of type blocks. (<=1 means no type info)
!              If there is no type information, each following line contains 
!              the coordinates for a point. 
!              If there are a number of type blocks, then each block starts 
!              with the type ID (integer as below) and the number of points
!              in the block, then each line has point coordinates until the 
!              next block.
!              NOTE: All points of the same type must be in the same block
!
#define MY_RANK ""
#ifdef PTS_DLEVEL
#define DEBUG_LEVEL PTS_DLEVEL  
#endif

#include "../../DuctTeip/debug.h"

Module class_point_set 
Use fp
Use const
Use class_field
Use output
!
Implicit None
!
Private
!
Public :: point_set
Public :: INTERIOR_POINT,BOUNDARY_POINT,DUPLICATED_POINT
Public :: new_pts, destruct_pts, copy_pts, get_pts_ptr, get_pts_id, &
          set_pts_id, range_pts, range_pts_id, &
          dims_pts, span_pts_id, write_pts, gen_pts, &
          read_pts_part,get_ids_ptr,pts_get_info,get_pts_sids,add_pts,set_pts_zero,pts_set_nt
          
!
Integer, Parameter :: INTERIOR_POINT=0
Integer, Parameter :: BOUNDARY_POINT=1
Integer, Parameter :: DUPLICATED_POINT=-1

Integer, Parameter,Public :: ID_COL_MAX = 5
Integer, Parameter,Public :: ID_COL_PNO = 5
Integer, Parameter,Public :: ID_COL_SID = 4
Integer, Parameter,Public :: ID_COL_TO = 3
Integer, Parameter,Public :: ID_COL_FROM = 2
Integer, Parameter,Public :: ID_COL_TYPE = 1





Type point_set  
  Private
  Type(field) :: id     ! first line of each block (point type; range(point number))
  Type(field) :: x      ! for points in different block
End Type point_set
!

Interface pts_get_info
  Module Procedure  pts_get_info1,pts_get_info2
End Interface

Interface read_pts_part
  Module Procedure  read_pts2
End Interface


Interface new_pts
  Module Procedure new_pts,new_pts2,new_pts3, read_pts
End Interface
!
Interface get_pts_ptr
  Module Procedure get_pts_ptr1, get_pts_ptr2, get_pts_ptr3
End Interface
!
Interface range_pts
  Module Procedure range_pts1, range_pts2
End Interface
!
Interface gen_pts
  Module Procedure gen_pts_line, gen_pts_grid, gen_pts_polr
End Interface
!
Contains
!
  Function new_pts(np,nd,nt) Result(p)
  Integer, Intent(in) :: np,nd
  Integer, Optional :: nt
  Type(point_set) :: p
!
!    write(*,*) "-np,nd,nt", np,nd,nt
    If (Present(nt) .And. nt>1) Then
      p%id = new_field(nt,ID_COL_MAX)                  ! need nt rows field for nt type blocks
    Else
      ! Default, every point is counted as interior (for interpolation e.g.)
      p%id = new_field(1,ID_COL_MAX)
      p%id%i%i2(1,1:2) = (/INTERIOR_POINT,1/)  ! point type; first location of points
      p%id%i%i2(1,3) = np                      ! last location
    End If
    p%x = new_field(.True.,np,nd) 
    p%id%i%i2 = 0 
  End Function new_pts
!
  Function new_pts2(x) Result(p)
  Type(point_set), Intent(in) :: x
  Type(point_set) :: p
!
  Integer :: np,nd,nt
!
    np = Ubound(x%x%r%r2,1)
    nd = Ubound(x%x%r%r2,2)

    nt = Ubound(x%id%i%i2,1)
!    write(*,*) "--np,nd,nt", np,nd,nt
    p = new_pts(np,nd,nt)
  End Function new_pts2
  
  Function new_pts3(id,x) Result(p)
    Type(point_set) :: p 
    Real(kind=rfp), Dimension(:,:), Intent(in) :: x 
    Integer , Dimension(:,:), Intent(in) :: id
    Integer:: np,nd,nt,l
    np = Maxval(id(:,ID_COL_TO))
    nt = Ubound(id,1)
    nd = Ubound(x,2)
!    write(*,*) "---np,nd,nt", np,nd,nt
    p = new_pts(np,nd,nt)
    p%id%i%i2 = id
    p%x%r%r2 = x
!    TRACE2("IDs",(p%id%i%i2(l,:),l=1,Ubound(p%id%i%i2,1)))
!    TRACE2("Pts",(p%x%r%r2 (l,:),l=1,Ubound(p%x%r%r2 ,1)))
  End Function new_pts3
!
  Function read_pts(fname) Result(p)
  Character(len=*), Intent(in) :: fname
  Type(point_set) :: p
  Integer :: np,nd,nt,j,k,m,i
!
    Open(38,file=fname)
    Read(38,*) np,nd,nt
    p = new_pts(np,nd,nt)    
    If (nt<=1) Then
      Do k=1,np
        Read(38,*) (p%x%r%r2(k,j), j=1,nd)
      End Do
      p%id%i%i2(1,ID_COL_FROM) = 1
      p%id%i%i2(1,ID_COL_TO  ) = np
      p%id%i%i2(1,ID_COL_TYPE) = 0

    Else
      i=0
      Do m=1,nt
        Read(38,*) p%id%i%i2(m,1), np  ! point type(inner/boundary);  point number
        p%id%i%i2(m,2) = i+1           ! location of first point of each type point
        p%id%i%i2(m,3) = i+np          ! last location of this type point
        Do k=1,np
          Read(38,*) (p%x%r%r2(i+k,j), j=1,nd)
        End Do
        i=i+np
      End Do
    End If
    Close(38)
  End Function read_pts

  Subroutine pts_get_info1(pts,np,nd,nt) 

      type(point_set) , intent(in) :: pts
      Integer ,Intent(out) 	   :: np,nd,nt


      np = ubound(pts%x%r%r2,1)
      nd = ubound(pts%x%r%r2,2)
      nt = ubound(pts%id%i%i2,1)

  End Subroutine pts_get_info1

!!$------------------------------------------------------------------------------------------------------------------------------
!!$ Returns the number of points, dimensions and types in a given Point Set file.
!!$------------------------------------------------------------------------------------------------------------------------------
  Subroutine pts_get_info2(fname,np,nd,nt) 

      Character(len=*), Intent(in) :: fname
      Integer ,Intent(out) 	   :: np,nd,nt

      Open(38,file=fname)
      Read(38,*) np,nd,nt
      Close(38)      
    
  End Subroutine pts_get_info2
!!$------------------------------------------------------------------------------------------------------------------------------
!!$ 
!!$ This subroutine is called when it is required to read the file of Point Sets in chunks. A callback subroutine is provided that
!!$ will be called when the chunk is read completely. Whether the chunks can contain more than one Point Type or no is determined 
!!$ by the 'wt' argument. The 'obj' argument is a pointer to an object which will be passed back to the caller when calling the 
!!$ callback function.
!!$ Note: The chunk size is specified in terms of NUMBER OF POINTS in a chunk and not by the memory space it requires.
!!$------------------------------------------------------------------------------------------------------------------------------

  Subroutine  read_pts2(p,fname,chunk_pts,wt,callback,obj) 
  
      Character(len=1)  , Dimension(:) ,Pointer :: pts,obj     
      Character(len=*)  , Intent(in)            :: fname
      Type(point_set)   , Intent(inout)         :: p
      Logical                                   :: wt
      Integer                                   :: np,nd,nt,j,k,m,&
                                                   rd_pts,chunk_pts,l,n,flag

      Interface
        Subroutine callback(pts,dist,flag)
        
            Character(len=1),Dimension(:),Intent(inout) :: pts,dist
            Integer                                     ::flag
            
        End Subroutine 
      End Interface
    !
    Allocate( pts(sizeof(p)) )
    
    Open(38,file=fname)
    Read(38,*) np,nd,nt
    
    If (nt<=1) Then
    
      p = new_pts(Min(chunk_pts,np) ,nd,nt)    
      
      Do k=1,np
      
        Read(38,*) (p%x%r%r2(k,j), j=1,nd)
        
        If ( Mod(k , chunk_pts) == 0 ) Then             
        
            pts = Transfer(p,pts)
            Call callback(pts,obj,1)

            Call destruct_pts(p)           	
            p = new_pts(Min(chunk_pts,np-k) ,nd,nt)    
            
        End If 
        
      End Do
      
      pts = Transfer(p,pts)
      
      Call callback(pts,obj,1)
      Call destruct_pts(p)
	  
    Else
    
      rd_pts = 1
      p = new_pts(chunk_pts ,nd,nt)                  
      p%id%i%i2 = 0
      
      Do m=1,nt
      
        Read(38,*) p%id%i%i2(m,1), np  
        
        p%id%i%i2(m,ID_COL_FROM) = rd_pts      
        
        Do k=1,np

            Read(38,*) (p%x%r%r2(rd_pts,j), j=1,nd)

            If ( rd_pts == chunk_pts ) Then             

                p%id%i%i2(m,ID_COL_TO) = rd_pts          
                
                TRACE4 ("++",(l,(p%id%i%i2(l,1:ID_COL_MAX),l=1,Ubound(p%id%i%i2,1))) ) 
                TRACE4 ("rd pts ", (p%x%r%r2(l,:),l=1,rd_pts) ) 
                
                flag = Count(p%id%i%i2(:,ID_COL_FROM) .NE. 0 )
                pts  = Transfer(p,pts)
                
                Call callback(pts,obj,flag)
                
                rd_pts =  0
                p%id%i%i2(:,ID_COL_FROM:ID_COL_TO) = 0
                
                If ( k < np ) p%id%i%i2(m,ID_COL_FROM) = 1
                
            End If 
            
            rd_pts = rd_pts + 1
            
        End Do

        p%id%i%i2(m,ID_COL_TO) = rd_pts-1

        If ( wt .Or. ( m == nt .And. rd_pts .ne. 1) ) Then 
        

            TRACE4 ("++",(l,(p%id%i%i2(l,1:ID_COL_MAX),l=1,Ubound(p%id%i%i2,1))) ) 
            TRACE4 ("rd pts ", (p%x%r%r2(l,:),l=1,rd_pts) ) 
            flag = Count(p%id%i%i2(:,ID_COL_FROM) .ne. 0 )
            pts  = Transfer(p,pts)
            
            Call callback(pts,obj,flag)
            p%id%i%i2(:,ID_COL_FROM:ID_COL_TO) = 0
            
            rd_pts = 1
        	
        End If ! remaining chunk of current type
        
      End Do ! loop over np
      
    End If !nt > 1
    
    Close(38)
  End Subroutine read_pts2
  
!!$------------------------------------------------------------------------------------------------------------------------------
!!$ This function appends the 'pts' Point Set data to the 'p'. It is assumed that 'p' has already adequate memory allocated to.
!!$ The from:to indices of the IDs in 'p' are updated accordingly to show the  positions of the new points data.
!!$------------------------------------------------------------------------------------------------------------------------------
 Function add_pts(p,pts) Result(succeed)

    Real(kind=rfp) ,Dimension(:,:) ,Pointer :: this_ptr ,in_ptr
    Integer	   ,Dimension(:,:) ,Pointer :: this_ids,in_ids
    Type(point_set),Intent(inout) 	    :: p ,pts
    Integer 				    :: ids_from,max_id,ptr_from,ptr_to,ptr_cnt,i,l
    Logical 				    :: succeed

    TRACE3("add_pts","Enter")

    succeed = .False.

    this_ids => get_ids_ptr(p)
    this_ptr => get_pts_ptr(p)
      in_ids => get_ids_ptr(pts)
      in_ptr => get_pts_ptr(pts)
      
    If ( .Not. Associated(in_ids) ) Then 
        TRACE3("add_pts","Exit,no in ids")
        Return 
    End If 

    If ( .Not. Associated(in_ptr) ) Then 
        TRACE3("add_pts","Exit,no in ptr")
        Return 
    End If 

    max_id  = Maxval(this_ids(:,ID_COL_TYPE) ) + 1
    ptr_cnt = Maxval(  in_ids(:,ID_COL_TO  ) ) 

    ids_from = 1
    ptr_from = 1

    Do i= 1,Ubound(this_ids,1)

        If (this_ids(i,ID_COL_FROM) == 0 .Or. this_ids(i,ID_COL_TO) == 0 ) Exit

        ids_from = i+1
        ptr_from =  this_ids(i,ID_COL_TO)+1
        TRACE3("This IDs",(this_ids(i,:) ))
    End Do 

    ptr_to = ptr_from + ptr_cnt-1
    TRACE3("max_id,pos",(max_id,ids_from))
    TRACE3("ptr_from,to,cnt",(ptr_from,ptr_to,ptr_cnt) )

    If (ptr_to > Ubound( this_ptr,1) .Or. ptr_cnt > Ubound(in_ptr,1) ) Then 
        TRACE3("this p#",Ubound( this_ptr,1))
        TRACE3("add_pts","Exit,idx out of range")
        Return 
    End If 
    
    Do i = 1, Ubound(in_ids,1)

        If (in_ids(i,ID_COL_FROM) == 0 .Or. in_ids(i,ID_COL_TO) == 0 ) Cycle

        this_ids(ids_from,:) = in_ids(i,:)
        this_ids(ids_from,ID_COL_FROM) = this_ids(ids_from,ID_COL_FROM) + ptr_from-1
        this_ids(ids_from,ID_COL_TO  ) = this_ids(ids_from,ID_COL_TO  ) + ptr_from-1
        this_ids(ids_from,ID_COL_TYPE) = max_id 

        max_id = max_id + 1

        TRACE3("ADDED ",(ids_from,this_ids(ids_from,:) ) )
        TRACE3("IN IDs",(i,in_ids(i,:) ))

        ids_from = ids_from + 1

    End Do 

    this_ptr(ptr_from:ptr_to,:) = in_ptr(1:ptr_cnt,:)
    TRACE3("pts filled f,t from 1,t",(ptr_from,ptr_to,ptr_cnt))
    succeed = .True.

    TRACE3("add_pts","Exit")
    
 End Function add_pts
  Subroutine get_pts_sids(p,i,id,sid)
    Type(point_set),Intent(inout) :: p 
    Integer , Intent (in) :: i 
    Integer, Intent (out) :: id,sid
    
    id  = p%id%i%i2(i,ID_COL_TYPE)
    sid = p%id%i%i2(i,ID_COL_SID)
    
  End Subroutine get_pts_sids
!
! 
  Subroutine destruct_pts(p)
  Type(point_set) :: p
!
    Call destruct_field(p%x)
    Call destruct_field(p%id)

  End Subroutine destruct_pts
!
! 
  Subroutine copy_pts(x,c)
  Type(point_set), Intent(in) :: x
  Type(point_set), Intent(out) :: c
!
    c%x%r%r2 = x%x%r%r2
    c%id%i%i2 = x%id%i%i2
  End Subroutine copy_pts
!
! get part of the pointer set
!
  Function get_pts_ptr1(rg,pts) Result(p)
  Integer, Dimension(1:2), Intent(in) :: rg
  Type(point_set), Intent(in) :: pts
  Real(kind=rfp), Dimension(:,:), Pointer :: p
!
    p => pts%x%r%r2(rg(1):rg(2),:)
  End Function get_pts_ptr1
!
!
  Function get_pts_ptr2(id,pts) Result(p)
  Integer, Intent(in) :: id
  Type(point_set) :: pts
  Real(kind=rfp), Dimension(:,:), Pointer :: p
  !
  Logical :: found
  Integer :: k,nb
  Integer, Dimension(1:2) :: rg
  !
    rg = range_pts1(id,pts)
    If ((rg(2)-rg(1)) > 0) Then
      p => pts%x%r%r2(rg(1):rg(2),:)
    Else
      Nullify(p)
    End If
  End Function get_pts_ptr2
!
!
  Function get_ids_ptr(pts) Result(p)
   Type(point_set), Intent(in) :: pts
   Integer , Dimension(:,:),Pointer::p
   integer :: l 
    If (Associated(pts%id%i%i2)) Then 
        p => pts%id%i%i2(:,:)
!        TRACE3("+IDs",(pts%id%i%i2(l,:),l=1,Ubound(pts%id%i%i2,1)))
!        TRACE3("+IDs",(p(l,:),l=1,Ubound(p,1)))
    Else
        Nullify(p)
!       TRACE3("null IDs","")
    End If 
  End Function get_ids_ptr
 
  Function get_pts_ptr3(pts) Result(p)
  Type(point_set), Intent(in) :: pts
  Real(kind=rfp), Dimension(:,:), Pointer :: p
!
    If (Associated(pts%x%r%r2)) Then 
       p => pts%x%r%r2(:,:)
    Else
        Nullify(p)
    End If 
  End Function get_pts_ptr3
!
!
  Subroutine get_pts_id(i,pts,Type,rg)
  Integer, Intent(in) :: i
  Type(point_set), Intent(in) :: pts
  Integer, Intent(out) :: Type
  Integer, Dimension(1:2), Intent(out) :: rg
!
    If (0 < i .And. i <= Ubound(pts%id%i%i2,1)) Then
      Type = pts%id%i%i2(i,1)
      rg = pts%id%i%i2(i,2:3)
    Else
      Write(*,*) "Warning: get_pts_id: Index i=",i," out of range"
    End If
  End Subroutine get_pts_id
!
!
  Subroutine set_pts_id(i,Type,rg,pts)
  Integer, Intent(in) :: i
  Integer, Intent(in) :: Type
  Integer, Dimension(1:2), Intent(in) :: rg
  Type(point_set), Intent(out) :: pts
!
    If (0 < i .And. i <= Ubound(pts%id%i%i2,1)) Then
      pts%id%i%i2(i,1) = Type
      pts%id%i%i2(i,2:3) = rg 
    Else
      Write(*,*) "Warning: set_pts_id failed: Index i=",i," out of range"
    End If
  End Subroutine set_pts_id
!
!
  Function range_pts1(id,pts) Result(rg)
  Integer, Intent(in) :: id
  Type(point_set) :: pts
  Integer, Dimension(1:2) :: rg
  !
  Integer :: k,nb
  !
    TRACE3("range_pts1","enter ")
    nb = Count(pts%id%i%i2(:,1)==id .And. pts%id%i%i2(:,2) .ne. 0 )
    If (nb>1) Then 
      Write(*,*) "WARNING: There are ", nb," blocks with points of type ",id
      Write(*,*) "Illegal structure. Only the first block will be retrieved"
    Elseif (nb==0) Then
      Write(*,*) "WARNING: In range_pts: No points of type ",id
      rg = (/1,0/)
    End If
!
    If (nb>=1) Then
      Do k=1,Ubound(pts%id%i%i2,1)
        If (pts%id%i%i2(k,1)==id) Then
          rg = pts%id%i%i2(k,2:3)
          Exit
        End If
      End Do    
    End If
    TRACE3("range_pts1","exit")
  End Function range_pts1
!
!
  Function range_pts2(pts) Result(rg)
  Type(point_set), Intent(in) :: pts
  Integer, Dimension(1:2) :: rg
!
    rg(1) = 1
    rg(2) = Ubound(pts%x%r%r2,1)
!    TRACE1("range_pts2",rg)

  End Function range_pts2
!
!
  Function range_pts_id(pts) Result(nt)
  Type(point_set), Intent(in) :: pts
  Integer :: nt
!
    nt = Ubound(pts%id%i%i2,1)   
  End Function range_pts_id
!
!
  Function dims_pts(pts) Result(nd)
    Type(point_set), Intent(in) :: pts
    Integer :: nd
!
    nd =  Ubound(pts%x%r%r2,2)
  End Function dims_pts
!
!

  Subroutine pts_set_nt(pts,nt)

    Type(point_set) , Intent(inout) :: pts
    Integer         , Intent(in)    :: nt
    Integer 			    :: block_sz,i
    
    pts%id = new_field(nt,ID_COL_MAX)
    TRACE3("PTS set nt",nt)
    block_sz= Ubound(pts%x%r%r2,1) / nt
    TRACE3("BLOCK Size",block_sz)
    Do i = 1,nt
       pts%id%i%i2(i,ID_COL_TYPE) =  i 
       pts%id%i%i2(i,ID_COL_FROM) = (i-1)*block_sz+1
       pts%id%i%i2(i,ID_COL_TO  ) =  i   *block_sz
       TRACE3("asm From,To",( (i-1)*block_sz+1 , i   *block_sz ) )
    End Do
    
  End Subroutine pts_set_nt

  Function span_pts_id(pts) Result(sp)

    Type(point_set), Intent(in) :: pts
    Integer, Dimension(1:2) :: sp

    sp(1) = Minval(pts%id%i%i2(:,1),pts%id%i%i2(:,ID_COL_FROM) /= 0)
    sp(2) = Maxval(pts%id%i%i2(:,1),pts%id%i%i2(:,ID_COL_FROM) /= 0)

  End Function span_pts_id


  Subroutine write_pts(pts,fname)
  Type(point_set), Intent(in) :: pts
  Character(len=*), Optional :: fname
  !
  Integer :: k
  !
    If (Present(fname)) Then
      Call write_to_file(fname,pts%x%r%r2)
    Else
      Do k=1,Ubound(pts%id%i%i2,1)
        Write(*,*) pts%id%i%i2(k,:)
      End Do
      write (*,*) "--------"
      Do k=1,Ubound(pts%x%r%r2,1)
        Write(*,*) pts%x%r%r2(k,:)
      End Do
    End If   
  End Subroutine write_pts
!
! 
  subroutine set_pts_zero(pts)
    Type(point_set) :: pts
    pts%id%i%i2 = 0 
    pts%x%r%r2  = 0
    
    end subroutine 

  Function gen_pts_line(rg,N,rnd) Result(x)
  Real(kind=rfp), Dimension(1:2), Intent(in) :: rg
  Integer, Intent(in) :: N  
  Real(kind=rfp), Optional, Intent(in) :: rnd
  Type(point_set) :: x
!
  Integer :: k
  Integer, Dimension(1:2) :: irg
  Real(kind=rfp) :: h
  Real(kind=rfp), Dimension(1:N-2) :: move
  !
    If ( N>0 .And. rg(2)>=rg(1)) Then
      If (N>1) Then
        x = new_pts(N,1,3)
        h = (rg(2)-rg(1))/Real(N-1,rfp)
        irg = 1
        Call set_pts_id(1,BOUNDARY_POINT,irg,x)
        irg(1) = 2; irg(2) = N-1
        Call set_pts_id(2,INTERIOR_POINT,irg,x)
        irg = N
        Call set_pts_id(3,2*BOUNDARY_POINT,irg,x)
      Else
        x = new_pts(N,1,1) ! Makes no sense to have three types for one point
        h = 0.0_rfp
        irg = 1
        Call set_pts_id(1,INTERIOR_POINT,irg,x)
      End If
      Do k=1,N
        x%x%r%r2(k,1) = rg(1) + h*(k-1)
      End Do
    Else
      Write(*,*) 'Error: Incompatible data given in gen_pts'
    End If
!
! Randomize if asked for
!
    If (Present(rnd) .And. N>2) Then
      !
      ! I am not sure if we get the same sequence every time we run a.out.
      !
      Call Random_number(move)
      move = rnd*2.0_rfp*(move-0.5_rfp)
      x%x%r%r2(2:N-1,1) = x%x%r%r2(2:N-1,1) + move
    End If    
  End Function gen_pts_line
!
!
  Function gen_pts_grid(x,y) Result(p)
  Type(point_set), Intent(in) :: x,y
!
  Type(point_set) :: p
!
  Integer :: np,nd,nt,nx,ny,i,j,pos
  Integer, Dimension(1:2) :: rg
  Real(kind=rfp), Dimension(:,:), Pointer :: pt,xp,yp
!
! We assume that x,y are one-dimensional and generated by 
! using for example gen_pts_line
!  
    nx = Ubound(x%x%r%r2,1)
    nd = Ubound(x%x%r%r2,2)
    ny = Ubound(y%x%r%r2,1)
    nd = Max(nd,Ubound(y%x%r%r2,2))
    If (nd > 1) Then
      Write(*,*) 'Error: All vectors must be 1D in gen_pts(x,y,z)'
    End If
    nd = 2 ! The grid is 2D
    np = nx*ny
    nt = 4+1
    xp => get_pts_ptr(x)
    yp => get_pts_ptr(y)

!
    p = new_pts(np,nd,nt)
    pt => get_pts_ptr(p)
!
! Since we need the points in blocks, we must first take all boundaries.
!
    !
    ! Boundaries with fixed x
    !
    Do i=1,2
      Do j=1,ny
        pos = (i-1)*ny + j
        pt(pos,1) = xp((i-1)*(nx-1)+1,1)
        pt(pos,2) = yp(j,1)
      End Do
      rg(1) = (i-1)*ny + 1; rg(2) = i*ny
      Call set_pts_id(i,i*BOUNDARY_POINT,rg,p)
    End Do
    !
    ! Boundaries with fixed y (corners excluded)
    !
    Do j=1,2
      Do i=2,nx-1
        pos = 2*ny + (j-1)*(nx-2) + i-1
        pt(pos,1) = xp(i,1)
        pt(pos,2) = yp((j-1)*(ny-1)+1,1)
      End Do
      rg(1) = 2*ny + (j-1)*(nx-2) + 1; rg(2) = 2*ny + j*(nx-2)
      Call set_pts_id(j+2,(j+2)*BOUNDARY_POINT,rg,p)
    End Do
    !
    ! Interior points
    !
    Do i=2,nx-1
      Do j=2,ny-1
        pos = 2*(ny+nx-2) + (i-2)*(ny-2) + j-1 
        pt(pos,1) = xp(i,1)
        pt(pos,2) = yp(j,1) 
      End Do
    End Do
    rg(1) = 2*(ny+nx-2) + 1; rg(2) = np
    Call set_pts_id(5,INTERIOR_POINT,rg,p)
    !
    ! We can also add randomization here.
    ! 
  End Function gen_pts_grid
!
!
  Function gen_pts_polr(R,Nr,Nt,c) Result(p)
  Real(kind=rfp), Intent(in) :: R
  Integer, Intent(in) :: Nr,Nt 
  Real(kind=rfp), Dimension(1:2), Intent(in), Optional :: c
!
  Type(point_set) :: p
!
  Integer :: i,j,np,nd,ni,pos
  Integer, Dimension(1:2) :: rg
  Real(kind=rfp) :: hr,ht
  Real(kind=rfp), Dimension(:,:), Pointer :: pt
!
    If (Nr <=1 .Or. Nt <=1) Then
      Write(*,*) 'Error: In gen_pts, Nr and Nt must both be > 1'
    Else
      np = (Nr-1)*Nt + 1  ! The origin is only one point
      nd = 2
      ni = 2
      p = new_pts(np,nd,ni)
      pt => get_pts_ptr(p)
      hr = R/(Nr-1)
      ht = 2.0_rfp*pi/Nt
      !
      ! Start with radius R (the boundary) and move inwards.  
      !
      Do i=Nr,2,-1
        Do j=1,Nt
          pos = (Nr-i)*Nt + j
          pt(pos,1) = hr*(i-1)*Cos(ht*(j-1))
          pt(pos,2) = hr*(i-1)*Sin(ht*(j-1))
        End Do
      End Do
      pt(np,:) = 0.0_rfp
      rg(1) = 1; rg(2) = Nt 
      Call set_pts_id(1,BOUNDARY_POINT,rg,p)
      rg(1) = Nt + 1; rg(2) = np
      Call set_pts_id(2,INTERIOR_POINT,rg,p)
      !
      ! If the center points is given, move x and y accordingly
      !
      If (Present(c)) Then
        pt(:,1) = pt(:,1) + c(1)
        pt(:,2) = pt(:,2) + c(2)
      End If
    End If
!
! When this set is unpacked in MATLAB, first reshape all except the last row
! with Nt, Nr-1, 2, then add the Nr:th  column of zeros to xx and yy.
! If desired, copy first row and add to end to get full circle.
!
  End Function gen_pts_polr
!
! NOTE! As needed, more routines for generating point sets will be added.
!
End Module class_point_set

