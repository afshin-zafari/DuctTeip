#include <dlfcn.h>
#include <strings.h>
#include <stdio.h>

int *dlopenf_(char *name, int *mode, int len)
{
   void *handle;
   char *blnk;

/* Strip any trailing blanks in the library name */

   blnk = strchr(name,' ');
   if (blnk) *blnk='\0';

   printf("name:%s,mode:%d,len:%d,\n",name,*mode,len);
   handle = dlopen(name, *mode);
   if ( handle== NULL ) 
    printf ("Error:%s\n",dlerror());
   printf("handle:%x\n",handle);

   return (handle);

}

int *dlsymf_(void *handle, char *name, int len)
{
   char *blnk;
   void *fptr;

/* Strip any trailing blanks in the symbol name */

   blnk = strchr(name, ' ');
   if (blnk) 
   {
      *blnk++='_';      /* Add a trailing underscore to the symbol.
                           Assuming that the library is a Fortran library */
      *blnk = '\0';
   }

   printf("symf  name:%s,len:%d,\n",name,len);
   printf("handle:%x\n",handle);
   fptr = dlsym(handle, name);
   
   printf("fptr:%x\n",fptr);
   if ( fptr == NULL ) 
    printf ("Error:%s\n",dlerror());
    
    typedef int (*pf)(int *,int*) ;
    typedef int (*taskfp)(void *);
    pf add;
    taskfp dtinit;
    *(void **) (&dtinit)=dlsym(handle, "m1.dt_init_");
//    *(void **) (&add )=dlsym(handle, name);;
//   printf("Add is %x\n",add);
   int a=1,b=3;
    int dd[4]={1,2,3,4};
struct {int a; float y;int z[4];char c[20];int **ip;} conf;
	//conf.ip =(int **)&dd;
	conf.y=22.8;conf.a=12;conf.z[0]=conf.z[1]=99;
    strcpy(conf.c,"In C helpers");
    printf("filled in C as %d,y %f,z %d,%d,%d,%d, c %s,ip %x\n",conf.a,conf.y,conf.z[0],conf.z[1],conf.z[2],conf.z[3],conf.c,conf.ip);
   //dtinit((void *)&conf); 

//   int c =  add(&a,&b);
   
   printf("Result is a %d,y %f,z %d,%d,%d,%d, c %s,ip %x\n",conf.a,conf.y,conf.z[0],conf.z[1],conf.z[2],conf.z[3],conf.c,conf.ip);

   return((void*)dtinit);

}
