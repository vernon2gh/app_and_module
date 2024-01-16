#ifndef __TRACE_H
#define __TRACE_H

#include <stdbool.h>

void trace_configure(int pid, const char *function);
void trace_exit(void);

void trace_on(void);
void trace_off(void);

#endif /* __TRACE_H */
