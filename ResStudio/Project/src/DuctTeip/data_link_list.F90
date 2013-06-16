module data_link_list
  use tgen_types

  implicit none
  


contains
!-------------------------------------------------------------------
  function dl_get_cur(list) result(node)
    type(dlink_list) , intent(inout),pointer:: list
    type(dlink_list_node),pointer :: node
    
    node => list%first

  end function dl_get_cur
!-------------------------------------------------------------------

  function dl_pop_cur(list) result(node)
    type(dlink_list) , intent(inout),pointer:: list
    type(dlink_list_node),pointer :: node
    
    node => list%first
    list%first=>node%next

  end function dl_pop_cur

!-------------------------------------------------------------------
  function dl_new_list() result(list)
    type(dlink_list) , pointer:: list
    type(dlink_list_node),pointer :: node
    
    allocate (list) 
    nullify ( list%first )

  end function dl_new_list

!-------------------------------------------------------------------
  subroutine  dl_add_data(list,Dp)

    type(dlink_list) , intent(inout),pointer:: list
    type(data_object) ,intent(in),pointer:: Dp
    type(dlink_list_node),pointer :: node

    allocate(node)
    node%item = 0
    node%obj = 0
    node%next => list%first
    node%Dp => Dp
    list%first => node    

  end subroutine  dl_add_data

!-------------------------------------------------------------------
  function get_defined_data_for(list,M) result(Dp)
    type(dlink_list) , intent(in) :: list
    type(matrix_block), intent(in ) :: M
    type(data_object) , pointer:: Dp
    type(dlink_list_node),pointer :: cur
    cur => list%first
    do while ( associated(cur) ) 
       Dp=>cur%Dp
       cur=>cur%next
       if (Dp %M%by /= M%by) cycle
       if (Dp %M%bx /= M%bx) cycle
       if (Dp %M%mat_type /= M%mat_type) cycle
       return 
    end do
    Dp=>NULL()
  end function get_defined_data_for

!-------------------------------------------------------------------
  function get_data_for_id(list,id) result(Dp)
    type(dlink_list) , intent(in) :: list
    integer , intent(in) :: id
    type(data_object) , pointer:: Dp
    type(dlink_list_node),pointer :: cur
    cur => list%first
    do while ( associated(cur) ) 
       Dp=>cur%Dp
       cur=>cur%next
       if (Dp %Id  /= id ) cycle
       return 
    end do
    Dp=>NULL()
  end function get_data_for_id

!-------------------------------------------------------------------
  subroutine  dl_destroy(list) 
    type(dlink_list) , intent(in) :: list
    type(dlink_list_node),pointer :: c,n
    c => list%first
    do while ( associated(c) ) 
       n=>c%next
       deallocate( c%Dp%M)
       deallocate( c%Dp)
       deallocate( c ) 
       c=>n     
    end do
  end subroutine dl_destroy


end module data_link_list
