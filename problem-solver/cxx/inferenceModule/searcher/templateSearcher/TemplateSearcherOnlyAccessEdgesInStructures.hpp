#pragma once

#include "TemplateSearcherInStructures.hpp"

namespace inference
{

class TemplateSearcherOnlyAccessEdgesInStructures : public TemplateSearcherInStructures
{
public:
  explicit TemplateSearcherOnlyAccessEdgesInStructures(
      ScMemoryContext * context,
      ScAddrVector const & otherInputStructures);

  explicit TemplateSearcherOnlyAccessEdgesInStructures(ScMemoryContext * ms_context);

  void searchTemplate(
      ScAddr const & templateAddr,
      ScTemplateParams const & templateParams,
      ScAddrHashSet const & variables,
      Replacements & result) override;
};

}  // namespace inference