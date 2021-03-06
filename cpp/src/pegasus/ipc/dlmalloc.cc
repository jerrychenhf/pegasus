// Licensed to the Apache Software Foundation (ASF) under one
// or more contributor license agreements.  See the NOTICE file
// distributed with this work for additional information
// regarding copyright ownership.  The ASF licenses this file
// to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance
// with the License.  You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.  See the License for the
// specific language governing permissions and limitations
// under the License.

#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <cerrno>
#include <string>
#include <vector>
#include "common/logging.h"
#include "ipc/malloc.h"
#include "ipc/store_config.h"

namespace pegasus {

void* fake_mmap(size_t);
int fake_munmap(void*, int64_t);

#define MMAP(s) fake_mmap(s)
#define MUNMAP(a, s) fake_munmap(a, s)
#define DIRECT_MMAP(s) fake_mmap(s)
#define DIRECT_MUNMAP(a, s) fake_munmap(a, s)
#define USE_DL_PREFIX
#define HAVE_MORECORE 0
#define DEFAULT_MMAP_THRESHOLD MAX_SIZE_T
#define DEFAULT_GRANULARITY ((size_t)128U * 1024U)
#define USE_LOCKS 1

#include "ipc/thirdparty/dlmalloc.c"  // NOLINT

#undef MMAP
#undef MUNMAP
#undef DIRECT_MMAP
#undef DIRECT_MUNMAP
#undef USE_DL_PREFIX
#undef HAVE_MORECORE
#undef DEFAULT_GRANULARITY

// dlmalloc.c defined DEBUG which will conflict with LOG(DEBUG).
#ifdef DEBUG
#undef DEBUG
#endif

constexpr int GRANULARITY_MULTIPLIER = 2;

static void* pointer_advance(void* p, ptrdiff_t n) { return (unsigned char*)p + n; }

static void* pointer_retreat(void* p, ptrdiff_t n) { return (unsigned char*)p - n; }

// Create a buffer. This is creating a temporary file and then
// immediately unlinking it so we do not leave traces in the system.
int create_buffer(int64_t size) {
  int fd;
  std::string file_template = store_config->directory;
#ifdef _WIN32
  if (!CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE,
                         (DWORD)((uint64_t)size >> (CHAR_BIT * sizeof(DWORD))),
                         (DWORD)(uint64_t)size, NULL)) {
    fd = -1;
  }
#else
  file_template += "/pegasusXXXXXX";
  std::vector<char> file_name(file_template.begin(), file_template.end());
  file_name.push_back('\0');
  fd = mkstemp(&file_name[0]);
  if (fd < 0) {
    LOG(FATAL) << "create_buffer failed to open file " << &file_name[0];
    return -1;
  }
  // Immediately unlink the file so we do not leave traces in the system.
  if (unlink(&file_name[0]) != 0) {
    LOG(FATAL) << "failed to unlink file " << &file_name[0];
    return -1;
  }
  if (!store_config->hugepages_enabled) {
    // Increase the size of the file to the desired size. This seems not to be
    // needed for files that are backed by the huge page fs, see also
    // http://www.mail-archive.com/kvm-devel@lists.sourceforge.net/msg14737.html
    if (ftruncate(fd, (off_t)size) != 0) {
      LOG(FATAL) << "failed to ftruncate file " << &file_name[0];
      return -1;
    }
  }
#endif
  return fd;
}

void* fake_mmap(size_t size) {
  // Add kMmapRegionsGap so that the returned pointer is deliberately not
  // page-aligned. This ensures that the segments of memory returned by
  // fake_mmap are never contiguous.
  size += kMmapRegionsGap;

  int fd = create_buffer(size);
  CHECK(fd >= 0) << "Failed to create buffer during mmap";
  // MAP_POPULATE can be used to pre-populate the page tables for this memory region
  // which avoids work when accessing the pages later. However it causes long pauses
  // when mmapping the files. Only supported on Linux.
  void* pointer = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  if (pointer == MAP_FAILED) {
    LOG(ERROR) << "mmap failed with error: " << std::strerror(errno);
    if (errno == ENOMEM && store_config->hugepages_enabled) {
      LOG(ERROR)
          << "  (this probably means you have to increase /proc/sys/vm/nr_hugepages)";
    }
    return pointer;
  }

  // Increase dlmalloc's allocation granularity directly.
  mparams.granularity *= GRANULARITY_MULTIPLIER;

  AddMmapRecord(pointer, fd, size);

  // We lie to dlmalloc about where mapped memory actually lives.
  pointer = pointer_advance(pointer, kMmapRegionsGap);
  
  //TO DO: change to DEBUG level
  LOG(INFO) << pointer << " = fake_mmap(" << size << ")";
  return pointer;
}

int fake_munmap(void* addr, int64_t size) {
  //TO DO: change to DEBUG level
  LOG(INFO) << "fake_munmap(" << addr << ", " << size << ")";
  addr = pointer_retreat(addr, kMmapRegionsGap);
  size += kMmapRegionsGap;

  int fd = GetMappedFd(addr, size);
  if (fd < 0) {
    // Reject requests to munmap that don't directly match previous
    // calls to mmap, to prevent dlmalloc from trimming.
    return -1;
  }

  int r = munmap(addr, size);
  if (r == 0) {
    close(fd);
  }
  
  RemoveMmapRecord(addr);
  return r;
}

void SetMallocGranularity(int value) { change_mparam(M_GRANULARITY, value); }

}  // namespace pegasus
