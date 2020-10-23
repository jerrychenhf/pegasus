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

#include <map>
#include <string>
#include <list>
#include <functional> 
#include <algorithm>

#ifndef __CONSISTENT_HASH_H__
#define __CONSISTENT_HASH_H__

template <typename T,
        typename Hash,
        typename Alloc = std::allocator<std::pair<const typename Hash::result_type,T > > >
class consistent_hash_map
{
public:
    typedef typename Hash::result_type size_type;
    typedef std::map<size_type,T,std::less<size_type>,Alloc> map_type;
    typedef typename map_type::value_type value_type;
    typedef value_type& reference;
    typedef const value_type& const_reference;
    typedef typename map_type::iterator iterator;
    typedef typename map_type::reverse_iterator reverse_iterator;
    typedef Alloc allocator_type;

public:
    consistent_hash_map() {
    }

    ~consistent_hash_map() {
    }
public:
    std::size_t size() const {
        return nodes_.size();
    }

    bool empty() const {
        return nodes_.empty();
    }

    std::pair<iterator,bool> insert(const T& node) {
        size_type hash = hasher_(node);
        return nodes_.insert(value_type(hash,node));
    }

    void erase(iterator it) {
        nodes_.erase(it);
    }

    std::size_t erase(const T& node) {
        size_type hash = hasher_(node);
        return nodes_.erase(hash);
    }

    iterator find(size_type hash) {
        if(nodes_.empty()) {
            return nodes_.end();
        }

        iterator it = nodes_.lower_bound(hash);

        if (it == nodes_.end()) {
            it = nodes_.begin();
        }

        return it;
    }

    iterator begin() { return nodes_.begin(); }
    iterator end() { return nodes_.end(); }
    reverse_iterator rbegin() { return nodes_.rbegin(); }
    reverse_iterator rend() { return nodes_.rend(); }
private:
    Hash hasher_;
    map_type nodes_;
};

#endif //__CONSISTENT_HASH_H__
