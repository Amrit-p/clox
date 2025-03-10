#ifndef HELPER_H
#define HELPER_H
int helper_num_places(int n);
#include "log.h"
#define NOTIMPLEMENTED(message)                                                                 \
    do                                                                                \
    {                                                                                 \
        fprintf(stderr, BOLDRED"NOT IMPLEMENTD"RESET":"message" at %s:%d in %s\n", __FILE__, __LINE__, __func__); \
        exit(1);                                                                      \
    } while (0)
#define NOTREACHABLE                                                                  \
    do                                                                               \
    {                                                                                \
        fprintf(stderr, BOLDRED"NOT REACHABLE"RESET": %s:%d in %s\n", __FILE__, __LINE__, __func__); \
        exit(1);                                                                     \
    } while (0)
#endif