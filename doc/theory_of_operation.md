# Theory of operation

## Capturing PI call trace

`dpcpp_trace` uses XPTI to intercept SYCL runtime calls to Plugin Interface and
captures API call arguments. The `record` command inserts a few environment
variables with the help of `execve` call:

- `XPTI_TRACE_ENABLE=1` to enable tracing inside DPC++ runtime.
- `XPTI_FRAMEWORK_DISPATCHER=libxptifw.so` to specify the event dispatcher.
- `XPTI_SUBSCRIBERS=libplugin_record.so` to specify the subscriber library,
  which does most of the work.

The subscriber library listens to `sycl.pi.debug` stream, and records all
arguments into a binary file, which format is described below.

To learn more about XPTI, please, refer to
[xpti](https://github.com/intel/llvm/blob/sycl/xpti/doc/SYCL_Tracing_Implementation.md)
and [xptifw](https://github.com/intel/llvm/blob/sycl/xptifw/doc/XPTI_Framework.md)
documentation.

### Binary trace file format

Each `.pi_trace` file is composed of PI call records. Each record has the
following format:
```
uint32_t Size - record size in bytes
<size bytes of data> - record data in Protobuf format
```

Protobuf schema for PI API calls is described [here](https://github.com/alexbatashev/dpcpp_trace/blob/main/tools/schemas/api_call.proto).

### Handling string arguments

Some PI APIs accept C-style strings as input parameters. In that case the whole
string with terminating `\0` is stored in the arguments data, and the size of
the data block will be equal to `sizeof` of other arguments + length of the
string.

### Handling multithreading

`dpcpp_trace` records traces per each thread. To be able to reliably
distinguish between thread, the tool injects a library, that intercepts
`pthread_create` function and assigns each thread a unique name. The first
thread is always named `main`. On thread creation, newly thread is assigned name
in format `<current_thread_name>_n`, where `n` is the index number of the
thread, started by current thread. For each thread a file with thread name is
created. When printing, `dpcpp_trace` tool can either show traces per thread,
or sort records by API call time.

### Device images

Device images are dumped to the output directory on the first call to
`piextDeviceSelectBinary`. The following naming scheme is used:
```
<index of binary image>_<target_arch>.<extension>
```

Where extension is one of the following:
- `PI_DEVICE_BINARY_TYPE_SPIRV` -> `.spv`
- `PI_DEVICE_BINARY_TYPE_NATIVE` -> `.bin`
- `PI_DEVICE_BINARY_TYPE_LLVMIR_BITCODE` -> `.bc`
- `PI_DEVICE_BINARY_TYPE_NONE` -> `.none`

TBD describe device image descriptors.

## Replaying PI traces

### Emulating plugins

Plugin emulation relies on SYCL runtime's plugin override capability.
`dpcpp_trace` provides a PI plugin implementation, that reads trace files and
responses PI calls with that info.

Trace files are read sequentially, so, it is essential for the program to have
the same environment and command line arguments.

### Emulating DPC++ runtime
TBD

## Packed reproducers

### Recording (Linux)
On Linux `dpcpp_trace` uses `ptrace` function to intercept `openat` system call.
Each opened file is recorded into `files_config.json` file.

### Packing
When trace is recorded, a separate `dpcpp_trace pack` run can be performed to
copy application executable and its dependencies into trace directory. A special
`replay_file_map.json` file is composed to provide mapping between original
files and their packed versions. On Linux, paths, that start with `/dev`,
`/sys`, or `/proc` are skipped.

### Compression
`dpcpp_trace` uses [zstd](https://facebook.github.io/zstd/) for compression
algorithm. The following schema describes the contents of compressed files:

```
uint8_t version -- compression algorithm version
<<< repeated >>>
uint8_t kind -- record kind: 0 for directory, 1 for file
uint64_t size -- size of the file (or direcrory) name
char filename[size] -- file name
<<< files only >>>
uint64_t length -- length of compressed file data
char data[length] -- compressed file
```

### Replaying
When `dpcpp_trace replay` is invoked, the tool checks for `replay_file_map.json`
file and sets up hooks for system calls. If the original file is found in the
map, it will be redirected inside trace directory. It is illegal to pass command
line arguments to `replay` if trace contains packed reproducer.
