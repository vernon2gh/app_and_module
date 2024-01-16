#ifndef __DYNAMIC_DEBUG_H
#define __DYNAMIC_DEBUG_H

void dynamic_debug_start(void);
void dynamic_debug_end(void);

void dynamic_debug_control(const char *buf);

#endif /* __DYNAMIC_DEBUG_H */
