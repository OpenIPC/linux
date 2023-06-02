#include <asm/neon.h>
#include <asm/unaligned.h>
#include <linux/module.h>
extern void float_test(long long s64input, long long *ps64output);
void floatTest(long long s64input, long long *ps64output)
{
    kernel_neon_begin();
    float_test(s64input, ps64output);
    kernel_neon_end();
}
EXPORT_SYMBOL_GPL(floatTest);
