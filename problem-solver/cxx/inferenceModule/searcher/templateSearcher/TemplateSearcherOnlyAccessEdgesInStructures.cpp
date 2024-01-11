#include "keynodes/InferenceKeynodes.hpp"

#include "sc-agents-common/utils/IteratorUtils.hpp"

#include "TemplateSearcherOnlyAccessEdgesInStructures.hpp"

namespace inference
{

TemplateSearcherOnlyAccessEdgesInStructures::TemplateSearcherOnlyAccessEdgesInStructures(
    ScMemoryContext * context,
    ScAddrVector const & otherInputStructures)
  : TemplateSearcherInStructures(context, otherInputStructures)
{
  this->contentOfAllInputStructures = std::make_unique<ScAddrHashSet>();
}

TemplateSearcherOnlyAccessEdgesInStructures::TemplateSearcherOnlyAccessEdgesInStructures(ScMemoryContext * context)
  : TemplateSearcherOnlyAccessEdgesInStructures(context, {})
{
}
void TemplateSearcherOnlyAccessEdgesInStructures::searchTemplate(
    ScAddr const & templateAddr,
    ScTemplateParams const & templateParams,
    ScAddrHashSet const & variables,
    Replacements & result)
{
  searchWithoutContentResult = std::make_unique<ScTemplateSearchResult>();
  ScTemplate searchTemplate;
  if (context->HelperBuildTemplate(searchTemplate, templateAddr, templateParams))
  {
    if (context->HelperCheckEdge(
            InferenceKeynodes::concept_template_with_links, templateAddr, ScType::EdgeAccessConstPosPerm))
    {
      searchTemplateWithContent(searchTemplate, templateAddr, templateParams, result);
    }
    else
    {
      int resultCounter = 0;
      int filterCounter = 0;
      if (this->contentOfAllInputStructures->empty())
      {
        SC_LOG_INFO("start input processing");
        for (auto const & inputStructure : inputStructures)
        {
          ScAddrVector const & edges =
              utils::IteratorUtils::getAllWithType(context, inputStructure, ScType::EdgeAccess);
          contentOfAllInputStructures->insert(edges.cbegin(), edges.cend());
        }
        SC_LOG_INFO("input processed(" << contentOfAllInputStructures->size() << ")");
      }
      context->HelperSmartSearchTemplate(
          searchTemplate,
          [templateParams, &result, &variables, &resultCounter, this](
              ScTemplateSearchResultItem const & item) -> ScTemplateSearchRequest {
              resultCounter++;
              if (resultCounter % 10000 == 0)
                SC_LOG_INFO("created " << resultCounter << "th result");
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
          [&filterCounter, this](ScAddr const & item) -> bool {
            filterCounter++;
            if (filterCounter % 100000 == 0)
              SC_LOG_INFO("filtered " << filterCounter << "th item");
            // Filter result item belonging to any of the input structures
            return !context->GetElementType(item).BitAnd(ScType::EdgeAccess) || contentOfAllInputStructures->count(item);
          });
    }
  }
  else
  {
    throw std::runtime_error("Template is not built.");
  }
}
}  // namespace inference
