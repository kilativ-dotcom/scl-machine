/*
* This source file is part of an OSTIS project. For the latest info, see http://ostis.net
* Distributed under the MIT License
* (See accompanying file COPYING.MIT or copy at http://opensource.org/licenses/MIT)
*/
#include "sc-memory/sc_debug.hpp"

#include "LRUScAddrSet.hpp"

namespace inference
{
LRUScAddrSet::LRUScAddrSet(std::size_t maxSize)
  : maxSize(maxSize)
{
  // todo(kilativ-dotcom): do smth if maxSize is 0
  if (maxSize <= 0)
    SC_THROW_EXCEPTION(utils::ExceptionInvalidParams, "LRUScAddrSet: size cannot be <= 0(" << maxSize << " is passed)");
}

bool LRUScAddrSet::contains(ScAddr const & element)
{
  auto const & mapIterator = mapElements.find(element);
  if (mapIterator != mapElements.cend())
  {
    moveElementToHead(mapIterator->second);
    return true;
  }
  return false;
}

void LRUScAddrSet::moveElementToHead(std::list<ScAddr>::iterator const & element)
{
  if (element != recentlyUsedElements.end())
  {
    auto it = mapElements.find(*element);
    if (it != mapElements.end() && it->second == element)
    {
      auto elementCopy = *element;
      recentlyUsedElements.erase(element);
      recentlyUsedElements.push_front(elementCopy);
      it->second = recentlyUsedElements.begin();
    }
  }
}

void LRUScAddrSet::insert(ScAddr const & element)
{
  auto const & currentElement = mapElements.find(element);
  if (currentElement != mapElements.cend())
    moveElementToHead(currentElement->second);
  else
    insertNewElement(element);
}

void LRUScAddrSet::insertNewElement(ScAddr const & element)
{
  recentlyUsedElements.push_front(element);
  mapElements[element] = recentlyUsedElements.begin();
  if (recentlyUsedElements.size() > maxSize)
    removeLRUElement();
}

void LRUScAddrSet::removeLRUElement()
{
  auto const & removedElement = std::prev(this->recentlyUsedElements.end());
  this->mapElements.erase(*removedElement);
  this->recentlyUsedElements.pop_back();
}

void LRUScAddrSet::clear()
{
  this->mapElements.clear();
  this->recentlyUsedElements.clear();
}
}  // namespace inference
