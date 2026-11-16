#include "common.h"



int c_clip3(int v, int i_min, int i_max)
{
    return ((v < i_min) ? i_min : ((v > i_max) ? i_max : v));
}



