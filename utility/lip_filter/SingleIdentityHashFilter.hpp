/**
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 **/

#ifndef QUICKSTEP_UTILITY_LIP_FILTER_SINGLE_IDENTITY_HASH_FILTER_HPP_
#define QUICKSTEP_UTILITY_LIP_FILTER_SINGLE_IDENTITY_HASH_FILTER_HPP_

#include <vector>

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <utility>
#include <vector>

#include "storage/StorageConstants.hpp"
#include "storage/ValueAccessor.hpp"
#include "storage/ValueAccessorUtil.hpp"
#include "types/Type.hpp"
#include "utility/lip_filter/LIPFilter.hpp"
#include "utility/Macros.hpp"

#include "glog/logging.h"

namespace quickstep {

/** \addtogroup Utility
 *  @{
 */

template <typename CppType>
class SingleIdentityHashFilter : public LIPFilter {
 public:
  SingleIdentityHashFilter(const std::size_t filter_cardinality)
      : LIPFilter(LIPFilterType::kSingleIdentityHashFilter),
        filter_cardinality_(filter_cardinality),
        bit_array_(GetByteSize(filter_cardinality)) {
    std::memset(bit_array_.data(),
                0x0,
                sizeof(std::atomic<std::uint8_t>) * GetByteSize(filter_cardinality));
  }

  void insertValueAccessor(ValueAccessor *accessor,
                           const attribute_id attr_id,
                           const Type *attr_type) override {
    InvokeOnAnyValueAccessor(
        accessor,
        [&](auto *accessor) -> void {  // NOLINT(build/c++11)
      if (attr_type->isNullable()) {
        insertValueAccessorInternal<true>(accessor, attr_id);
      } else {
        insertValueAccessorInternal<false>(accessor, attr_id);
      }
    });
  }

  std::size_t filterBatch(ValueAccessor *accessor,
                          const attribute_id attr_id,
                          const bool is_attr_nullable,
                          std::vector<tuple_id> *batch,
                          const std::size_t batch_size) const override {
    return InvokeOnAnyValueAccessor(
        accessor,
        [&](auto *accessor) -> std::size_t {  // NOLINT(build/c++11)
      if (is_attr_nullable) {
        return filterBatchInternal<true>(accessor, attr_id, batch, batch_size);
      } else {
        return filterBatchInternal<false>(accessor, attr_id, batch, batch_size);
      }
    });
  }


  /**
   * @brief Inserts a given value into the hash filter.
   *
   * @param key_begin A pointer to the value being inserted.
   */
  inline void insert(const void *key_begin) {
    const CppType hash = *reinterpret_cast<const CppType *>(key_begin) % filter_cardinality_;
    bit_array_[hash >> 3].fetch_or(1 << (hash & 0x7), std::memory_order_relaxed);
  }

  /**
   * @brief Test membership of a given value in the hash filter.
   *        If true is returned, then a value may or may not be present in the hash filter.
   *        If false is returned, a value is certainly not present in the hash filter.
   *
   * @param key_begin A pointer to the value being tested for membership.
   */
  inline bool contains(const void *key_begin) const {
    const CppType hash = *reinterpret_cast<const CppType *>(key_begin) % filter_cardinality_;
    return ((bit_array_[hash >> 3].load(std::memory_order_relaxed) & (1 << (hash & 0x7))) > 0);
  }

 private:
  inline static std::size_t GetByteSize(const std::size_t bit_size) {
    return (bit_size + 7) / 8;
  }

  template <bool is_attr_nullable, typename ValueAccessorT>
  inline void insertValueAccessorInternal(ValueAccessorT *accessor,
                                          const attribute_id attr_id) {
    accessor->beginIteration();
    while (accessor->next()) {
      const void *value = accessor->template getUntypedValue<is_attr_nullable>(attr_id);
      if (!is_attr_nullable || value != nullptr) {
        insert(value);
      }
    }
  }

  template <bool is_attr_nullable, typename ValueAccessorT>
  inline std::size_t filterBatchInternal(const ValueAccessorT *accessor,
                                         const attribute_id attr_id,
                                         std::vector<tuple_id> *batch,
                                         const std::size_t batch_size) const {
    std::size_t out_size = 0;
    for (std::size_t i = 0; i < batch_size; ++i) {
      const tuple_id tid = batch->at(i);
      const void *value =
          accessor->template getUntypedValueAtAbsolutePosition(attr_id, tid);
      if (contains(value)) {
        batch->at(out_size) = tid;
        ++out_size;
      }
    }
    return out_size;
  }


  std::size_t filter_cardinality_;
  alignas(kCacheLineBytes) std::vector<std::atomic<std::uint8_t>> bit_array_;

  DISALLOW_COPY_AND_ASSIGN(SingleIdentityHashFilter);
};

/** @} */

}  // namespace quickstep

#endif  // QUICKSTEP_UTILITY_LIP_FILTER_SINGLE_IDENTITY_HASH_FILTER_HPP_
