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
//
// Macros for dealing with memory alignment.
#ifndef PEGASUS_UTIL_ALIGNMENT_H
#define PEGASUS_UTIL_ALIGNMENT_H

// Round down 'x' to the nearest 'align' boundary
#define PEGASUS_ALIGN_DOWN(x, align) ((x) & (~(align) + 1))

// Round up 'x' to the nearest 'align' boundary
#define PEGASUS_ALIGN_UP(x, align) (((x) + ((align) - 1)) & (~(align) + 1))

#endif
