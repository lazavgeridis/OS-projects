/* port_master_utils.h */

#include <semaphore.h>
#include "SharedMem.h"

void park_small_medium(SharedMemory *, sem_t *, sem_t *);
void park_on_large_space(SharedMemory *, long, long, long, sem_t *, sem_t *, sem_t *, sem_t *);

void park_small_large(SharedMemory *, sem_t *, sem_t *);
void park_on_medium_space(SharedMemory *, long, long, long, sem_t *, sem_t *, sem_t *);

void park_medium_large(SharedMemory *, sem_t *, sem_t *);
void park_on_small_space(SharedMemory *, long, long, long, sem_t *, sem_t *, sem_t *);

void park_large(SharedMemory *, sem_t *, sem_t *);



long find_min(long *);
