/*
 * Copyright (C) 2020-2021 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "shared/source/command_stream/aub_command_stream_receiver_hw.h"
#include "shared/source/command_stream/command_stream_receiver_hw.h"
#include "shared/source/command_stream/tbx_command_stream_receiver_hw.h"
#include "shared/source/helpers/populate_factory.h"
#include "shared/source/utilities/perf_counter.h"

#include "opencl/source/command_queue/command_queue_hw.h"
#include "opencl/source/device_queue/device_queue_hw.h"
#include "opencl/source/helpers/cl_hw_helper.h"
#include "opencl/source/mem_obj/buffer.h"
#include "opencl/source/mem_obj/image.h"
#include "opencl/source/sampler/sampler.h"

#include <type_traits>

namespace NEO {

typedef SKLFamily Family;

struct EnableOCLGen9 {
    EnableOCLGen9() {
        populateFactoryTable<AUBCommandStreamReceiverHw<Family>>();
        populateFactoryTable<BufferHw<Family>>();
        populateFactoryTable<ClHwHelperHw<Family>>();
        populateFactoryTable<CommandQueueHw<Family>>();
        populateFactoryTable<CommandStreamReceiverHw<Family>>();
        populateFactoryTable<DeviceQueueHw<Family>>();
        populateFactoryTable<ImageHw<Family>>();
        populateFactoryTable<SamplerHw<Family>>();
        populateFactoryTable<TbxCommandStreamReceiverHw<Family>>();
    }
};

static EnableOCLGen9 enable;
} // namespace NEO
