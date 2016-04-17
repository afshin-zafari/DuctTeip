! TODO: should be able to handle 32 and 64 bit memory addresses
module tl_handle_class

	implicit none

	type, public :: tl_handle
		!private
		integer :: address = 1337
		! integer, pointer :: address 
	end type tl_handle
	
end module tl_handle_class
