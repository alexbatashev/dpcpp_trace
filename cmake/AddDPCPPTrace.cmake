macro(set_dpcpp_trace_common_options target_name)
  if (NOT OpenCL_INCLUDE_DIRS)
    find_package(OpenCL REQUIRED)
  endif()

  target_include_directories(${target_name} PRIVATE
    ${OpenCL_INCLUDE_DIRS}
    ${PROJECT_SOURCE_DIR}/include
    ${INTEL_LLVM_BINARY_PATH}/include/sycl
    ${INTEL_LLVM_SOURCE_PATH}/sycl/tools/xpti_helpers
    ${INTEL_LLVM_SOURCE_PATH}/xpti/include)
  target_link_directories(${target_name} PRIVATE ${INTEL_LLVM_BINARY_PATH}/lib)

  target_link_libraries(${target_name} PRIVATE xptifw)
  target_compile_definitions(${target_name} PRIVATE -DCL_TARGET_OPENCL_VERSION=300)
endmacro()

function(add_dpcpp_trace_library target_name kind)
  add_library(${target_name} ${kind} ${ARGN})

  set_dpcpp_trace_common_options(${target_name})

  target_compile_options(${target_name} PRIVATE -fPIC)
endfunction()

function(add_dpcpp_trace_executable target_name)
  add_executable(${target_name} ${ARGN})

  set_dpcpp_trace_common_options(${target_name})
endfunction()