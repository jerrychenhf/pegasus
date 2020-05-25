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

#ifndef PEGASUS_STORE_CONFIG_H
#define PEGASUS_STORE_CONFIG_H

#include <stddef.h>
#include <string>
#include <cstring>

namespace pegasus {
  
/// The  store configuration
struct StoreConfig {
  /// Boolean flag indicating whether to start the object store with hugepages
  /// support enabled. Huge pages are substantially larger than normal memory
  /// pages (e.g. 2MB or 1GB instead of 4KB) and using them can reduce
  /// bookkeeping overhead from the OS.
  bool hugepages_enabled;
  
  /// A (platform-dependent) directory where to create the memory-backed file.
  std::string directory;
};  

/// Globally accessible reference to store configuration.
/// TODO(pcm): This can be avoided with some refactoring of existing code
/// by making it possible to pass a context object through dlmalloc.

extern const StoreConfig* store_config;

}  // namespace pegasus

#endif  // PEGASUS_STORE_CONFIG_H
