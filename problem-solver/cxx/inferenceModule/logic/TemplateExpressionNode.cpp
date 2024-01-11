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
    SC_LOG_INFO("TemplateExpressionNode: compute for not empty arguments");
    std::vector<ScTemplateParams> const & templateParamsVector = templateManager->createTemplateParams(formula);
    templateSearcher->searchTemplate(formula, templateParamsVector, variables, replacements);
  }
  else
  {
    SC_LOG_INFO("TemplateExpressionNode: compute for empty arguments");
    templateSearcher->searchTemplate(formula, ScTemplateParams(), variables, replacements);
  }
  SC_LOG_INFO("TemplateExpressionNode: compute finished");

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
  SC_LOG_INFO("TemplateExpressionNode: call search for " << paramsVector.size() << " params");
  templateSearcher->searchTemplate(formula, paramsVector, variables, resultReplacements);
  SC_LOG_INFO("TemplateExpressionNode: search finished");
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

  Replacements fakeReplacements;
  fakeReplacements[context->CreateNode(ScType::NodeVar)].push_back(context->CreateNode(ScType::NodeConstClass));
  LogicFormulaResult const & resultWithoutReplacements = find(fakeReplacements);

  Replacements const & intersection =
      ReplacementsUtils::intersectReplacements(replacements, resultWithoutReplacements.replacements);
  
  
  ScAddrHashSet allVariables;
  ReplacementsUtils::getKeySet(replacements, allVariables);
  ScAddrHashSet formulaVariables;
  templateSearcher->getVariables(formula, formulaVariables);
  allVariables.insert(formulaVariables.cbegin(), formulaVariables.cend());

  if (outputStructure.IsValid() && templateManager->getFillingType() == SEARCHED_AND_GENERATED &&
      ReplacementsUtils::getColumnsAmount(intersection) > 0)
  {
    for (auto const & pair : intersection)
    {
      if (formulaVariables.find(pair.first) != formulaVariables.cend())
      for (auto const & replacement : pair.second)
      {
        if (outputStructureElements.find(replacement) == outputStructureElements.cend())
        {
          context->CreateEdge(ScType::EdgeAccessConstPosPerm, outputStructure, replacement);
          outputStructureElements.insert(replacement);
        }
      }
    }
    ScAddrHashSet formulaConstants;
    templateSearcher->getConstants(formula, formulaConstants);
    for (auto const & formulaConstant : formulaConstants)
    {
      if (outputStructureElements.find(formulaConstant) == outputStructureElements.cend())
      {
        context->CreateEdge(ScType::EdgeAccessConstPosPerm, outputStructure, formulaConstant);
        outputStructureElements.insert(formulaConstant);
      }
    }
  }

  size_t count = 0;
  if (templateManager->getGenerationType() == GENERATE_UNIQUE_FORMULAS)
  {
    Replacements const & difference =
        ReplacementsUtils::subtractReplacements(replacements, resultWithoutReplacements.replacements);
    generateByReplacements(difference, result, count);
  }
  else
    generateByReplacements(intersection, result, count);

  result.replacements = intersection;

  SC_LOG_DEBUG(
      "Atomic logical formula " << context->HelperGetSystemIdtf(formula) << " is generated " << count << " times");

  return result;
}
void TemplateExpressionNode::generateByReplacements(
    Replacements const & replacements,
    LogicFormulaResult & result,
    size_t & count)
{
  vector<ScTemplateParams> const & paramsVector = ReplacementsUtils::getReplacementsToScTemplateParams(replacements);
  for (auto const & params : paramsVector)
  {
    ScTemplate generatedTemplate;
    context->HelperBuildTemplate(generatedTemplate, formula, params);

    ScTemplateGenResult generationResult;
    ScTemplate::Result const & genTemplate = context->HelperGenTemplate(generatedTemplate, generationResult);
    if (genTemplate)
    {
      ++count;
      result.isGenerated = true;
      result.value = true;
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
    if (templateManager->getReplacementsUsingType() == REPLACEMENTS_FIRST && result.isGenerated)
      break;
  }
  if (result.isGenerated)
  {
    ScAddrHashSet formulaConstants;
    templateSearcher->getConstants(formula, formulaConstants);
    for (auto const & formulaConstant : formulaConstants)
    {
      if (outputStructureElements.find(formulaConstant) == outputStructureElements.cend())
      {
        context->CreateEdge(ScType::EdgeAccessConstPosPerm, outputStructure, formulaConstant);
        outputStructureElements.insert(formulaConstant);
      }
    }
  }
}
