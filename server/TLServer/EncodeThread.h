#ifndef ENCODETHREAD_H
#define ENCODETHREAD_H

#include "const.h"
#include "SharedQueue.h"

void start_encode_h265(SharedQueue *shared_queue, TaskScheduler *scheduler);
void start_encode_h264(SharedQueue *shared_queue, TaskScheduler *scheduler);

#endif
