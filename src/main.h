#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <limits.h>


#include "vector2d.h"
#include "vector3d.h"
#include "matrix3d.h"

extern int filtercreate(int fps);
extern int filterdestroy();
extern int filterstep(unsigned char *buffer, int w, int h, unsigned int color, char *text, int64_t framecount);
