#include <dlfcn.h>
#include <strings.h>
#include <stdio.h>

int *dlopenf_(char *name, int *mode, int len)
{
   void *handle;
   char *blnk;



   //printf("name:%s,mode:%d,len:%d,\n",name,*mode,len);
   handle = dlopen(name, *mode);
   if ( handle== NULL ) 
       printf ("Error:%s\n",dlerror());
   //printf("handle:%x\n",handle);

   return (handle);

}

int *dlsymf_(void *handle, char *name, int len)
{
   char *blnk;
   void *fptr;



   //printf("symf  name:%s,len:%d,\n",name,len);
   //printf("handle:%x\n",handle);
   fptr = dlsym(handle, name);
   
   //printf("fptr:%x,%d\n",fptr,fptr);
   if ( fptr == NULL ) 
       printf ("Error:%s\n",dlerror());

   return(fptr);

}
