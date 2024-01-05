#include "keynodes/InferenceKeynodes.hpp"

#include "TemplateSearcherOnlyAccessEdgesInStructures.hpp"

namespace inference
{

TemplateSearcherOnlyAccessEdgesInStructures::TemplateSearcherOnlyAccessEdgesInStructures(
    ScMemoryContext * context,
    ScAddrVector const & otherInputStructures)
  : TemplateSearcherInStructures(context, otherInputStructures)
{
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
              if (templateParams.Get(variable, argument))
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
            return !context->GetElementType(item).BitAnd(ScType::EdgeAccess) ||
                   std::any_of(
                       inputStructures.cbegin(),
                       inputStructures.cend(),
                       [&item, this](ScAddr const & structure) -> bool {
                         return context->HelperCheckEdge(structure, item, ScType::EdgeAccessConstPosPerm);
                       });
          });
    }
  }
  else
  {
    throw std::runtime_error("Template is not built.");
  }
}
}  // namespace inference
