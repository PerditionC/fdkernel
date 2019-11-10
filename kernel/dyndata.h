/*
    DynData.h
    
    declarations for dynamic NEAR data allocations
    
    the DynBuffer must initially be large enough to hold
    the PreConfig data.
    after the disksystem has been initialized, the kernel is 
    moveable and Dyn.Buffer resizable, but not before 
*/

#ifndef DYNDATA_H
#define DYNDATA_H

#ifdef __cplusplus
extern "C" {
#endif

void far *DynAlloc(char *what, unsigned num, unsigned size);
void far *DynLast(void);
void DynFree(void *ptr);

#ifdef __cplusplus
}
#endif

#endif
