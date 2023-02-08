/*
 * This source file is part of an OSTIS project. For the latest info, see http://ostis.net
 * Distributed under the MIT License
 * (See accompanying file COPYING.MIT or copy at http://opensource.org/licenses/MIT)
 */

#include "DirectInferenceManager.hpp"

#include <sc-memory/sc_addr.hpp>
#include <sc-agents-common/utils/GenerationUtils.hpp>
#include <sc-agents-common/utils/IteratorUtils.hpp>

#include <algorithm>

#include "utils/ContainersUtils.hpp"
#include "logic/LogicExpression.hpp"

using namespace inference;

DirectInferenceManager::DirectInferenceManager(ScMemoryContext * ms_context)
  : ms_context(ms_context)
{
  solutionTreeGenerator = std::make_unique<SolutionTreeGenerator>(ms_context);
  templateManager = std::make_unique<TemplateManager>(ms_context);
  templateSearcher = std::make_unique<TemplateSearcher>(ms_context);
}

ScAddr DirectInferenceManager::applyInference(
    ScAddr const & targetStructure,
    ScAddr const & formulasSet,
    ScAddr const & arguments,
    ScAddr const & inputStructure)
{
  ScAddr outputStructure = ms_context->CreateNode(ScType::NodeConstStruct);
  ScAddrVector argumentVector = utils::IteratorUtils::getAllWithType(ms_context, arguments, ScType::Node);
  for (ScAddr const & argument : argumentVector)
  {
    templateSearcher->addParam(argument);
  }
  templateSearcher->setArguments(arguments);

  bool targetAchieved = isTargetAchieved(targetStructure, argumentVector);
  if (targetAchieved)
  {
    SC_LOG_DEBUG("Target is already achieved");
    return solutionTreeGenerator->createSolution(outputStructure, targetAchieved);
  }

  std::vector<std::vector<ScAddr>> vectorOfVectorsOfFormulas;

  ScAddr setOfFormulas =
      utils::IteratorUtils::getAnyByOutRelation(ms_context, formulasSet, scAgentsCommon::CoreKeynodes::rrel_1);
  while (setOfFormulas.IsValid())
  {
    std::vector<ScAddr> vectorOfFormulas;
    vectorOfVectorsOfFormulas.push_back(utils::IteratorUtils::getAllWithType(ms_context, setOfFormulas, ScType::Node));
    setOfFormulas = utils::IteratorUtils::getNextFromSet(ms_context, formulasSet, setOfFormulas);
  }

  if (vectorOfVectorsOfFormulas.empty())
  {
    SC_THROW_EXCEPTION(utils::ExceptionItemNotFound, "No rule sets found.");
  }

  
  SC_LOG_DEBUG("Start rule applying. There is " + to_string(vectorOfVectorsOfFormulas.size()) + " formulas sets");
  for (int indexOfFormulaSet = 0; indexOfFormulaSet < vectorOfVectorsOfFormulas.size() && !targetAchieved; ++indexOfFormulaSet)
  {
    vector<ScAddr> const & formulaSet = vectorOfVectorsOfFormulas[indexOfFormulaSet];
    SC_LOG_DEBUG(
        "There is " + to_string(formulaSet.size()) + " formulas in " + to_string(indexOfFormulaSet + 1) +
        " set");
    for (auto formula : formulaSet)
    {
      SC_LOG_DEBUG("Trying to generate by formula: " + ms_context->HelperGetSystemIdtf(formula));
      bool isGenerated = useFormula(formula, argumentVector, outputStructure);
      SC_LOG_DEBUG(std::string("Logical formula is ") + (isGenerated ? "generated" : "not generated"));
      if (isGenerated)
      {
        solutionTreeGenerator->addNode(formula, ScTemplateParams());
        targetAchieved = isTargetAchieved(targetStructure, argumentVector);
        if (targetAchieved)
        {
          SC_LOG_DEBUG("Target achieved");
          break;
        }
        else
        {
          indexOfFormulaSet = -1;
          break;
        }
      }
    }
  }

  return solutionTreeGenerator->createSolution(outputStructure, targetAchieved);
}

ScAddrQueue DirectInferenceManager::createQueue(ScAddr const & set)
{
  ScAddrQueue queue;
  ScAddrVector elementList = utils::IteratorUtils::getAllWithType(ms_context, set, ScType::Node);

  ContainersUtils::addToQueue(elementList, queue);
  return queue;
}

bool DirectInferenceManager::useFormula(
    ScAddr const & rule,
    ScAddrVector /*const*/ & argumentVector,
    ScAddr const & outputStructure)
{
  LogicFormulaResult formulaResult = {false, false, {}};
  ScAddr const formulaRoot =
      utils::IteratorUtils::getAnyByOutRelation(ms_context, rule, InferenceKeynodes::rrel_main_key_sc_element);
  if (!formulaRoot.IsValid())
    return false;

  LogicExpression logicExpression(ms_context, templateSearcher.get(), templateManager.get(), outputStructure);

  unique_ptr<LogicExpressionNode> expressionRoot = logicExpression.build(formulaRoot);
  expressionRoot->setArgumentVector(argumentVector);

  LogicFormulaResult result = expressionRoot->compute(formulaResult);

  return result.isGenerated;
}

vector<ScAddrQueue> DirectInferenceManager::createFormulasQueuesListByPriority(ScAddr const & formulasSet)
{
  vector<ScAddrQueue> formulasQueuesList;

  ScAddr setOfFormulas =
      utils::IteratorUtils::getAnyByOutRelation(ms_context, formulasSet, scAgentsCommon::CoreKeynodes::rrel_1);
  while (setOfFormulas.IsValid())
  {
    formulasQueuesList.push_back(createQueue(setOfFormulas));
    setOfFormulas = utils::IteratorUtils::getNextFromSet(ms_context, formulasSet, setOfFormulas);
  }

  return formulasQueuesList;
}

bool DirectInferenceManager::isTargetAchieved(ScAddr const & targetStructure, ScAddrVector const & argumentVector)
{
  std::vector<ScTemplateParams> const templateParamsVector =
      templateManager->createTemplateParams(targetStructure, argumentVector);
  return std::any_of(
      templateParamsVector.cbegin(),
      templateParamsVector.cend(),
      [this, &targetStructure](ScTemplateParams const & templateParams) {
        return !templateSearcher->searchTemplate(targetStructure, templateParams).empty();
      });
}
