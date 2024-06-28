/*
* This source file is part of an OSTIS project. For the latest info, see http://ostis.net
* Distributed under the MIT License
* (See accompanying file COPYING.MIT or copy at http://opensource.org/licenses/MIT)
 */


#include "sc_test.hpp"

#include "model/LRUScAddrSet.hpp"

namespace LRUMapTest
{
class LRUScAddrSetTest : public ScMemoryTest
{
protected:
  void SetUp() override
  {
    ScMemoryTest::SetUp();
    this->one = m_ctx->HelperResolveSystemIdtf("one", ScType::NodeConst);
    this->two = m_ctx->HelperResolveSystemIdtf("two", ScType::NodeConst);
    this->three = m_ctx->HelperResolveSystemIdtf("three", ScType::NodeConst);
    this->four = m_ctx->HelperResolveSystemIdtf("four", ScType::NodeConst);
    this->five = m_ctx->HelperResolveSystemIdtf("five", ScType::NodeConst);
    this->six = m_ctx->HelperResolveSystemIdtf("six", ScType::NodeConst);
  }

  ScAddr one;
  ScAddr two;
  ScAddr three;
  ScAddr four;
  ScAddr five;
  ScAddr six;
};

TEST_F(LRUScAddrSetTest, cacheSize3)
{
  inference::LRUScAddrSet cache(3);
  cache.insert(one);
  cache.insert(two);
  cache.insert(three);
  EXPECT_TRUE(cache.contains(one));
  EXPECT_TRUE(cache.contains(two));
  EXPECT_TRUE(cache.contains(three));
  EXPECT_FALSE(cache.contains(four));
}

TEST_F(LRUScAddrSetTest, invalidCacheSize)
{
  EXPECT_THROW((inference::LRUScAddrSet(0)), utils::ExceptionInvalidParams);
}

TEST_F(LRUScAddrSetTest, elementsAreBeingDeletedFromCache)
{
  inference::LRUScAddrSet cache(3);
  cache.insert(one);
  cache.insert(two);
  cache.insert(three);
  EXPECT_TRUE(cache.contains(one));
  EXPECT_TRUE(cache.contains(two));
  EXPECT_TRUE(cache.contains(three));
  cache.insert(four);
  cache.insert(five);
  cache.insert(six);
  EXPECT_FALSE(cache.contains(one));
  EXPECT_FALSE(cache.contains(two));
  EXPECT_FALSE(cache.contains(three));
  EXPECT_TRUE(cache.contains(four));
  EXPECT_TRUE(cache.contains(five));
  EXPECT_TRUE(cache.contains(six));
}

TEST_F(LRUScAddrSetTest, LRUelementsAreBeingDeletedFromCacheFirst)
{
  inference::LRUScAddrSet cache(3);
  cache.insert(one);
  cache.insert(two);
  cache.insert(three);
  EXPECT_TRUE(cache.contains(one));
  EXPECT_TRUE(cache.contains(two));
  EXPECT_TRUE(cache.contains(three));
  cache.insert(four);
  EXPECT_FALSE(cache.contains(one));
  EXPECT_TRUE(cache.contains(four));
  cache.insert(five);
  EXPECT_FALSE(cache.contains(two));
  EXPECT_TRUE(cache.contains(five));
  cache.insert(six);
  EXPECT_FALSE(cache.contains(three));
  EXPECT_TRUE(cache.contains(six));
}

TEST_F(LRUScAddrSetTest, elementAccessMovesItToHead)
{
  inference::LRUScAddrSet cache(3);
  cache.insert(one);
  cache.insert(two);
  cache.insert(three);
  EXPECT_TRUE(cache.contains(one));
  EXPECT_TRUE(cache.contains(two));
  EXPECT_TRUE(cache.contains(three));
  // this contains moves "one" to head so "two" will be deleted if cache size is too big
  EXPECT_TRUE(cache.contains(one));
  cache.insert(four);
  EXPECT_FALSE(cache.contains(two));
  EXPECT_TRUE(cache.contains(three));
  EXPECT_TRUE(cache.contains(one));
  EXPECT_TRUE(cache.contains(four));
  cache.insert(five);
  EXPECT_FALSE(cache.contains(three));
  EXPECT_TRUE(cache.contains(one));
  EXPECT_TRUE(cache.contains(four));
  EXPECT_TRUE(cache.contains(five));
  cache.insert(six);
  EXPECT_FALSE(cache.contains(one));
  EXPECT_TRUE(cache.contains(four));
  EXPECT_TRUE(cache.contains(five));
  EXPECT_TRUE(cache.contains(six));
}

TEST_F(LRUScAddrSetTest, elementUpdateMovesItToHead)
{
  inference::LRUScAddrSet cache(3);
  cache.insert(one);
  cache.insert(two);
  cache.insert(three);
  EXPECT_TRUE(cache.contains(one));
  EXPECT_TRUE(cache.contains(two));
  EXPECT_TRUE(cache.contains(three));
  // this insert moves "one" to head so "two" will be deleted if cache size is too big
  cache.insert(one);
  cache.insert(four);
  EXPECT_FALSE(cache.contains(two));
  EXPECT_TRUE(cache.contains(three));
  EXPECT_TRUE(cache.contains(one));
  EXPECT_TRUE(cache.contains(four));
  cache.insert(five);
  EXPECT_FALSE(cache.contains(three));
  EXPECT_TRUE(cache.contains(one));
  EXPECT_TRUE(cache.contains(four));
  EXPECT_TRUE(cache.contains(five));
  cache.insert(six);
  EXPECT_FALSE(cache.contains(one));
  EXPECT_TRUE(cache.contains(four));
  EXPECT_TRUE(cache.contains(five));
  EXPECT_TRUE(cache.contains(six));
}
}
