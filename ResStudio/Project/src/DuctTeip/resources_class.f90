module resources_class

	implicit none
	type GPU
		integer :: Memory
		integer:: ComputeCapacity
	end type GPU
	type core
		integer :: Memory
	end type core
	type node
		type(Core),dimension(1:16)    ::cores
		type(GPU),dimension(1:2)      ::gpus
                integer :: TotalMemory
                character(len=20)::ProcCode
	end type node
	type cluster
		type(node), dimension(:),pointer::nodes
		integer :: TotalMemory
		integer :: TotalProcess
		integer :: MinMemoryPerNode
		integer :: MaxMemoryPerNode
	end type cluster
	type resources
		type(cluster),dimension(1:2)::clusters
                integer :: rank
	end type resources
	
	contains

	function res_new(rank) result (this)
	
            type(resources) :: this
            integer :: rank
            allocate(this%clusters(1)%nodes(1:1))
            this%rank = rank
            this%clusters(1)%nodes(1)%TotalMemory = this%rank
            this%clusters(1)%nodes(1)%ProcCode= "Proc_"//achar(this%rank+48)
        end function res_new
	
	function res_get_node_info(this,idx) result (the_node)
	
            type(resources) :: this
            type(node):: the_node
            integer, optional::idx

            if( .not. present (idx)  ) then
                idx = 1
            end if

            the_node = this%clusters(1)%nodes(idx)
	
            end function res_get_node_info

        subroutine res_set_node_info(this,idx, mem,pcode)
            type(resources) :: this
            integer :: mem,idx
            character(len=20),optional :: pcode
            this%clusters(1)%nodes(idx)%TotalMemory = mem
            if ( present(pcode)) then
                this%clusters(1)%nodes(idx)%ProcCode= pcode
            end if
	end subroutine res_set_node_info


end module  resources_class

