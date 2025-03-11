#include "table.h"
#include "helper.h"

Obj *object_allocate(size_t size, ObjType type)
{
    Obj *object = calloc(1, size);
    object->type = type;
    return object;
}

bool is_obj_type(Value value, ObjType type)
{
    return IS_OBJ(value) && AS_OBJ(value)->type == type;
}
void object_print(Value value)
{
    switch (OBJ_TYPE(value))
    {
    case OBJ_STRING:
        fprintf(stdout, "%s", AS_CSTRING(value));
        break;
    case OBJ_FUNCTION:
        fprintf(stdout, "<fn %s>", AS_FUNCTION(value)->name->chars);
        break;
    default:
        NOTREACHABLE;
    }
}
ObjString *allocate_string(size_t length, uint32_t hash)
{
    ObjString *string = (ObjString *)object_allocate(sizeof(ObjString) + (length + 1) * sizeof(char), OBJ_STRING);
    string->length = length;
    string->hash = hash;
    return string;
}
ObjString *new_string(char *chars, size_t length)
{
    uint32_t hash = 0;
    if (chars != NULL)
        hash = table_hash_string(chars, length);

    ObjString *string = allocate_string(length, hash);
    string->length = length;
    if (chars)
        strcpy(string->chars, chars);
    return string;
}

ObjString *cstr_to_objstr(char *chars)
{
    size_t chars_len = strlen(chars);
    ObjString *string = new_string(chars, chars_len);
    return string;
}

ObjFunction *new_function()
{
    ObjFunction *function = ALLOCATE_OBJ(ObjFunction, OBJ_FUNCTION);
    function->chunk = init_chunk();
    return function;
}