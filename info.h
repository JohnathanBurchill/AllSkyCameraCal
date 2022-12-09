#ifndef _INFO_H
#define _INFO_H

#define QT(x) #x
#define STR(macro) QT(macro)

#include "main.h"

void usage(ProgramState *state, char *name);
void aboutASCC(void);

#endif // _INFO_H
