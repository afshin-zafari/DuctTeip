Module link_list_class
  Implicit None
  Type link_list
     Type(link_list_node),Pointer::first
  End Type link_list
  Type link_list_node
     Integer :: item,obj
     Type(link_list_node),Pointer::next
  End Type link_list_node


contains

  Function ll_get_cur(list) Result(node)
    Type(link_list) , Intent(inout),Pointer:: list
    Type(link_list_node),Pointer :: node
    
    node => list%first

  End Function ll_get_cur

  Function ll_pop_cur(list) Result(node)
    Type(link_list) , Intent(inout),Pointer:: list
    Type(link_list_node),Pointer :: node
    
    node => list%first
    list%first=>node%next

  End Function ll_pop_cur

  Function ll_new_list() Result(list)
    Type(link_list) , Pointer:: list
    Type(link_list_node),Pointer :: node
    
    Allocate (list) 
    Nullify ( list%first )

  End Function ll_new_list

  Subroutine  ll_add_item(list,item,obj_type)

    Type(link_list) , Intent(inout),Pointer:: list
    Integer , Intent(in) :: item,obj_type
    Type(link_list_node),Pointer :: node

    Allocate(node)
    node%item = item
    node%obj = obj_type
    node%next => list%first
    list%first => node
    

  End Subroutine  ll_add_item

End Module link_list_class
