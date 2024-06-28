/*
* This source file is part of an OSTIS project. For the latest info, see http://ostis.net
* Distributed under the MIT License
* (See accompanying file COPYING.MIT or copy at http://opensource.org/licenses/MIT)
*/

#pragma once

#include <unordered_map>

#include "sc-memory/sc_addr.hpp"

namespace inference
{
class LRUScAddrSet
{
public:
  explicit LRUScAddrSet(std::size_t maxSize);

  bool contains(ScAddr const & element);

  void insert(ScAddr const & element);

  void clear();
private:
  std::list<ScAddr> recentlyUsedElements;
  std::unordered_map<ScAddr, std::list<ScAddr>::iterator, ScAddrHashFunc<sc_uint32>> mapElements;
  std::size_t const maxSize;

  void moveElementToHead(std::list<ScAddr>::iterator const & element);
  void insertNewElement(ScAddr const & element);
  void removeLRUElement();

};

}  // namespace inference
