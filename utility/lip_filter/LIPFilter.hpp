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

#ifndef QUICKSTEP_UTILITY_LIP_FILTER_LIP_FILTER_HPP_
#define QUICKSTEP_UTILITY_LIP_FILTER_LIP_FILTER_HPP_

#include <cstddef>
#include <vector>

#include "catalog/CatalogTypedefs.hpp"
#include "storage/StorageBlockInfo.hpp"
#include "utility/Macros.hpp"

#include "glog/logging.h"

namespace quickstep {

class Type;
class ValueAccessor;

/** \addtogroup Utility
 *  @{
 */

enum class LIPFilterType {
  kBloomFilter,
  kExactFilter,
  kSingleIdentityHashFilter
};

class LIPFilter {
 public:
  LIPFilterType getType() const {
    return type_;
  }

  virtual void insertValueAccessor(ValueAccessor *accessor,
                                   const attribute_id attr_id,
                                   const Type *attr_type) = 0;

  virtual std::size_t filterBatch(ValueAccessor *accessor,
                                  const attribute_id attr_id,
                                  const bool is_attr_nullable,
                                  std::vector<tuple_id> *batch,
                                  const std::size_t batch_size) const = 0;

  virtual std::size_t onesCount() const = 0;

 protected:
  LIPFilter(const LIPFilterType &type)
      : type_(type) {
  }

 private:
  LIPFilterType type_;

  DISALLOW_COPY_AND_ASSIGN(LIPFilter);
};

/** @} */

}  // namespace quickstep

#endif  // QUICKSTEP_UTILITY_LIP_FILTER_LIP_FILTER_HPP_
