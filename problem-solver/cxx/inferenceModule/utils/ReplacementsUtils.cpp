/*
 * This source file is part of an OSTIS project. For the latest info, see http://ostis.net
 * Distributed under the MIT License
 * (See accompanying file COPYING.MIT or copy at http://opensource.org/licenses/MIT)
 */

#include "ReplacementsUtils.hpp"
#include "sc-memory/kpm/sc_agent.hpp"

Replacements inference::ReplacementsUtils::intersectReplacements(
    Replacements const & first,
    Replacements const & second)
{
  Replacements result;
  size_t resultSize = 0;
  ScAddrHashSet firstKeys;
  getKeySet(first, firstKeys);
  ScAddrHashSet secondKeys;
  getKeySet(second, secondKeys);
  ScAddrHashSet commonKeysSet = getCommonKeys(firstKeys, secondKeys);
  size_t firstAmountOfColumns = getColumnsAmount(first);
  size_t secondAmountOfColumns = getColumnsAmount(second);

  if (firstAmountOfColumns == 0)
    return copyReplacements(second);
  if (secondAmountOfColumns == 0)
    return copyReplacements(first);

  for (size_t columnIndexInFirst = 0; columnIndexInFirst < firstAmountOfColumns; ++columnIndexInFirst)
  {
    for (size_t columnIndexInSecond = 0; columnIndexInSecond < secondAmountOfColumns; ++columnIndexInSecond)
    {
      bool commonPartsAreIdentical = true;
      for (ScAddr const & commonKey : commonKeysSet)
      {
        if (first.find(commonKey)->second[columnIndexInFirst] != second.find(commonKey)->second[columnIndexInSecond])
          commonPartsAreIdentical = false;
      }
      if (commonPartsAreIdentical)
      {
        for (ScAddr const & firstKey : firstKeys)
          result[firstKey].push_back(first.find(firstKey)->second[columnIndexInFirst]);
        for (ScAddr const & secondKey : secondKeys)
        {
          if (result[secondKey].size() == resultSize)
            result[secondKey].push_back(second.find(secondKey)->second[columnIndexInSecond]);
        }
        ++resultSize;
      }
    }
  }
  removeDuplicateColumns(result);
  return result;
}

Replacements inference::ReplacementsUtils::uniteReplacements(Replacements const & first, Replacements const & second)
{
  Replacements result;
  size_t resultSize = 0;
  ScAddrHashSet firstKeys;
  getKeySet(first, firstKeys);
  ScAddrHashSet secondKeys;
  getKeySet(second, secondKeys);
  ScAddrHashSet commonKeysSet = getCommonKeys(firstKeys, secondKeys);
  size_t firstAmountOfColumns = getColumnsAmount(first);
  size_t secondAmountOfColumns = getColumnsAmount(second);

  if (firstAmountOfColumns == 0)
    return copyReplacements(second);
  if (secondAmountOfColumns == 0)
    return copyReplacements(first);

  for (size_t columnIndexInFirst = 0; columnIndexInFirst < firstAmountOfColumns; ++columnIndexInFirst)
  {
    for (size_t columnIndexInSecond = 0; columnIndexInSecond < secondAmountOfColumns; ++columnIndexInSecond)
    {
      for (ScAddr const & firstKey : firstKeys)
        result[firstKey].push_back(first.find(firstKey)->second[columnIndexInFirst]);
      for (ScAddr const & secondKey : secondKeys)
      {
        if (result[secondKey].size() == resultSize)
          result[secondKey].push_back(second.find(secondKey)->second[columnIndexInSecond]);
      }
      ++resultSize;
      for (ScAddr const & secondKey : secondKeys)
        result[secondKey].push_back(second.find(secondKey)->second[columnIndexInSecond]);
      for (ScAddr const & firstKey : firstKeys)
      {
        if (result[firstKey].size() == resultSize)
          result[firstKey].push_back(first.find(firstKey)->second[columnIndexInFirst]);
      }
      ++resultSize;
    }
  }
  removeDuplicateColumns(result);
  return result;
}

void inference::ReplacementsUtils::getKeySet(Replacements const & map, ScAddrHashSet & keySet)
{
  for (auto const & pair : map)
    keySet.insert(pair.first);
}

ScAddrHashSet inference::ReplacementsUtils::getCommonKeys(ScAddrHashSet const & first, ScAddrHashSet const & second)
{
  ScAddrHashSet result;
  for (ScAddr const & key : first)
  {
    if (second.find(key) != second.end())
      result.insert(key);
  }
  return result;
}

Replacements inference::ReplacementsUtils::copyReplacements(Replacements const & replacements)
{
  Replacements result;
  for (auto const & pair : replacements)
  {
    ScAddr const & key = pair.first;
    for (ScAddr const & value : replacements.find(key)->second)
      result[key].push_back(value);
  }
  return result;
}

/**
 * @brief The size of the all ScAddrVector of variables is the same (it is a matrix)
 * @param replacements to convert to vector<ScTemplateParams>
 * @return vector<ScTemplateParams> of converted replacements
 */
vector<ScTemplateParams> inference::ReplacementsUtils::getReplacementsToScTemplateParams(
    Replacements const & replacements)
{
  vector<ScTemplateParams> result;
  ScAddrHashSet keys;
  getKeySet(replacements, keys);
  if (keys.empty())
    return result;

  size_t columnsAmount = replacements.begin()->second.size();
  for (size_t columnIndex = 0; columnIndex < columnsAmount; ++columnIndex)
  {
    ScTemplateParams params;
    for (ScAddr const & key : keys)
      params.Add(key, replacements.find(key)->second[columnIndex]);
    result.push_back(params);
  }
  return result;
}

size_t inference::ReplacementsUtils::getColumnsAmount(Replacements const & replacements)
{
  return (replacements.empty() ? 0 : replacements.begin()->second.size());
}

void inference::ReplacementsUtils::removeDuplicateColumns(Replacements & replacements)
{
  size_t columnsAmount = getColumnsAmount(replacements);
  ScAddrHashSet keys;
  getKeySet(replacements, keys);
  if (keys.empty())
    return;
  std::unordered_map<ScAddr, ScAddr, ScAddrHashFunc<uint32_t>> column;
  for (int columnNumber = 0; columnNumber < columnsAmount; ++columnNumber)
  {
    for (auto const & key : keys)
      column[key] = replacements.find(key)->second[columnNumber];
    for (int comparedColumnNumber = columnNumber + 1; comparedColumnNumber < columnsAmount; ++comparedColumnNumber)
    {
      bool columnIsUnique = false;
      for (auto const & key : keys)
      {
        if (column[key] != replacements.find(key)->second[comparedColumnNumber])
        {
          columnIsUnique = true;
          break;
        }
      }
      if (!columnIsUnique)
      {
        for (auto const & key : keys)
        {
          ScAddrVector & replacementValues = replacements.find(key)->second;
          replacementValues.erase(replacementValues.begin() + comparedColumnNumber);
        }
        columnsAmount--;
        comparedColumnNumber--;
      }
    }
  }
}
