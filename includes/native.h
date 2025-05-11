#ifndef NATIVE_H
#define NATIVE_H

#include "object.h"
#include "helper.h"
#include "VM.h"
#include <time.h>

typedef struct
{
    char *name;
    NativeFn function;
    int arity;
} NativeMapping;

Value clock_native(VM *vm, Value *args);
Value typeof_native(VM *vm, Value *args);
Value to_string_native(VM *vm, Value *args);
Value len_native(VM *vm, Value *args);
void native_init(Table *globals);

#endif