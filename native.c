#include "native.h"

NativeMapping natives[] = {
    {"typeof", typeof_native, 1},
    {"to_string", to_string_native, 1},
    {"len", len_native, 1},
    {"clock", clock_native, 0},
};

Value clock_native(VM *vm, Value *args)
{
    (void)args;
    (void)vm;
    return NUMBER_VAL((double)clock() / CLOCKS_PER_SEC, __LINE__, 0);
}
Value typeof_native(VM *vm, Value *args)
{
    (void)vm;
    char *type = value_typeof(args[0]);
    return OBJ_VAL(new_string(type, strlen(type)), __LINE__, 0);
}
Value to_string_native(VM *vm, Value *args)
{
    (void)vm;
    Value x = args[0];
    switch (x.type)
    {
    case VAL_NUMBER:
    {
        char temp[50];
        snprintf(temp, sizeof(temp), "%.15g", AS_NUMBER(x));
        return OBJ_VAL(new_string(temp, strlen(temp)), 0, 0);
    }
    case VAL_BOOL:
    {
        bool y = AS_BOOL(x);
        char temp[7];
        sprintf(temp, "%s", y ? "true" : "false");
        return OBJ_VAL(new_string(temp, strlen(temp)), 0, 0);
    }
    case VAL_NULL:
    {
        char *temp = "null";
        return OBJ_VAL(new_string(temp, strlen(temp)), 0, 0);
    }
    case VAL_OBJ:
    {
        Obj *obj = AS_OBJ(x);
        switch (obj->type)
        {
        case OBJ_STRING:
        {
            return OBJ_VAL(new_string(AS_CSTRING(x), strlen(AS_CSTRING(x))), 0, 0);
        }
        case OBJ_FUNCTION:
        {
            ObjFunction *function = AS_FUNCTION(x);
            return OBJ_VAL(function->name, 0, 0);
        }
        case OBJ_NATIVE:
        {
            ObjNative *native = AS_NATIVE(x);
            return OBJ_VAL(native->name, 0, 0);
        }
        default:
            NOTREACHABLE;
        }
    }
    default:
        NOTREACHABLE;
    }
}
Value len_native(VM *vm, Value *args)
{
    Value x = args[0];
    if (!IS_STRING(x))
    {
        vm->row = x.row;
        vm->col = x.col;
        char *template = "Function len expect \"String\". But given type is \"%s\"";
        char *given_type = value_typeof(x);
        char *buffer = calloc(strlen(template) + strlen(given_type) + 1, sizeof(char));
        sprintf(buffer, template, given_type);
        vm->message = buffer;
        return NULL_VAL(0, 0);
    }
    return NUMBER_VAL(AS_STRING(x)->length, 0, 0);
}
void native_init(Table *globals)
{
    int len = sizeof(natives) / sizeof(natives[0]);
    for (int i = 0; i < len; i++)
    {
        NativeMapping native = natives[i];
        ObjString *string = new_string(native.name, strlen(native.name));
        ObjNative *function = new_native(native.function, native.arity);
        function->name = string;
        Value val = OBJ_VAL(function, 0, 0);
        table_set(globals, string, val);
    }
}