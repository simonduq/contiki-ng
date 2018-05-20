#ifndef EFR32_DEF_H_
#define EFR32_DEF_H_

/*
 * The stdio.h that ships with the arm-gcc toolchain does this:
 *
 * int  _EXFUN(putchar, (int));
 * [...]
 * #define  putchar(x)  putc(x, stdout)
 *
 * This causes us a lot of trouble: For platforms using this toolchain, every
 * time we use putchar we need to first #undef putchar. What we do here is to
 * #undef putchar across the board. The resulting code will cause the linker
 * to search for a symbol named putchar and this allows us to use the
 * implementation under os/lib/dbg-io.
 *
 * This will fail if stdio.h is included before contiki.h, but it is common
 * practice to include contiki.h first
 */
#include <stdio.h>
#undef putchar

#endif /* EFR32_DEF_H_ */
