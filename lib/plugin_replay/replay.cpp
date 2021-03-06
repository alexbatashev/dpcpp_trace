#include "api_call.pb.h"
#include "constants.hpp"

#include <CL/sycl/detail/pi.hpp>

#include <array>
#include <cstdint>
#include <cstring>
#include <exception>
#include <filesystem>
#include <fstream>
#include <map>
#include <memory>
#include <pthread.h>

using namespace sycl::detail;

size_t GOffset = 0;
thread_local std::ifstream GTrace;

std::map<pi_kernel, pi_program> GKernelProgramMap;
std::map<pi_mem, pi_context> GMemContextMap;

static void ensureTraceOpened() {
  if (!GTrace.is_open()) {
    std::filesystem::path traceDir{getenv(kTracePathEnvVar)};
    std::array<char, 1024> buf;
    pthread_getname_np(pthread_self(), buf.data(), buf.size());
    std::string filename{buf.data()};
    filename += kPiTraceExt;
    auto traceFile = traceDir / filename;

    GTrace = std::ifstream(traceFile, std::ios::binary);
  }
}

static std::string funcIdToString(uint32_t funcId) {
  switch (static_cast<PiApiKind>(funcId)) {
#define _PI_API(api) \
    case PiApiKind::api: \
      return #api;
    default:
      return "<unknown>";
#include <CL/sycl/detail/pi.def>
#undef _PI_API
  }
}

void dieIfUnexpected(uint32_t funcId, PiApiKind expected) {
  if (funcId != static_cast<uint32_t>(expected)) {
    std::cerr << "Unexpected PI call: got " << funcIdToString(funcId) << " ("
              << funcId << ") ";
    std::cerr << " expected "
              << funcIdToString(static_cast<uint32_t>(expected));
    std::cerr << "\n";
    exit(-1);
  }
}

void replayGetInfo(dpcpp_trace::APICall &record, void *paramValue,
                   size_t *retSize) {
  char *ptr = paramValue ? static_cast<char *>(paramValue)
                         : reinterpret_cast<char *>(retSize);
  std::uninitialized_copy(record.small_outputs(0).begin(),
                          record.small_outputs(0).end(), ptr);
}

extern "C" {

dpcpp_trace::APICall getNextRecord(std::ifstream &is) {
  uint32_t size;
  is.read(reinterpret_cast<char *>(&size), sizeof(uint32_t));
  std::string rawData;
  rawData.resize(size + 1);
  is.read(rawData.data(), size);

  dpcpp_trace::APICall call;
  call.ParseFromString(rawData);

  return call;
}

pi_result piPlatformsGet(pi_uint32 numEntries, pi_platform *platforms,
                         pi_uint32 *numPlatforms) {
  ensureTraceOpened();
  auto record = getNextRecord(GTrace);

  dieIfUnexpected(record.function_id(), PiApiKind::piPlatformsGet);

  if (numPlatforms != nullptr) {
    *numPlatforms =
        *reinterpret_cast<const pi_uint32 *>(record.small_outputs(0).data());
  }

  if (numEntries > 0 && platforms != nullptr) {
    for (size_t i = 0; i < numEntries; i++) {
      platforms[i] = reinterpret_cast<pi_platform>(new int{1});
    }
  }

  return static_cast<pi_result>(record.return_value());
}

pi_result piPlatformGetInfo(pi_platform platform, pi_platform_info param_name,
                            size_t param_value_size, void *param_value,
                            size_t *param_value_size_ret) {
  ensureTraceOpened();
  auto record = getNextRecord(GTrace);

  dieIfUnexpected(record.function_id(), PiApiKind::piPlatformGetInfo);

  replayGetInfo(record, param_value, param_value_size_ret);

  return static_cast<pi_result>(record.return_value());
}

pi_result piDevicesGet(pi_platform platform, pi_device_type type,
                       pi_uint32 numEntries, pi_device *devs,
                       pi_uint32 *numDevices) {
  ensureTraceOpened();
  auto record = getNextRecord(GTrace);

  dieIfUnexpected(record.function_id(), PiApiKind::piDevicesGet);

  if (numDevices != nullptr) {
    *numDevices =
        *reinterpret_cast<const pi_uint32 *>(record.small_outputs(0).data());
  }

  if (numEntries > 0 && devs != nullptr) {
    for (size_t i = 0; i < numEntries; i++) {
      devs[i] = reinterpret_cast<pi_device>(new int{1});
    }
  }

  return static_cast<pi_result>(record.return_value());
}

pi_result piDeviceGetInfo(pi_device device, pi_device_info param_name,
                          size_t param_value_size, void *param_value,
                          size_t *param_value_size_ret) {
  ensureTraceOpened();
  auto record = getNextRecord(GTrace);

  dieIfUnexpected(record.function_id(), PiApiKind::piDeviceGetInfo);

  replayGetInfo(record, param_value, param_value_size_ret);

  return static_cast<pi_result>(record.return_value());
}

pi_result piDeviceRetain(pi_device) {
  ensureTraceOpened();
  auto record = getNextRecord(GTrace);

  dieIfUnexpected(record.function_id(), PiApiKind::piDeviceRetain);
  return static_cast<pi_result>(record.return_value());
}

pi_result piDeviceRelease(pi_device) {
  ensureTraceOpened();
  auto record = getNextRecord(GTrace);

  dieIfUnexpected(record.function_id(), PiApiKind::piDeviceRelease);
  return static_cast<pi_result>(record.return_value());
}

pi_result piContextCreate(const pi_context_properties *properties,
                          pi_uint32 num_devices, const pi_device *devices,
                          void (*pfn_notify)(const char *errinfo,
                                             const void *private_info,
                                             size_t cb, void *user_data),
                          void *user_data, pi_context *ret_context) {
  ensureTraceOpened();
  auto record = getNextRecord(GTrace);

  dieIfUnexpected(record.function_id(), PiApiKind::piContextCreate);
  *ret_context = reinterpret_cast<pi_context>(new int{1});
  return static_cast<pi_result>(record.return_value());
}

pi_result piContextGetInfo(pi_context context, pi_context_info param_name,
                           size_t param_value_size, void *param_value,
                           size_t *param_value_size_ret) {
  ensureTraceOpened();
  auto record = getNextRecord(GTrace);

  dieIfUnexpected(record.function_id(), PiApiKind::piContextGetInfo);

  replayGetInfo(record, param_value, param_value_size_ret);

  return static_cast<pi_result>(record.return_value());
}

pi_result piContextRelease(pi_context) {
  ensureTraceOpened();
  auto record = getNextRecord(GTrace);

  dieIfUnexpected(record.function_id(), PiApiKind::piContextRelease);
  return static_cast<pi_result>(record.return_value());
}

pi_result piContextRetain(pi_context) {
  ensureTraceOpened();
  auto record = getNextRecord(GTrace);

  dieIfUnexpected(record.function_id(), PiApiKind::piContextRetain);
  return static_cast<pi_result>(record.return_value());
}

pi_result piQueueCreate(pi_context context, pi_device device,
                        pi_queue_properties properties, pi_queue *queue) {
  ensureTraceOpened();
  auto record = getNextRecord(GTrace);

  dieIfUnexpected(record.function_id(), PiApiKind::piQueueCreate);
  *queue = reinterpret_cast<pi_queue>(new int{1});
  return static_cast<pi_result>(record.return_value());
}

pi_result piQueueGetInfo(pi_queue command_queue, pi_queue_info param_name,
                         size_t param_value_size, void *param_value,
                         size_t *param_value_size_ret) {
  ensureTraceOpened();
  auto record = getNextRecord(GTrace);

  dieIfUnexpected(record.function_id(), PiApiKind::piQueueGetInfo);

  replayGetInfo(record, param_value, param_value_size_ret);

  return static_cast<pi_result>(record.return_value());
}

pi_result piQueueRetain(pi_queue) {
  ensureTraceOpened();
  auto record = getNextRecord(GTrace);

  dieIfUnexpected(record.function_id(), PiApiKind::piQueueRetain);
  return static_cast<pi_result>(record.return_value());
}

pi_result piQueueRelease(pi_queue command_queue) {
  ensureTraceOpened();
  auto record = getNextRecord(GTrace);

  dieIfUnexpected(record.function_id(), PiApiKind::piQueueRelease);
  return static_cast<pi_result>(record.return_value());
}

pi_result piQueueFinish(pi_queue command_queue) {
  ensureTraceOpened();
  auto record = getNextRecord(GTrace);

  dieIfUnexpected(record.function_id(), PiApiKind::piQueueFinish);
  return static_cast<pi_result>(record.return_value());
}

pi_result piMemBufferCreate(pi_context context, pi_mem_flags flags, size_t size,
                            void *host_ptr, pi_mem *ret_mem,
                            const pi_mem_properties *properties) {
  ensureTraceOpened();
  auto record = getNextRecord(GTrace);

  dieIfUnexpected(record.function_id(), PiApiKind::piMemBufferCreate);
  *ret_mem = reinterpret_cast<pi_mem>(new int{1});
  GMemContextMap[*ret_mem] = context;
  return static_cast<pi_result>(record.return_value());
}

pi_result piMemGetInfo(pi_mem mem, cl_mem_info param_name,
                       size_t param_value_size, void *param_value,
                       size_t *param_value_size_ret) {
  ensureTraceOpened();
  auto record = getNextRecord(GTrace);

  dieIfUnexpected(record.function_id(), PiApiKind::piMemGetInfo);

  replayGetInfo(record, param_value, param_value_size_ret);

  if (param_name == CL_MEM_CONTEXT) {
    *static_cast<pi_context *>(param_value) = GMemContextMap[mem];
  }

  return static_cast<pi_result>(record.return_value());
}

pi_result piMemImageGetInfo(pi_mem image, pi_image_info param_name,
                            size_t param_value_size, void *param_value,
                            size_t *param_value_size_ret) {
  ensureTraceOpened();
  auto record = getNextRecord(GTrace);

  dieIfUnexpected(record.function_id(), PiApiKind::piMemImageGetInfo);

  replayGetInfo(record, param_value, param_value_size_ret);

  return static_cast<pi_result>(record.return_value());
}

pi_result piMemRetain(pi_mem mem) {
  ensureTraceOpened();
  auto record = getNextRecord(GTrace);

  dieIfUnexpected(record.function_id(), PiApiKind::piMemImageGetInfo);

  return static_cast<pi_result>(record.return_value());
}

pi_result piextDeviceSelectBinary(pi_device device, pi_device_binary *binaries,
                                  pi_uint32 num_binaries,
                                  pi_uint32 *selected_binary_ind) {
  ensureTraceOpened();
  auto record = getNextRecord(GTrace);

  dieIfUnexpected(record.function_id(), PiApiKind::piextDeviceSelectBinary);
  *selected_binary_ind =
      *reinterpret_cast<const pi_uint32 *>(record.small_outputs(0).data());

  return static_cast<pi_result>(record.return_value());
}

pi_result piProgramCreateWithBinary(
    pi_context context, pi_uint32 num_devices, const pi_device *device_list,
    const size_t *lengths, const unsigned char **binaries,
    size_t num_metadata_entries, const pi_device_binary_property *metadata,
    pi_int32 *binary_status, pi_program *ret_program) {
  ensureTraceOpened();
  auto record = getNextRecord(GTrace);

  dieIfUnexpected(record.function_id(), PiApiKind::piProgramCreateWithBinary);
  *ret_program = reinterpret_cast<pi_program>(new int{1});
  return static_cast<pi_result>(record.return_value());
}

pi_result piProgramCreate(pi_context context, const void *il, size_t length,
                          pi_program *ret_program) {
  ensureTraceOpened();
  auto record = getNextRecord(GTrace);

  dieIfUnexpected(record.function_id(), PiApiKind::piProgramCreate);
  *ret_program = reinterpret_cast<pi_program>(new int{1});
  return static_cast<pi_result>(record.return_value());
}

pi_result piProgramBuild(pi_program program, pi_uint32 num_devices,
                         const pi_device *device_list, const char *options,
                         void (*pfn_notify)(pi_program program,
                                            void *user_data),
                         void *user_data) {
  ensureTraceOpened();
  auto record = getNextRecord(GTrace);

  dieIfUnexpected(record.function_id(), PiApiKind::piProgramBuild);
  return static_cast<pi_result>(record.return_value());
}

pi_result piProgramGetInfo(pi_program program, pi_program_info param_name,
                           size_t param_value_size, void *param_value,
                           size_t *param_value_size_ret) {
  ensureTraceOpened();
  auto record = getNextRecord(GTrace);

  dieIfUnexpected(record.function_id(), PiApiKind::piProgramGetInfo);

  replayGetInfo(record, param_value, param_value_size_ret);

  return static_cast<pi_result>(record.return_value());
}

pi_result piProgramCompile(
    pi_program program, pi_uint32 num_devices, const pi_device *device_list,
    const char *options, pi_uint32 num_input_headers,
    const pi_program *input_headers, const char **header_include_names,
    void (*pfn_notify)(pi_program program, void *user_data), void *user_data) {
  ensureTraceOpened();
  auto record = getNextRecord(GTrace);

  dieIfUnexpected(record.function_id(), PiApiKind::piProgramCompile);

  return static_cast<pi_result>(record.return_value());
}

pi_result piProgramLink(pi_context context, pi_uint32 num_devices,
                        const pi_device *device_list, const char *options,
                        pi_uint32 num_input_programs,
                        const pi_program *input_programs,
                        void (*pfn_notify)(pi_program program, void *user_data),
                        void *user_data, pi_program *ret_program) {
  ensureTraceOpened();
  auto record = getNextRecord(GTrace);

  dieIfUnexpected(record.function_id(), PiApiKind::piProgramLink);

  return static_cast<pi_result>(record.return_value());
}

pi_result piProgramRetain(pi_program program) {
  ensureTraceOpened();
  auto record = getNextRecord(GTrace);

  dieIfUnexpected(record.function_id(), PiApiKind::piProgramRetain);

  return static_cast<pi_result>(record.return_value());
}

pi_result piextProgramSetSpecializationConstant(pi_program prog,
                                                pi_uint32 spec_id,
                                                size_t spec_size,
                                                const void *spec_value) {
  ensureTraceOpened();
  auto record = getNextRecord(GTrace);

  dieIfUnexpected(record.function_id(),
                  PiApiKind::piextProgramSetSpecializationConstant);

  return static_cast<pi_result>(record.return_value());
}

pi_result piKernelCreate(pi_program program, const char *kernel_name,
                         pi_kernel *ret_kernel) {
  ensureTraceOpened();
  auto record = getNextRecord(GTrace);

  dieIfUnexpected(record.function_id(), PiApiKind::piKernelCreate);
  *ret_kernel = reinterpret_cast<pi_kernel>(new int{1});
  GKernelProgramMap[*ret_kernel] = program;
  return static_cast<pi_result>(record.return_value());
}

pi_result piKernelSetExecInfo(pi_kernel kernel, pi_kernel_exec_info value_name,
                              size_t param_value_size,
                              const void *param_value) {
  ensureTraceOpened();
  auto record = getNextRecord(GTrace);

  dieIfUnexpected(record.function_id(), PiApiKind::piKernelSetExecInfo);
  return static_cast<pi_result>(record.return_value());
}

pi_result piKernelGetInfo(pi_kernel kernel, pi_kernel_info param_name,
                          size_t param_value_size, void *param_value,
                          size_t *param_value_size_ret) {
  ensureTraceOpened();
  auto record = getNextRecord(GTrace);

  dieIfUnexpected(record.function_id(), PiApiKind::piKernelGetInfo);

  replayGetInfo(record, param_value, param_value_size_ret);

  if (param_name == PI_KERNEL_INFO_PROGRAM && param_value) {
    *static_cast<pi_program *>(param_value) = GKernelProgramMap[kernel];
  }

  return static_cast<pi_result>(record.return_value());
}

pi_result piKernelGetGroupInfo(pi_kernel kernel, pi_device device,
                               pi_kernel_group_info param_name,
                               size_t param_value_size, void *param_value,
                               size_t *param_value_size_ret) {
  ensureTraceOpened();
  auto record = getNextRecord(GTrace);

  dieIfUnexpected(record.function_id(), PiApiKind::piKernelGetGroupInfo);

  replayGetInfo(record, param_value, param_value_size_ret);

  return static_cast<pi_result>(record.return_value());
}

pi_result piextKernelSetArgMemObj(pi_kernel kernel, pi_uint32 arg_index,
                                  const pi_mem *arg_value) {
  ensureTraceOpened();
  auto record = getNextRecord(GTrace);

  dieIfUnexpected(record.function_id(), PiApiKind::piextKernelSetArgMemObj);
  return static_cast<pi_result>(record.return_value());
}

pi_result piKernelSetArg(pi_kernel kernel, pi_uint32 arg_index, size_t arg_size,
                         const void *arg_value) {
  ensureTraceOpened();
  auto record = getNextRecord(GTrace);

  dieIfUnexpected(record.function_id(), PiApiKind::piKernelSetArg);
  return static_cast<pi_result>(record.return_value());
}

pi_result piKernelRetain(pi_kernel kernel) {
  ensureTraceOpened();
  auto record = getNextRecord(GTrace);

  dieIfUnexpected(record.function_id(), PiApiKind::piKernelRetain);
  return static_cast<pi_result>(record.return_value());
}

__SYCL_EXPORT pi_result piextKernelSetArgPointer(pi_kernel kernel,
                                                 pi_uint32 arg_index,
                                                 size_t arg_size,
                                                 const void *arg_value) {
  ensureTraceOpened();
  auto record = getNextRecord(GTrace);

  dieIfUnexpected(record.function_id(), PiApiKind::piextKernelSetArgPointer);
  return static_cast<pi_result>(record.return_value());
}

pi_result piEnqueueKernelLaunch(
    pi_queue queue, pi_kernel kernel, pi_uint32 work_dim,
    const size_t *global_work_offset, const size_t *global_work_size,
    const size_t *local_work_size, pi_uint32 num_events_in_wait_list,
    const pi_event *event_wait_list, pi_event *event) {
  ensureTraceOpened();
  auto record = getNextRecord(GTrace);

  dieIfUnexpected(record.function_id(), PiApiKind::piEnqueueKernelLaunch);
  *event = reinterpret_cast<pi_event>(new int{1});
  return static_cast<pi_result>(record.return_value());
}

pi_result piEnqueueMemUnmap(pi_queue command_queue, pi_mem memobj,
                            void *mapped_ptr, pi_uint32 num_events_in_wait_list,
                            const pi_event *event_wait_list, pi_event *event) {
  ensureTraceOpened();
  auto record = getNextRecord(GTrace);

  dieIfUnexpected(record.function_id(), PiApiKind::piEnqueueMemUnmap);
  *event = reinterpret_cast<pi_event>(new int{1});
  delete[] static_cast<char *>(mapped_ptr);
  return static_cast<pi_result>(record.return_value());
}

pi_result piEnqueueEventsWait(pi_queue command_queue,
                              pi_uint32 num_events_in_wait_list,
                              const pi_event *event_wait_list,
                              pi_event *event) {
  ensureTraceOpened();
  auto record = getNextRecord(GTrace);

  dieIfUnexpected(record.function_id(), PiApiKind::piEnqueueEventsWait);
  *event = reinterpret_cast<pi_event>(new int{1});
  return static_cast<pi_result>(record.return_value());
}

pi_result piEnqueueEventsWaitWithBarrier(pi_queue command_queue,
                                         pi_uint32 num_events_in_wait_list,
                                         const pi_event *event_wait_list,
                                         pi_event *event) {
  ensureTraceOpened();
  auto record = getNextRecord(GTrace);

  dieIfUnexpected(record.function_id(),
                  PiApiKind::piEnqueueEventsWaitWithBarrier);
  *event = reinterpret_cast<pi_event>(new int{1});
  return static_cast<pi_result>(record.return_value());
}

pi_result piEventsWait(pi_uint32 num_events, const pi_event *event_list) {
  ensureTraceOpened();
  auto record = getNextRecord(GTrace);

  dieIfUnexpected(record.function_id(), PiApiKind::piEventsWait);
  return static_cast<pi_result>(record.return_value());
}

pi_result piEventRelease(pi_event) {
  ensureTraceOpened();
  auto record = getNextRecord(GTrace);

  dieIfUnexpected(record.function_id(), PiApiKind::piEventRelease);
  return static_cast<pi_result>(record.return_value());
}

pi_result piMemRelease(pi_mem) {
  ensureTraceOpened();
  auto record = getNextRecord(GTrace);

  dieIfUnexpected(record.function_id(), PiApiKind::piMemRelease);
  return static_cast<pi_result>(record.return_value());
}

pi_result piProgramRelease(pi_program) {
  ensureTraceOpened();
  auto record = getNextRecord(GTrace);

  dieIfUnexpected(record.function_id(), PiApiKind::piProgramRelease);
  return static_cast<pi_result>(record.return_value());
}

pi_result piKernelRelease(pi_kernel) {
  ensureTraceOpened();
  auto record = getNextRecord(GTrace);

  dieIfUnexpected(record.function_id(), PiApiKind::piKernelRelease);
  return static_cast<pi_result>(record.return_value());
}

pi_result piEnqueueMemBufferMap(pi_queue command_queue, pi_mem buffer,
                                pi_bool blocking_map, pi_map_flags map_flags,
                                size_t offset, size_t size,
                                pi_uint32 num_events_in_wait_list,
                                const pi_event *event_wait_list,
                                pi_event *event, void **ret_map) {
  ensureTraceOpened();
  auto record = getNextRecord(GTrace);

  dieIfUnexpected(record.function_id(), PiApiKind::piEnqueueMemBufferMap);

  std::filesystem::path outDir{std::getenv(kTracePathEnvVar)};
  std::ifstream memIS{outDir / kBuffersPath / record.mem_obj_outputs(0)};
  size_t objSize;
  memIS.read(reinterpret_cast<char *>(&objSize), sizeof(size_t));

  auto memory = new char[objSize];

  memIS.read(memory, objSize);

  memIS.close();

  *ret_map = memory;
  *event = reinterpret_cast<pi_event>(new int{1});

  return static_cast<pi_result>(record.return_value());
}

pi_result piEnqueueMemBufferRead(pi_queue queue, pi_mem buffer,
                                 pi_bool blocking_read, size_t offset,
                                 size_t size, void *ptr,
                                 pi_uint32 num_events_in_wait_list,
                                 const pi_event *event_wait_list,
                                 pi_event *event) {
  ensureTraceOpened();
  auto record = getNextRecord(GTrace);

  dieIfUnexpected(record.function_id(), PiApiKind::piEnqueueMemBufferRead);

  std::filesystem::path outDir{std::getenv(kTracePathEnvVar)};
  std::ifstream memIS{outDir / kBuffersPath / record.mem_obj_outputs(0)};
  size_t objSize;
  memIS.read(reinterpret_cast<char *>(&objSize), sizeof(size_t));

  memIS.read(static_cast<char *>(ptr), size);
  memIS.close();

  *event = reinterpret_cast<pi_event>(new int{1});

  return static_cast<pi_result>(record.return_value());
}

pi_result piextUSMEnqueueMemcpy(pi_queue queue, pi_bool blocking, void *dst_ptr,
                                const void *src_ptr, size_t size,
                                pi_uint32 num_events_in_waitlist,
                                const pi_event *events_waitlist,
                                pi_event *event) {
  ensureTraceOpened();
  auto record = getNextRecord(GTrace);

  dieIfUnexpected(record.function_id(), PiApiKind::piextUSMEnqueueMemcpy);

  if (record.mem_obj_outputs().size() > 0) {
    std::filesystem::path outDir{std::getenv(kTracePathEnvVar)};
    std::ifstream memIS{outDir / kBuffersPath / record.mem_obj_outputs(0)};
    size_t objSize;
    memIS.read(reinterpret_cast<char *>(&objSize), sizeof(size_t));

    memIS.read(static_cast<char *>(dst_ptr), size);
    memIS.close();
  }

  *event = reinterpret_cast<pi_event>(new int{1});

  return static_cast<pi_result>(record.return_value());
}

pi_result piextUSMHostAlloc(void **result_ptr, pi_context context,
                            pi_usm_mem_properties *properties, size_t size,
                            pi_uint32 alignment) {
  ensureTraceOpened();
  auto record = getNextRecord(GTrace);

  dieIfUnexpected(record.function_id(), PiApiKind::piextUSMHostAlloc);
  *result_ptr = static_cast<void *>(new char[size]);

  return static_cast<pi_result>(record.return_value());
}

pi_result piextUSMDeviceAlloc(void **result_ptr, pi_context context,
                              pi_device device,
                              pi_usm_mem_properties *properties, size_t size,
                              pi_uint32 alignment) {
  ensureTraceOpened();
  auto record = getNextRecord(GTrace);

  dieIfUnexpected(record.function_id(), PiApiKind::piextUSMDeviceAlloc);
  *result_ptr = static_cast<void *>(new char[size]);

  return static_cast<pi_result>(record.return_value());
}

pi_result piextUSMFree(pi_context context, void *ptr) {
  ensureTraceOpened();
  auto record = getNextRecord(GTrace);

  dieIfUnexpected(record.function_id(), PiApiKind::piextUSMFree);

  delete[] static_cast<char *>(ptr);

  return static_cast<pi_result>(record.return_value());
}

pi_result piextUSMEnqueueMemset(pi_queue queue, void *ptr, pi_int32 value,
                                size_t count, pi_uint32 num_events_in_waitlist,
                                const pi_event *events_waitlist,
                                pi_event *event) {
  ensureTraceOpened();
  auto record = getNextRecord(GTrace);

  dieIfUnexpected(record.function_id(), PiApiKind::piextUSMEnqueueMemset);

  *event = reinterpret_cast<pi_event>(new int{1});

  return static_cast<pi_result>(record.return_value());
}

pi_result piTearDown(void *) {
  return PI_SUCCESS;
}

pi_result piPluginInit(pi_plugin *PluginInit) {

#define _PI_CL(pi_api)                                                         \
  (PluginInit->PiFunctionTable).pi_api = (decltype(&::pi_api))(&pi_api);

  _PI_CL(piPlatformsGet);
  _PI_CL(piPlatformGetInfo);

  _PI_CL(piDevicesGet);
  _PI_CL(piDeviceGetInfo);
  _PI_CL(piDeviceRetain);
  _PI_CL(piDeviceRelease);
  _PI_CL(piextDeviceSelectBinary);
  _PI_CL(piContextCreate);
  _PI_CL(piContextGetInfo);
  _PI_CL(piContextRelease);
  _PI_CL(piContextRetain);

  _PI_CL(piQueueCreate);
  _PI_CL(piQueueGetInfo);
  _PI_CL(piQueueRetain);
  _PI_CL(piQueueRelease);
  _PI_CL(piQueueFinish);

  _PI_CL(piMemBufferCreate);
  _PI_CL(piMemRelease);
  _PI_CL(piMemGetInfo);
  _PI_CL(piMemImageGetInfo);
  _PI_CL(piMemRetain);

  _PI_CL(piProgramBuild);
  _PI_CL(piProgramCreateWithBinary);
  _PI_CL(piProgramCreate);
  _PI_CL(piProgramRelease);
  _PI_CL(piProgramRetain);
  _PI_CL(piProgramCompile);
  _PI_CL(piProgramLink);
  _PI_CL(piProgramGetInfo);
  _PI_CL(piextProgramSetSpecializationConstant);

  _PI_CL(piKernelCreate);
  _PI_CL(piKernelSetExecInfo);
  _PI_CL(piKernelGetInfo);
  _PI_CL(piKernelGetGroupInfo);
  _PI_CL(piextKernelSetArgMemObj);
  _PI_CL(piextKernelSetArgPointer);
  _PI_CL(piKernelSetArg);
  _PI_CL(piKernelRelease);
  _PI_CL(piKernelRetain);

  _PI_CL(piEnqueueKernelLaunch);
  _PI_CL(piEnqueueMemBufferMap);
  _PI_CL(piEnqueueMemBufferRead);
  _PI_CL(piEnqueueMemUnmap);
  _PI_CL(piEnqueueEventsWait);
  _PI_CL(piEnqueueEventsWaitWithBarrier);

  _PI_CL(piEventsWait);
  _PI_CL(piEventRelease);

  _PI_CL(piextUSMEnqueueMemcpy);
  _PI_CL(piextUSMEnqueueMemset);
  _PI_CL(piextUSMDeviceAlloc);
  _PI_CL(piextUSMHostAlloc);
  _PI_CL(piextUSMFree);

  _PI_CL(piTearDown);

  return PI_SUCCESS;
}
}
