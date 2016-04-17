#ifndef __TL_H__
#define __TL_H__

#define tl_init             tl_init_
#define tl_destroy          tl_destroy_
#define tl_barrier          tl_barrier_
#define tl_add_task_safe    tl_add_task_safe_
#define tl_add_task_unsafe  tl_add_task_unsafe_
#define tl_add_task_named   tl_add_task_named_
#define tl_create_handles   tl_create_handles_
#define tl_create_handle    tl_create_handle_
#define tl_destroy_handles  tl_destroy_handles_
#define tl_destroy_handle   tl_destroy_handle_

#ifdef __cplusplus
extern "C" {
#endif

// tl_handle_t: opaque type for handles
typedef void *tl_handle_t;
// tl_task_function: task function signature. 
typedef void (*tl_task_function)(void *);

// tl_init: initialize the library
void tl_init(int *num_threads,int *node_id);

// tl_destroy: free library
void tl_destroy();

// tl_barrier: wait until all tasks have finished
void tl_barrier();

// tl_add_task_safe: add a task to be executed
//   all handles must be unique.
//
// arguments:
//   [in] function.....: function to call
//   [in] args.........: arguments passed on to function
//   [in] argsize......: size of argument struct
//   [in] read_handles.: pointer to array of handles for read access
//   [in] num_reads....: number of read accesses in read_handles
//   [in] write_handles: pointer to array of handles for write access
//   [in] num_writes...: number of write accesses in write_handles
//   [in] add_handles..: pointer to array of handles for add access
//   [in] num_adds.....: number of add accesses in add_handles
//   [in] name.........: name of task (for debugging)
void tl_add_task_safe(
  tl_task_function function,
  void *args, int *argsize,
  tl_handle_t **read_handles, int *num_reads,
  tl_handle_t **write_handles, int *num_writes,
  tl_handle_t **add_handles, int *num_adds);

// tl_add_task_unsafe: add a task to be executed
//   same as tl_add_task_safe, except handles need not be unique.
//   modifies read_handles, write_handles, add_handles.
void tl_add_task_unsafe(
  tl_task_function function,
  void *args, int *argsize,
  tl_handle_t **read_handles, int *num_reads,
  tl_handle_t **write_handles, int *num_writes,
  tl_handle_t **add_handles, int *num_adds);

// tl_add_task_: as add_task_unsafe, but with task name
void tl_add_task_named(
  const char *name,
  tl_task_function function,
  void *args, int *argsize,
  tl_handle_t **read_handles, int *num_reads,
  tl_handle_t **write_handles, int *num_writes,
  tl_handle_t **add_handles, int *num_adds,
  long int *len);


// tl_create_handles: create handles
//
// arguments:
//      [in] num....: number of handles to create
//   [inout] handles: preallocated vector to hold handles (size num*sizeof(tl_handle_t))
//      [in] name...: name of handles (for debugging)
void tl_create_handles(int *num, tl_handle_t **handles);

// tl_create_handles: create one handle
//
// arguments:
//   [inout] handles: preallocated space to hold handle
void tl_create_handle(tl_handle_t *handle);

// tl_destroy_handles: destroy handles
//    [in] handles: pointer to handles to free
void tl_destroy_handles(tl_handle_t **handles, int *num);

// tl_destroy_handles: destroy one handle
void tl_destroy_handle(tl_handle_t *handle);

void echo0_(int pointer_to_fortran_arg);
void echo1_(int *pointer_to_fortran_arg);
void echo2_(int **pointer_to_fortran_arg);

#ifdef __cplusplus
}
#endif

#endif // __TL_H__
