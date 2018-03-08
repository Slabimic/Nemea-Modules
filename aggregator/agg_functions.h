//
// Created by slabimic on 24/02/18.
//

#include <unirec/unirec.h>

#ifndef AGGREGATOR_AGG_FUNCTIONS_H
#define AGGREGATOR_AGG_FUNCTIONS_H


void sum_int64(const void *src, void *dst);
void sum_int32(const void *src, void *dst);
void sum_int16(const void *src, void *dst);
void sum_int8(const void *src, void *dst);
void sum_uint64(const void *src, void *dst);
void sum_uint32(const void *src, void *dst);
void sum_uint16(const void *src, void *dst);
void sum_uint8(const void *src, void *dst);
void sum_float(const void *src, void *dst);
void sum_double(const void *src, void *dst);

void avg_int64(const void *src, void *dst);
void avg_int32(const void *src, void *dst);
void avg_int16(const void *src, void *dst);
void avg_int8(const void *src, void *dst);
void avg_uint64(const void *src, void *dst);
void avg_uint32(const void *src, void *dst);
void avg_uint16(const void *src, void *dst);
void avg_uint8(const void *src, void *dst);
void avg_float(const void *src, void *dst);
void avg_double(const void *src, void *dst);
/*
template <typename T>
void make_avg(void *src, uint32_t count);
*/
/*
 * Implementation in header file because of errors
 * aggregation_module-configuration.o: undefined reference to `void make_avg<unsigned char>(void*, unsigned int)'
*/
template <typename T>
void make_avg(void *src, uint32_t count)
{
   *((T*)src) /= count;
}

void nope(const void *src, void *dst);


#endif //AGGREGATOR_AGG_FUNCTIONS_H