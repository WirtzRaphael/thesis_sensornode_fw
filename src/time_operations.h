#ifndef TIME_OPERATIONS_H_
#define TIME_OPERATIONS_H_

uint64_t time_operations_get_time_us(void);
uint64_t time_operations_get_time_diff_us(uint64_t time_start, uint64_t time_end);
uint64_t time_operations_get_time_diff_s(uint64_t time_start, uint64_t time_end);


void time_operations_datetime_from_now(datetime_t *t, int32_t sec);
void time_operations_datetime_print(datetime_t *t);

#endif /* TIME_OPERATIONS_H_ */