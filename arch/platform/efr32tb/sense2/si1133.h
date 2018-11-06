#ifndef SI1133_H
#define SI1133_H

int si1133_init(void);
int32_t si1133_get_data(uint32_t *lux, uint32_t *uv);

#endif
