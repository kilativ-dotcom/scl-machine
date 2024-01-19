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
  SC_LOG_DEBUG(
      "intersecting " << first.size() << "x" << getColumnsAmount(first) << " and " << second.size() << "x"
                      << getColumnsAmount(second));
  Replacements result;
  std::vector<std::pair<size_t, size_t>> firstSecondPairs;
  ScAddrHashSet firstKeys;
  getKeySet(first, firstKeys);
  ScAddrHashSet secondKeys;
  getKeySet(second, secondKeys);
  size_t firstAmountOfColumns = getColumnsAmount(first);
  size_t secondAmountOfColumns = getColumnsAmount(second);

  if (firstAmountOfColumns == 0)
    return copyReplacements(second);
  if (secondAmountOfColumns == 0)
    return copyReplacements(first);

  ScAddrHashSet commonKeysSet = getCommonKeys(firstKeys, secondKeys);

  
  ReplacementsHashes firstHashes = calculateHashesForCommonKeys(first, commonKeysSet);
  ReplacementsHashes secondHashes = calculateHashesForCommonKeys(second, commonKeysSet);
  for (auto const & firstHashPair : firstHashes)
  {
    auto const & secondHashPairIterator = secondHashes.find(firstHashPair.first);
    if (secondHashPairIterator == secondHashes.cend())
      continue;
    for (auto const & columnIndexInFirst : firstHashPair.second)
    {
      for (auto const & columnIndexInSecond : secondHashPairIterator->second)
      {
        bool commonPartsAreIdentical = true;
        for (ScAddr const & commonKey : commonKeysSet)
        {
          if (first.find(commonKey)->second[columnIndexInFirst] != second.find(commonKey)->second[columnIndexInSecond])
          {
            commonPartsAreIdentical = false;
            break;
          }
        }
        if (commonPartsAreIdentical)
          firstSecondPairs.emplace_back(columnIndexInFirst, columnIndexInSecond);
      }
    }
  }

  for (ScAddr const & firstKey : firstKeys)
    result[firstKey].reserve(firstSecondPairs.size());
  for (ScAddr const & secondKey : secondKeys)
  {
    if (!commonKeysSet.count(secondKey))
      result[secondKey].reserve(firstSecondPairs.size());
  }

  for (auto const & firstSecondPair : firstSecondPairs)
  {
    for (ScAddr const & firstKey : firstKeys)
      result[firstKey].push_back(first.find(firstKey)->second[firstSecondPair.first]);
    for (ScAddr const & secondKey : secondKeys)
    {
      if (!commonKeysSet.count(secondKey))
        result[secondKey].push_back(second.find(secondKey)->second[firstSecondPair.second]);
    }
  }
  removeDuplicateColumns(result);
  SC_LOG_DEBUG("returning intersected " << result.size() << "x" << getColumnsAmount(result));
  return result;
}

Replacements inference::ReplacementsUtils::subtractReplacements(Replacements const & first, Replacements const & second)
{
  SC_LOG_DEBUG(
      "subtracting " << first.size() << "x" << getColumnsAmount(first) << " and " << second.size() << "x"
                     << getColumnsAmount(second));
  Replacements result;
  ScAddrHashSet firstKeys;
  getKeySet(first, firstKeys);
  ScAddrHashSet secondKeys;
  getKeySet(second, secondKeys);
  size_t firstAmountOfColumns = getColumnsAmount(first);
  size_t secondAmountOfColumns = getColumnsAmount(second);
  std::vector<size_t> firstColumns;
  firstColumns.reserve(firstAmountOfColumns);

  if (firstAmountOfColumns == 0 || secondAmountOfColumns == 0)
    return copyReplacements(first);

  ScAddrHashSet commonKeysSet = getCommonKeys(firstKeys, secondKeys);

  if (commonKeysSet.empty())
    return copyReplacements(first);

  ReplacementsHashes firstHashes = calculateHashesForCommonKeys(first, commonKeysSet);
  ReplacementsHashes secondHashes = calculateHashesForCommonKeys(second, commonKeysSet);
  for (auto const & firstHashPair : firstHashes)
  {
    auto const & secondHashPairIterator = secondHashes.find(firstHashPair.first);
    if (secondHashPairIterator == secondHashes.cend())
    {
      firstColumns.insert(firstColumns.end(), firstHashPair.second.cbegin(), firstHashPair.second.cend());
      continue;
    }
    for (auto const & columnIndexInFirst : firstHashPair.second)
    {
      bool hasPairWithSimilarValues = false;
      for (auto const & columnIndexInSecond : secondHashPairIterator->second)
      {
        bool hasDifferentValuesForAnyKey = false;
        for (ScAddr const & commonKey : commonKeysSet)
        {
          if (first.find(commonKey)->second[columnIndexInFirst] != second.find(commonKey)->second[columnIndexInSecond])
          {
            hasDifferentValuesForAnyKey = true;
            break;
          }
        }
        hasPairWithSimilarValues = !hasDifferentValuesForAnyKey;
        if (hasPairWithSimilarValues)
          break;
      }
      if (!hasPairWithSimilarValues)
        firstColumns.push_back(columnIndexInFirst);
    }
  }

  for (ScAddr const & firstKey : firstKeys)
    result[firstKey].reserve(firstColumns.size());

  for (auto const & firstColumn : firstColumns)
  {
    for (ScAddr const & firstKey : firstKeys)
      result[firstKey].push_back(first.find(firstKey)->second[firstColumn]);
  }
  removeDuplicateColumns(result);
  SC_LOG_DEBUG("returning subtracted " << result.size() << "x" << getColumnsAmount(result));
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
  SC_LOG_DEBUG("ReplacementsUtils::removeDuplicateColumns for " << getColumnsAmount(replacements) << " columns");
  ScAddrHashSet keys;
  getKeySet(replacements, keys);
  if (keys.empty())
    return;
  ReplacementsHashes const & replacementsHashes = calculateHashesForCommonKeys(replacements, keys);
  std::unordered_map<ScAddr, ScAddr, ScAddrHashFunc<uint32_t>> column;
  std::set<size_t> columnsToRemove;
  for (auto const & replacementsHash : replacementsHashes)
  {
    auto const & columnsForHash = replacementsHash.second;
    if (columnsForHash.size() > 1)
    {
      for (size_t firstColumnIndex = 0; firstColumnIndex < columnsForHash.size(); ++firstColumnIndex)
      {
        for (auto const & key : keys)
          column[key] = replacements.find(key)->second[columnsForHash[firstColumnIndex]];
        for (size_t comparedColumnIndex = firstColumnIndex + 1; comparedColumnIndex < columnsForHash.size();
             ++comparedColumnIndex)
        {
          bool columnIsUnique = false;
          for (auto const & key : keys)
          {
            if (column[key] != replacements.find(key)->second[columnsForHash[comparedColumnIndex]])
            {
              columnIsUnique = true;
              break;
            }
          }
          if (!columnIsUnique)
            columnsToRemove.insert(columnsForHash[comparedColumnIndex]);
        }
      }
    }
  }
  for (auto columnToRemove = columnsToRemove.crbegin(); columnToRemove != columnsToRemove.crend(); columnToRemove++)
  {
    for (auto const & key : keys)
    {
      ScAddrVector & replacementValues = replacements.find(key)->second;
      replacementValues.erase(replacementValues.begin() + static_cast<long>(*columnToRemove));
    }
  }

  SC_LOG_DEBUG("ReplacementsUtils::removeDuplicateColumns finish with " << getColumnsAmount(replacements) << " columns");
}

ReplacementsHashes inference::ReplacementsUtils::calculateHashesForCommonKeys(
    Replacements const & replacements,
    ScAddrHashSet const & commonKeys)
{
  ReplacementsHashes replacementsHashes;
  size_t const columnsAmount = ReplacementsUtils::getColumnsAmount(replacements);
  size_t const commonKeysAmount = commonKeys.empty() ? 1 : commonKeys.size();
  std::vector<size_t> primes = {7, 13, 17, 19, 31, 41, 43};
  for (size_t columnNumber = 0; columnNumber < columnsAmount; ++columnNumber)
  {
    int primeInd = 0;
    size_t offsets = 0;
    for (auto const & commonKey : commonKeys)
      offsets += replacements.find(commonKey)->second.at(columnNumber).GetRealAddr().offset *
                 primes.at(primeInd++ % primes.size());
    replacementsHashes[offsets / commonKeysAmount].push_back(columnNumber);
  }
  return replacementsHashes;
}

Replacements inference::ReplacementsUtils::removeRows(Replacements const & replacements, ScAddrHashSet & keysToRemove)
{
  Replacements result;
  for (auto const & replacement : replacements)
  {
    if (keysToRemove.count(replacement.first))
      continue;
    result[replacement.first].assign(replacement.second.cbegin(), replacement.second.cend());
  }
  return result;
}
