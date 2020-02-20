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

#ifndef LRU_CACHE_H
#define LRU_CACHE_H

#include <list>
#include <unordered_map>
#include <utility>

#include <boost/optional.hpp>
#include "dataset/cache_engine.h"
#include "cache/cache_region.h"
#include "dataset/cache_store.h"

// modified from boost LRU cache -> the boost cache supported only an
// ordered map.
namespace pegasus {
// a cache which evicts the least recently used item when it is full
template <class CacheEntryKey, class CacheRegion>
class LruCache {
 public:
  using key_type = CacheEntryKey;
  using value_type = CacheRegion;
  using list_type = std::list<key_type>;
  struct hasher {
    template <typename I>
    std::size_t operator()(const I& i) const {
      return i.Hash();
    }
  };
  using map_type =
      std::unordered_map<key_type, std::pair<value_type, typename list_type::iterator>,
                         hasher>;
                
  explicit LruCache(size_t capacity) : cache_capacity_(capacity) {}

  ~LruCache() {}

  size_t size() const { return map_.size(); }

  size_t capacity() const { return cache_capacity_; }

  bool empty() const { return map_.empty(); }

  bool contains(const key_type& key) { return map_.find(key) != map_.end(); }

  void insert(const key_type& key, const value_type& value, CacheStore* cache_store) {
    typename map_type::iterator i = map_.find(key);
    if (i == map_.end()) {
      // insert item into the cache, but first check if it is full
      if (size() >= cache_capacity_) {
        // cache is full, evict the least recently used item
        evict();
      }

      // insert the new item
      lru_list_.push_front(key);
      map_[key] = std::make_pair(value, lru_list_.begin());
      evict_map_[key] = cache_store;
    }
  }

  boost::optional<value_type> get(const key_type& key) {
    // lookup value in the cache
    typename map_type::iterator value_for_key = map_.find(key);
    if (value_for_key == map_.end()) {
      // value not in cache
      return boost::none;
    }

    // return the value, but first update its place in the most
    // recently used list
    typename list_type::iterator postition_in_lru_list = value_for_key->second.second;
    if (postition_in_lru_list != lru_list_.begin()) {
      // move item to the front of the most recently used list
      lru_list_.erase(postition_in_lru_list);
      lru_list_.push_front(key);

      // update iterator in map
      postition_in_lru_list = lru_list_.begin();
      const value_type& value = value_for_key->second.first;
      map_[key] = std::make_pair(value, postition_in_lru_list);

      // return the value
      return value;
    } else {
      // the item is already at the front of the most recently
      // used list so just return it
      return value_for_key->second.first;
    }
  }

  void clear() {
    map_.clear();
    lru_list_.clear();
  }

 private:
  void evict() {
    // evict item from the end of most recently used list
    key_type evict_key = lru_list_.back();
    // TODO concurrently free and access
    value_type evict_value = map_.find(evict_key)->second.first;
    CacheStore* cache_store = evict_map_.find(evict_key)->second;
   // cache_store->Free(evict_value.get());

    typename list_type::iterator i = --lru_list_.end();
    map_.erase(*i);
    lru_list_.erase(i);
  }

 private:
  map_type map_;
  std::unordered_map<key_type, CacheStore*, hasher> evict_map_;
  list_type lru_list_;
  size_t cache_capacity_;
};
}  // namespace gandiva
#endif  // LRU_CACHE_H
