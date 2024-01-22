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
  this->contentOfAllInputStructures = std::make_unique<ScAddrHashSet>();
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
  searchWithoutContentResult = std::make_unique<ScTemplateSearchResult>();
  ScTemplate searchTemplate;
  if (context->HelperBuildTemplate(searchTemplate, templateAddr, templateParams))
  {
    this->contentOfAllInputStructures->clear();
    SC_LOG_DEBUG("start input structures processing");
    for (auto const & inputStructure : inputStructures)
    {
      ScAddrVector const & elements =
          utils::IteratorUtils::getAllWithType(context, inputStructure, ScType::Unknown);
      contentOfAllInputStructures->insert(elements.cbegin(), elements.cend());
    }
    SC_LOG_DEBUG("input structures processed, found " << contentOfAllInputStructures->size() << " elements");
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
            for (ScAddr const & variable : variables)
            {
              ScAddr argument;
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
            return contentOfAllInputStructures->count(item);
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
      [templateParams, &result, &variables](ScTemplateSearchResultItem const & item) -> ScTemplateSearchRequest {
        // Add search result item to the answer container
        for (ScAddr const & variable : variables)
        {
          ScAddr argument;
          if (item.Has(variable))
          {
            result[variable].push_back(item[variable]);
          }
          if (templateParams.Get(variable, argument))
          {
            result[variable].push_back(argument);
          }
        }
        return ScTemplateSearchRequest::STOP;
      },
      [&linksContentMap, this](ScTemplateSearchResultItem const & item) -> bool {
        // Filter result item by the same content and belonging to any of the input structures
        if (!isContentIdentical(item, linksContentMap))
          return false;
        for (size_t i = 0; i < item.Size(); i++)
        {
          if (!contentOfAllInputStructures->count(item[i]))
            return false;
        }
        return true;
      });
}

std::map<std::string, std::string> TemplateSearcherInStructures::getTemplateLinksContent(ScAddr const & templateAddr)
{
  std::map<std::string, std::string> linksContent;
  ScIterator3Ptr const & linksIterator = context->Iterator3(templateAddr, ScType::EdgeAccessConstPosPerm, ScType::Link);
  while (linksIterator->Next())
  {
    ScAddr const & linkAddr = linksIterator->Get(2);
    std::string stringContent;
    if (contentOfAllInputStructures->count(linkAddr))
    {
      context->GetLinkContent(linkAddr, stringContent);
      linksContent.emplace(to_string(linkAddr.Hash()), stringContent);
    }
  }

  return linksContent;
}
