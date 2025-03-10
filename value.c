#include "value.h"
#include "helper.h"
#include "object.h"

#include <stdio.h>
#include <stdlib.h>

Values *init_values()
{
    Values *values = calloc(1, sizeof(Values));
    init_array(values);
    return values;
}

byte value_push(Values *values, Value data)
{
    array_push(values, data);
    return (byte)(array_size(values) - 1);
}
Value value_pop(Values *values)
{
    return array_pop(values);
}

void values_dump(Values *values, FILE *stream)
{
    for (size_t i = 0; i < array_size(values); i++)
    {
        fprintf(stream, "#%04zu  ", i);
        Value data = array_at(values, i);
        switch (data.type)
        {
        case VAL_NUMBER:
            fprintf(stream, "%g\n", AS_NUMBER(data));
            break;
        case VAL_BOOL:
            fprintf(stream, "%s\n", AS_BOOL(data) ? "true" : "false");
            break;
        case VAL_NULL:
            fprintf(stream, "null\n");
            break;
        case VAL_OBJ:
        {
            Obj *obj = AS_OBJ(data);

            switch (obj->type)
            {
            case OBJ_STRING:
                fprintf(stream, "\"%s\"\n", AS_CSTRING(data));
                break;
            default:
                fprintf(stream, "Object\n");
            }
            break;
        }
        default:
            NOTREACHABLE;
        }
    }
}
void values_free(Values *values)
{
    array_free(values);
    free(values);
}

void value_print(Value value, uint8_t new_ln)
{
    switch (value.type)
    {
    case VAL_BOOL:
        fprintf(stdout, AS_BOOL(value) ? "true" : "false");
        break;
    case VAL_NULL:
        fprintf(stdout, "null");
        break;
    case VAL_NUMBER:
        fprintf(stdout, "%g", AS_NUMBER(value));
        break;
    case VAL_OBJ:
        object_print(value);
        break;
    default:
        NOTREACHABLE;
    }
    if (new_ln)
        fprintf(stdout, "\n");
}
