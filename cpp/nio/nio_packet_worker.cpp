//
// Created by hujianzhe on 18-1-8.
//

#include "nio_packet_worker.h"

namespace Util {
static NioPacketWorker default_work;
NioPacketWorker* NioPacketWorker::defaultWorker(void) { return &default_work; }
}