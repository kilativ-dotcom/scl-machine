/*
 * This source file is part of an OSTIS project. For the latest info, see http://ostis.net
 * Distributed under the MIT License
 * (See accompanying file COPYING.MIT or copy at http://opensource.org/licenses/MIT)
 */

#include "TemplateExpressionNode.hpp"

#include "inferenceConfig/InferenceConfig.hpp"

#include "sc-agents-common/utils/GenerationUtils.hpp"

TemplateExpressionNode::TemplateExpressionNode(
    ScMemoryContext * context,
    std::shared_ptr<TemplateSearcherAbstract> templateSearcher,
    std::shared_ptr<TemplateManagerAbstract> templateManager,
    std::shared_ptr<SolutionTreeManagerAbstract> solutionTreeManager,
    ScAddr const & outputStructure,
    ScAddr const & formula)
  : context(context)
  , templateSearcher(std::move(templateSearcher))
  , templateManager(std::move(templateManager))
  , solutionTreeManager(std::move(solutionTreeManager))
  , outputStructure(outputStructure)
  , formula(formula)
{
}

void TemplateExpressionNode::compute(LogicFormulaResult & result) const
{
  Replacements replacements;
  ScAddrHashSet variables;
  templateSearcher->getVariables(formula, variables);
  // Template params should be created only if argument vector is not empty. Else search with any possible replacements
  if (!argumentVector.empty())
  {
    std::vector<ScTemplateParams> const & templateParamsVector = templateManager->createTemplateParams(formula);
    templateSearcher->searchTemplate(formula, templateParamsVector, variables, replacements);
  }
  else
  {
    templateSearcher->searchTemplate(formula, ScTemplateParams(), variables, replacements);
  }

  result.replacements = replacements;
  result.value = !result.replacements.empty();
  SC_LOG_DEBUG(
      "Compute atomic logical formula " << context->HelperGetSystemIdtf(formula)
                                        << (result.value ? " true" : " false"));
}

LogicFormulaResult TemplateExpressionNode::find(Replacements & replacements) const
{
  LogicFormulaResult result;
  std::vector<ScTemplateParams> paramsVector = ReplacementsUtils::getReplacementsToScTemplateParams(replacements);
  Replacements resultReplacements;
  ScAddrHashSet variables;
  templateSearcher->getVariables(formula, variables);
  templateSearcher->searchTemplate(formula, paramsVector, variables, resultReplacements);
  result.replacements = resultReplacements;
  result.value = !result.replacements.empty();

  std::string const idtf = context->HelperGetSystemIdtf(formula);
  SC_LOG_DEBUG("Find Statement " << idtf << (result.value ? " true" : " false"));

  return result;
}

/**
 * @brief Generate atomic logical formula using replacements
 * @param replacements variables and ScAddrs to use in generation
 * @return LogicFormulaResult{bool: value, bool: isGenerated, Replacements: replacements}
 */
LogicFormulaResult TemplateExpressionNode::generate(Replacements & replacements)
{
  LogicFormulaResult result;

  // Convert replacements to templateParams to generate by them
  std::vector<ScTemplateParams> paramsVector = ReplacementsUtils::getReplacementsToScTemplateParams(replacements);
  if (paramsVector.empty())
  {
    SC_LOG_DEBUG("Atomic logical formula " << context->HelperGetSystemIdtf(formula) << " is not generated");
    return result;
  }

  ScAddrHashSet variables;
  ReplacementsUtils::getKeySet(replacements, variables);
  templateSearcher->getVariables(formula, variables);

  size_t count = 0;
  Replacements searchResult;
  for (ScTemplateParams const & scTemplateParams : paramsVector)
  {
    if (templateManager->getReplacementsUsingType() == REPLACEMENTS_FIRST && result.isGenerated)
      break;

    if (templateManager->getGenerationType() == GENERATE_UNIQUE_FORMULAS)
    {
      templateSearcher->searchTemplate(formula, scTemplateParams, variables, searchResult);
    }

    if (templateManager->getGenerationType() != GENERATE_UNIQUE_FORMULAS || searchResult.empty())
    {
      ScTemplate generatedTemplate;
      context->HelperBuildTemplate(generatedTemplate, formula, scTemplateParams);

      ScTemplateGenResult generationResult;
      ScTemplate::Result const & genTemplate = context->HelperGenTemplate(generatedTemplate, generationResult);
      if (genTemplate)
      {
        ++count;
        result.isGenerated = true;
        result.value = true;
        Replacements temporalReplacements;
        for (ScAddr const & variable : variables)
        {
          ScAddrVector replacementsVector;
          ScAddr outAddr;
          generationResult.Get(variable, outAddr);
          bool const generationHasVar = outAddr.IsValid();
          ScAddr outResult;
          bool const paramsHaveVar = scTemplateParams.Get(variable, outResult);
          if (generationHasVar)
            replacementsVector.push_back(generationResult[variable]);
          else if (paramsHaveVar)
            replacementsVector.push_back(outResult);
          else
            SC_THROW_EXCEPTION(
                utils::ExceptionInvalidState,
                "generation result and template params do not have replacement for "
                    << context->HelperGetSystemIdtf(variable));
          temporalReplacements[variable] = replacementsVector;
        }
        result.replacements = ReplacementsUtils::uniteReplacements(result.replacements, temporalReplacements);
      }

      if (outputStructure.IsValid())
      {
        for (size_t i = 0; i < generationResult.Size(); ++i)
        {
          ScAddr const & generatedElement = generationResult[i];
          if (outputStructureElements.find(generatedElement) == outputStructureElements.cend())
          {
            context->CreateEdge(ScType::EdgeAccessConstPosPerm, outputStructure, generatedElement);
            outputStructureElements.insert(generatedElement);
          }
        }
      }
    }

    if (templateManager->getFillingType() == SEARCHED_AND_GENERATED && !searchResult.empty())
    {
      for (const auto & elements : searchResult)
      {
        for (const auto & element : elements.second)
        {
          if (outputStructureElements.find(element) == outputStructureElements.cend())
          {
            context->CreateEdge(ScType::EdgeAccessConstPosPerm, outputStructure, element);
            outputStructureElements.insert(element);
          }
        }
      }
    }
  }

  SC_LOG_DEBUG(
      "Atomic logical formula " << context->HelperGetSystemIdtf(formula) << " is generated " << count << " times");

  return result;
}
