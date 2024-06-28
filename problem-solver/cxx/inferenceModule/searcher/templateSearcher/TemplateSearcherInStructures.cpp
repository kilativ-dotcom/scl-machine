/*
 * This source file is part of an OSTIS project. For the latest info, see http://ostis.net
 * Distributed under the MIT License
 * (See accompanying file COPYING.MIT or copy at http://opensource.org/licenses/MIT)
 */

#include "TemplateSearcherInStructures.hpp"

#include <memory>
#include <algorithm>

#include "sc-agents-common/utils/CommonUtils.hpp"
#include "sc-agents-common/utils/IteratorUtils.hpp"

#include "keynodes/InferenceKeynodes.hpp"

using namespace inference;

TemplateSearcherInStructures::TemplateSearcherInStructures(
    ScMemoryContext * context,
    ScAddrVector const & otherInputStructures)
  : TemplateSearcherAbstract(context)
{
  this->contentOfAllInputStructures = std::make_unique<LRUScAddrSet>(2*1000*1000);
  inputStructures = otherInputStructures;
}

TemplateSearcherInStructures::TemplateSearcherInStructures(ScMemoryContext * context)
  : TemplateSearcherInStructures(context, {})
{
}

void TemplateSearcherInStructures::searchTemplate(
    ScAddr const & templateAddr,
    ScTemplateParams const & templateParams,
    ScAddrHashSet const & variables,
    Replacements & result)
{
  ScTemplate searchTemplate;
  if (context->HelperBuildTemplate(searchTemplate, templateAddr, templateParams))
  {
    prepareBeforeSearch();
    if (context->HelperCheckEdge(
            InferenceKeynodes::concept_template_with_links, templateAddr, ScType::EdgeAccessConstPosPerm))
    {
      searchTemplateWithContent(searchTemplate, templateAddr, templateParams, result);
    }
    else
    {
      context->HelperSmartSearchTemplate(
          searchTemplate,
          [templateParams, &result, &variables, this](
              ScTemplateSearchResultItem const & item) -> ScTemplateSearchRequest {
            // Add search result item to the answer container
            ScAddr argument;
            for (ScAddr const & variable : variables)
            {
              if (item.Has(variable))
              {
                result[variable].push_back(item[variable]);
              }
              else if (templateParams.Get(variable, argument))
              {
                result[variable].push_back(argument);
              }
            }
            if (replacementsUsingType == ReplacementsUsingType::REPLACEMENTS_FIRST)
              return ScTemplateSearchRequest::STOP;
            else
              return ScTemplateSearchRequest::CONTINUE;
          },
          [this](ScAddr const & item) -> bool {
            // Filter result item belonging to any of the input structures
            return isValidElement(item);
          });
    }
  }
  else
  {
    throw std::runtime_error("Template is not built.");
  }
}

void TemplateSearcherInStructures::searchTemplateWithContent(
    ScTemplate const & searchTemplate,
    ScAddr const & templateAddr,
    ScTemplateParams const & templateParams,
    Replacements & result)
{
  ScAddrHashSet variables;
  getVariables(templateAddr, variables);
  std::map<std::string, std::string> linksContentMap = getTemplateLinksContent(templateAddr);

  context->HelperSearchTemplate(
      searchTemplate,
      [templateParams, &result, &variables, this](ScTemplateSearchResultItem const & item) -> ScTemplateSearchRequest {
        // Add search result item to the answer container
        for (ScAddr const & variable : variables)
        {
          ScAddr argument;
          if (item.Get(variable, argument) || templateParams.Get(variable, argument))
          {
            result[variable].push_back(argument);
          }
        }
        if (replacementsUsingType == ReplacementsUsingType::REPLACEMENTS_FIRST)
          return ScTemplateSearchRequest::STOP;
        else
          return ScTemplateSearchRequest::CONTINUE;
      },
      [&linksContentMap, this](ScTemplateSearchResultItem const & item) -> bool {
        // Filter result item by the same content and belonging to any of the input structures
        if (!isContentIdentical(item, linksContentMap))
          return false;
        for (size_t i = 0; i < item.Size(); i++)
        {
          ScAddr const & checkedElement = item[i];
          if (isValidElement(checkedElement) == SC_FALSE)
            return false;
        }
        return true;
      });
}

std::map<std::string, std::string> TemplateSearcherInStructures::getTemplateLinksContent(ScAddr const & templateAddr)
{
  std::map<std::string, std::string> linksContent;
  ScIterator3Ptr const & linksIterator =
      context->Iterator3(templateAddr, ScType::EdgeAccessConstPosPerm, ScType::Link);
  while (linksIterator->Next())
  {
    ScAddr const & linkAddr = linksIterator->Get(2);
    std::string stringContent;
    if (isValidElement(linkAddr))
    {
      context->GetLinkContent(linkAddr, stringContent);
      linksContent.emplace(to_string(linkAddr.Hash()), stringContent);
    }
  }

  return linksContent;
}

void TemplateSearcherInStructures::prepareBeforeSearch()
{
  this->contentOfAllInputStructures->clear();
//  if (replacementsUsingType == REPLACEMENTS_ALL)
//  {
//    SC_LOG_DEBUG("start input structures processing");
//    for (auto const & inputStructure : inputStructures)
//    {
//      ScAddrVector const & elements = utils::IteratorUtils::getAllWithType(context, inputStructure, ScType::Unknown);
//      contentOfAllInputStructures->insert(elements.cbegin(), elements.cend());
//    }
//    SC_LOG_DEBUG("input structures processed, found " << contentOfAllInputStructures->size() << " elements");
//  }
}

bool TemplateSearcherInStructures::isValidElement(ScAddr const & element) const
{
  if (replacementsUsingType == REPLACEMENTS_FIRST)
    return std::any_of(inputStructures.cbegin(), inputStructures.cend(), [&element, this](ScAddr const &inputStructure){
      return context->HelperCheckEdge(inputStructure, element, ScType::EdgeAccessConstPosPerm);
    });
  else
  {
    if (contentOfAllInputStructures->contains(element))
      return true;
    bool const inAny = std::any_of(inputStructures.cbegin(), inputStructures.cend(), [&element, this](ScAddr const &inputStructure){
      return context->HelperCheckEdge(inputStructure, element, ScType::EdgeAccessConstPosPerm);
    });
    if (inAny)
      contentOfAllInputStructures->insert(element);
    return inAny;
  }
}
