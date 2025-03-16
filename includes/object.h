#ifndef OBJECT_H
#define OBJECT_H
#include <string.h>
#include "value.h"
#include "chunk.h"

typedef enum
{
    OBJ_STRING,
    OBJ_FUNCTION,
} ObjType;

struct Obj
{
    ObjType type;
};

#define OBJ_TYPE(value) (AS_OBJ(value)->type)
#define ALLOCATE_OBJ(type, objectType) \
    (type *)object_allocate(sizeof(type), objectType)

Obj *object_allocate(size_t size, ObjType type);
bool is_obj_type(Value value, ObjType type);
void object_print(Value value);

typedef struct ObjString ObjString;
struct ObjString
{
    Obj obj;
    size_t length;
    uint32_t hash;
    char chars[];
};
typedef struct
{
    Obj obj;
    int arity;
    Chunk *chunk;
    Values *values;
    ObjString *name;
} ObjFunction;

#define AS_STRING(value) ((ObjString *)AS_OBJ(value))
#define AS_CSTRING(value) (((ObjString *)AS_OBJ(value))->chars)
#define AS_FUNCTION(value) ((ObjFunction *)AS_OBJ(value))

#define IS_STRING(value) is_obj_type(value, OBJ_STRING)
#define IS_FUNCTION(value) is_obj_type(value, OBJ_FUNCTION)

ObjString *cstr_to_objstr(char *chars);
ObjString *new_string(char *chars, size_t length);
ObjString *allocate_string(size_t length, uint32_t hash);

ObjFunction *new_function();
#endif