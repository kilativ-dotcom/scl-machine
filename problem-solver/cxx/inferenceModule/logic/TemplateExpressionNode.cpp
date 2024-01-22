/*
 * This source file is part of an OSTIS project. For the latest info, see http://ostis.net
 * Distributed under the MIT License
 * (See accompanying file COPYING.MIT or copy at http://opensource.org/licenses/MIT)
 */

#include "TemplateExpressionNode.hpp"

#include "inferenceConfig/InferenceConfig.hpp"

#include "searcher/templateSearcher/TemplateSearcherGeneral.hpp"

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
  this->templateSearcherInKb = std::make_unique<TemplateSearcherGeneral>(context);
  this->templateSearcherInKb->setReplacementsUsingType(this->templateSearcher->getReplacementsUsingType());
  this->templateSearcherInKb->setOutputStructureFillingType(this->templateSearcher->getOutputStructureFillingType());
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
  std::vector<ScTemplateParams> paramsVector =
      ReplacementsUtils::getReplacementsToScTemplateParams(getReplacementsWithoutEdges(replacements));
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

Replacements TemplateExpressionNode::getReplacementsWithoutEdges(Replacements const & replacements) const
{
  ScAddrHashSet edges;
  for (auto const & replacement : replacements)
  {
    if (context->GetElementType(replacement.first).IsEdge())
      edges.insert(replacement.first);
  }
  return ReplacementsUtils::removeRows(replacements, edges);
}

/**
 * @brief Generate atomic logical formula using replacements
 * @param replacements variables and ScAddrs to use in generation
 * @return LogicFormulaResult{bool: value, bool: isGenerated, Replacements: replacements}
 */
LogicFormulaResult TemplateExpressionNode::generate(Replacements & replacements)
{
  SC_LOG_INFO("TemplateExpressionNode::generate");
  LogicFormulaResult result;
  if (ReplacementsUtils::getColumnsAmount(replacements) == 0)
  {
    SC_LOG_DEBUG("Atomic logical formula " << context->HelperGetSystemIdtf(formula) << " is not generated");
    return result;
  }

  ScAddrHashSet formulaVariables;
  templateSearcher->getVariables(formula, formulaVariables);
  size_t count = 0;
  Replacements searchResult;
  Replacements generatedReplacements;
  Replacements resultWithoutReplacements;
  if (templateSearcher->getAtomicLogicalFormulaSearchBeforeGenerationType() == SEARCH_WITHOUT_REPLACEMENTS)
  {
    Replacements fakeReplacements;
    fakeReplacements[context->CreateNode(ScType::NodeVar)].push_back(context->CreateNode(ScType::NodeConstClass));
    resultWithoutReplacements = findInKb(fakeReplacements);

    if (outputStructure.IsValid() && templateManager->getFillingType() == SEARCHED_AND_GENERATED
        && ReplacementsUtils::getColumnsAmount(resultWithoutReplacements) > 0)
    {
      Replacements const & intersection =
          ReplacementsUtils::intersectReplacements(replacements, resultWithoutReplacements);
      if (ReplacementsUtils::getColumnsAmount(intersection) > 0)
      {
        addToOutputStructure(intersection, formulaVariables);
        addFormulaConstantsToOutputStructure();
      }
    }
  }

  if (templateManager->getGenerationType() == GENERATE_UNIQUE_FORMULAS)
  {
    Replacements const & difference =
        ReplacementsUtils::subtractReplacements(replacements, resultWithoutReplacements);
    generateByReplacements(difference, result, count, formulaVariables, searchResult, generatedReplacements);
  }
  else
    generateByReplacements(replacements, result, count, formulaVariables, searchResult, generatedReplacements);

  result.replacements = ReplacementsUtils::intersectReplacements(
      ReplacementsUtils::intersectReplacements(searchResult, resultWithoutReplacements),
      generatedReplacements);

  SC_LOG_DEBUG(
      "Atomic logical formula " << context->HelperGetSystemIdtf(formula) << " is generated " << count << " times");

  return result;
}

Replacements TemplateExpressionNode::findInKb(Replacements const & replacements) const
{
  std::vector<ScTemplateParams> paramsVector =
      ReplacementsUtils::getReplacementsToScTemplateParams(getReplacementsWithoutEdges(replacements));
  Replacements resultReplacements;
  ScAddrHashSet variables;
  templateSearcherInKb->getVariables(formula, variables);
  SC_LOG_INFO("TemplateExpressionNode: call findInKb for " << paramsVector.size() << " params");
  templateSearcherInKb->searchTemplate(formula, paramsVector, variables, resultReplacements);
  SC_LOG_INFO("TemplateExpressionNode: findInKb finished");

  std::string const idtf = context->HelperGetSystemIdtf(formula);
  SC_LOG_DEBUG("findInKb Statement " << idtf << (!resultReplacements.empty() ? " true" : " false"));

  return resultReplacements;
}

void TemplateExpressionNode::generateByReplacements(
    Replacements const & replacements,
    LogicFormulaResult & result,
    size_t & count,
    ScAddrHashSet const & formulaVariables,
    Replacements & searchResult,
    Replacements & generatedReplacements)
{
  Replacements const & replacementsWithoutEdges = getReplacementsWithoutEdges(replacements);
  std::vector<ScTemplateParams> const & paramsVector = ReplacementsUtils::getReplacementsToScTemplateParams(replacementsWithoutEdges);
  processTemplateParams(paramsVector, formulaVariables, result, count, searchResult, generatedReplacements);
  bool const searchResultIsAddedToOutput =
      outputStructure.IsValid() && templateManager->getFillingType() == SEARCHED_AND_GENERATED &&
      ReplacementsUtils::getColumnsAmount(searchResult) > 0 &&
      templateSearcher->getAtomicLogicalFormulaSearchBeforeGenerationType() != SEARCH_WITHOUT_REPLACEMENTS;
  if (searchResultIsAddedToOutput)
    addToOutputStructure(searchResult, formulaVariables);
  if ((result.isGenerated || searchResultIsAddedToOutput) && outputStructure.IsValid())
    addFormulaConstantsToOutputStructure();
}

void TemplateExpressionNode::processTemplateParams(
    vector<ScTemplateParams> const & paramsVector,
    ScAddrHashSet const & formulaVariables,
    LogicFormulaResult & result,
    size_t & count,
    Replacements & searchResult,
    Replacements & generatedReplacements)
{
  for (ScTemplateParams const & params : paramsVector)
  {
    if (templateManager->getReplacementsUsingType() == REPLACEMENTS_FIRST && result.isGenerated)
      return;
    if (templateManager->getGenerationType() == GENERATE_UNIQUE_FORMULAS)
      templateSearcherInKb->searchTemplate(formula, params, formulaVariables, searchResult);
    if (templateManager->getGenerationType() != GENERATE_UNIQUE_FORMULAS || searchResult.empty())
      generateByParams(params, formulaVariables, generatedReplacements, result, count);
  }
}

void TemplateExpressionNode::generateByParams(
    ScTemplateParams const & params,
    ScAddrHashSet const & formulaVariables,
    Replacements & generatedReplacements,
    LogicFormulaResult & result,
    size_t & count)
{
  ScTemplate generatedTemplate;
  context->HelperBuildTemplate(generatedTemplate, formula, params);

  ScTemplateGenResult generationResult;
  ScTemplate::Result const & scTemplateResult = context->HelperGenTemplate(generatedTemplate, generationResult);
  if (scTemplateResult)
  {
    ++count;
    result.isGenerated = true;
    result.value = true;
    for (ScAddr const & variable : formulaVariables)
    {
      ScAddr outAddr;
      if (generationResult.Get(variable, outAddr) || params.Get(variable, outAddr))
        generatedReplacements[variable].push_back(outAddr);
      else
        SC_THROW_EXCEPTION(
            utils::ExceptionInvalidState,
            "generation result and template params do not have replacement for "
                << variable.Hash());
    }
    addToOutputStructure(generationResult);
  }
}

void TemplateExpressionNode::addFormulaConstantsToOutputStructure()
{
  ScAddrHashSet formulaConstants;
  templateSearcher->getConstants(formula, formulaConstants);
  addToOutputStructure(formulaConstants);
}

void TemplateExpressionNode::addToOutputStructure(Replacements const & replacements, ScAddrHashSet const & variables)
{
  if (outputStructure.IsValid())
  {
    for (auto const & pair : replacements)
    {
      if (variables.find(pair.first) != variables.cend())
      {
        for (auto const & replacement : pair.second)
          processOutputStructureElement(replacement);
      }
    }
  }
}

void TemplateExpressionNode::addToOutputStructure(ScAddrHashSet const & elements)
{
  if (outputStructure.IsValid())
  {
    for (auto const & element : elements)
      processOutputStructureElement(element);
  }
}

void TemplateExpressionNode::addToOutputStructure(ScTemplateResultItem const & item)
{
  if (outputStructure.IsValid())
  {
    for (size_t i = 0; i < item.Size(); ++i)
      processOutputStructureElement(item[i]);
  }
}

void TemplateExpressionNode::processOutputStructureElement(ScAddr const & element)
{
  if (outputStructureElements.find(element) == outputStructureElements.cend())
  {
    context->CreateEdge(ScType::EdgeAccessConstPosPerm, outputStructure, element);
    outputStructureElements.insert(element);
  }
}
