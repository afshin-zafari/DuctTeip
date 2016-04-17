#ifdef PARAMETERS_DLEVEL 
#define DEBUG_LEVEL PARAMETERS_DLEVEL 
#endif

!TODO : Comments
!TODO : Parameter Read From File
!TODO : Parameter Declaration by User

#include "debug.h"

Module parameters_class
    Use class_epsilon
    Use constants
    Use class_point_set
    Use fp

    Implicit None

    Integer,Parameter ::MAX_POINTS_PER_PROC = 100
    Type par_item
        Real(kind=rfp)::eps
        Character(len=PHI_NAME_LENGTH)::phi
        Character(len=FILE_NAME_LENGTH)::pts
        Integer::partition_id
        Logical :: done
    End Type par_item

    Type eps_parts
        Type(epsilon) ::eps
        Integer, Dimension(:),Pointer::parts
    End Type eps_parts

    Type parameters
        Type(par_item),Dimension(:),Pointer::par_list 
        !integer::last_idx
    End Type parameters

    Interface par_new
        Module Procedure par_new,par_new_prts
    End Interface
    Interface par_add
        Module Procedure par_add,par_add_prts
    End Interface

Contains
    Function par_get_params(this,idx) Result(pitem)
        Type(parameters) , Intent(in) :: this
        Integer :: idx
        Type(par_item)::pitem
        If ( idx > Ubound(this%par_list,1) .Or. idx < Lbound(this%par_list,1) ) Then
            pitem%phi = ""
            pitem%pts = ""
            pitem%eps = 0.0_rfp
            DEBUG_PRINT("par set idx",idx)
            Return
        End If
        pitem = this%par_list(idx) 
    End Function par_get_params
    Subroutine par_set_done(this,idx)
        Type(parameters) :: this
        Integer :: idx
        If (idx <= Ubound(this%par_list,1) .And. idx >= Lbound(this%par_list,1) )  &
             this%par_list(idx)%done = .True.
    End Subroutine par_set_done
    
    Function par_get_next_large_prob(this) Result(idx)
        Type(parameters) :: this
        Integer :: idx,np,nd,nt

        Do idx = 1,Ubound(this%par_list,1)
!           If (.Not. this%par_list(idx)%done) &
!                Call pts_get_info(this%par_list(idx)%pts,np,nd,nt)
           If (np>MAX_POINTS_PER_PROC)&
                Return
        End Do
        idx = 0 
    End Function par_get_next_large_prob
    
    Function par_new(phi_list,eps,file_list) Result(this)
        Type(parameters) :: this
        Type(epsilon),Intent(in) :: eps
        Character(len=FILE_NAME_LENGTH),Dimension(:) ,Intent(in) :: file_list
        Character(len=PHI_NAME_LENGTH),Dimension(:),Intent(in) :: phi_list
        Integer :: i_phi,i_eps,i_pts,dims,tot_comb,idx
        Integer ,Dimension ( 1:2):: sz
        Logical :: eps_real

        Call query_eps(eps,eps_real,dims,sz)
        If ( .Not. eps_real ) Then
            DEBUG_PRINT("eps_real",eps_real)
            Return
        End If

        tot_comb = sz(1) * Size(phi_list) * Size(file_list)
        Allocate ( this%par_list(0:tot_comb-1) )
        idx = 0
        Do i_phi = Lbound(phi_list,1),Ubound(phi_list,1)
            Do i_eps = 1,sz(1)
                Do i_pts = Lbound(file_list,1),Ubound(file_list,1)
                    this%par_list(idx)%eps = get_eps(eps,i_eps)
                    this%par_list(idx)%phi = Trim(phi_list(i_phi))
                    this%par_list(idx)%pts = Trim(file_list(i_pts))
                    this%par_list(idx)%done = .False.                    
                    idx = idx + 1
                End Do
            End Do
        End Do
    End Function par_new

    Function par_new_prts(phi_list,eps_prts,file_list) Result(this)
        Type(parameters) ,pointer:: this
        Type(eps_parts),Intent(in) :: eps_prts
        Character(len=FILE_NAME_LENGTH),Dimension(:) , Intent(in):: file_list
        Character(len=PHI_NAME_LENGTH),Dimension(:),Intent(in) :: phi_list
        allocate(this)
    End Function par_new_prts

    Function par_new_eps_parts(eps,parts) Result(eps_prts)
        Type(epsilon),Intent(in) :: eps
        Integer,Dimension(:),Intent(in)::parts
        Type(eps_parts),pointer ::eps_prts
        allocate(eps_prts)
    End Function par_new_eps_parts

    Subroutine par_add(this,phi_list,eps,file_list)
        Type(parameters),Intent(inout) :: this
        Type(epsilon),Intent(in) :: eps
        Character(len=FILE_NAME_LENGTH),Dimension(:),Intent(in) :: file_list
        Character(len=PHI_NAME_LENGTH),Dimension(:),Intent(in) :: phi_list
    End Subroutine par_add
    
    Subroutine par_add_prts(this,phi_list,eps_prts,file_list)
        Type(parameters),Intent(inout) :: this
        Type(eps_parts),Intent(in) ::eps_prts
        Character(len=FILE_NAME_LENGTH),Dimension(:),Intent(in) :: file_list
        Character(len=PHI_NAME_LENGTH),Dimension(:),Intent(in) :: phi_list
    End Subroutine par_add_prts

End Module parameters_class
