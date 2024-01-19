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
      if (this->contentOfAllInputStructures->empty())
      {
        SC_LOG_DEBUG("start input processing");
        for (auto const & inputStructure : inputStructures)
        {
          ScAddrVector const & edges =
              utils::IteratorUtils::getAllWithType(context, inputStructure, ScType::EdgeAccess);
          contentOfAllInputStructures->insert(edges.cbegin(), edges.cend());
        }
        SC_LOG_DEBUG("input processed, found " << contentOfAllInputStructures->size() << " edges");
      }
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
            // Filter result item by not being Access Edge or belonging to any of the input structures
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
