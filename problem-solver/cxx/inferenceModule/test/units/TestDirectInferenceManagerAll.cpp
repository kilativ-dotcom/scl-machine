/*
 * This source file is part of an OSTIS project. For the latest info, see http://ostis.net
 * Distributed under the MIT License
 * (See accompanying file COPYING.MIT or copy at http://opensource.org/licenses/MIT)
 */

#include "sc_test.hpp"
#include "scs_loader.hpp"

#include "sc-agents-common/keynodes/coreKeynodes.hpp"
#include "sc-agents-common/utils/IteratorUtils.hpp"
#include "sc-agents-common/utils/GenerationUtils.hpp"

#include "keynodes/InferenceKeynodes.hpp"
#include "factory/InferenceManagerFactory.hpp"

using namespace inference;

namespace inferenceManagerBuilderTest
{
ScsLoader loader;
std::string const TEST_FILES_DIR_PATH = TEMPLATE_SEARCH_MODULE_TEST_SRC_PATH "/testStructures/InferenceManagerInputStructuresStrategyAll/";

std::string const INPUT_STRUCTURE1 = "input_structure1";
std::string const INPUT_STRUCTURE2 = "input_structure2";
std::string const RULES_SET = "rules_set";
std::string const ARGUMENT = "argument";
std::string const TAIL = "tail";
std::string const TARGET_NODE_CLASS = "target_node_class";
std::string const CURRENT_NODE_CLASS = "current_node_class";

using InferenceManagerBuilderTest = ScMemoryTest;

void initialize()
{
  InferenceKeynodes::InitGlobal();
  scAgentsCommon::CoreKeynodes::InitGlobal();
}

void generateTripleInStructure(ScMemoryContext & context, ScAddr const & set, ScAddr const & element, ScAddr const & structure);
void generateBodyHeadInStructure(ScMemoryContext & context, ScAddr const & tailElement, ScAddr const & nrelBodyElements, ScAddr const & nrelHead, ScAddr const & structure);
void generateSnakes(ScMemoryContext & context, size_t snakesCount, size_t tailsElementsCount);
void generateTails(ScMemoryContext & context, ScAddr const & tail, size_t tailsCount, size_t tailsElementsCount);

TEST_F(InferenceManagerBuilderTest, SingleSuccessApplyInference)
{
  ScMemoryContext & context = *m_ctx;

  loader.loadScsFile(context, TEST_FILES_DIR_PATH + "singleApplyTest.scs");
  initialize();

  // Form input structures set of two structures. One of them consists one triple from premise and the other -- the second triple.
  ScAddr const & inputStructure1 = context.HelperResolveSystemIdtf(INPUT_STRUCTURE1);
  ScAddr const & inputStructure2 = context.HelperResolveSystemIdtf(INPUT_STRUCTURE2);
  ScAddrVector inputStructures {inputStructure1, inputStructure2};

  // Get arguments set. It is a singleton
  ScAddr const & argument = context.HelperResolveSystemIdtf(ARGUMENT);
  ScAddrVector arguments {argument};

  // Create output structure to generate structures in
  ScAddr const & outputStructure = context.CreateNode(ScType::NodeConstStruct);

  // Get formulas set to apply
  ScAddr const & formulasSet = context.HelperResolveSystemIdtf(RULES_SET);

  // Form inference params config
  InferenceParamsConfig const & inferenceParamsConfig {formulasSet, arguments, inputStructures, outputStructure};

  // Create inference manager with `strategy all` using director
  InferenceFlowConfig const & inferenceFlowConfig{false, false, false};
  std::unique_ptr<inference::InferenceManagerAbstract> iterationStrategy =
      inference::InferenceManagerFactory::constructDirectInferenceManagerAll(&context, inferenceFlowConfig, inputStructures);

  // Apply inference with configured manager and params config
  bool result = iterationStrategy->applyInference(inferenceParamsConfig);
  EXPECT_TRUE(result);

  ScAddr const & solution = iterationStrategy->getSolutionTreeManager()->createSolution(outputStructure, result);
  EXPECT_TRUE(
      context.HelperCheckEdge(InferenceKeynodes::concept_success_solution, solution, ScType::EdgeAccessConstPosPerm));

  ScAddr const & targetClass = context.HelperFindBySystemIdtf(TARGET_NODE_CLASS);
  EXPECT_TRUE(targetClass.IsValid());

  EXPECT_TRUE(context.HelperCheckEdge(targetClass, argument, ScType::EdgeAccessConstPosPerm));
}

TEST_F(InferenceManagerBuilderTest, SnakeApplyInference)
{
  ScMemoryContext & context = *m_ctx;

  loader.loadScsFile(context, TEST_FILES_DIR_PATH + "snakeSingleArgumentApplyTest.scs");
  initialize();

  ScAddr const & inputStructure1 = context.HelperResolveSystemIdtf(INPUT_STRUCTURE1);
  ScAddr const & inputStructure2 = context.HelperResolveSystemIdtf(INPUT_STRUCTURE2);
  ScAddrVector inputStructures{inputStructure1, inputStructure2};
  ScAddr const & argument = context.HelperResolveSystemIdtf(TAIL);
  ScAddrVector arguments {argument};
  ScAddr const & rulesSet = context.HelperResolveSystemIdtf(RULES_SET);
  ScAddr const & outputStructure = context.CreateNode(ScType::NodeConstStruct);

  InferenceFlowConfig const & inferenceFlowConfig {false, false, false};
  std::unique_ptr<inference::InferenceManagerAbstract> iterationStrategy =
      inference::InferenceManagerFactory::constructDirectInferenceManagerAll(&context, inferenceFlowConfig, inputStructures);

  InferenceParamsConfig const & inferenceParamsConfig {rulesSet, arguments, inputStructures, outputStructure};
  bool result = iterationStrategy->applyInference(inferenceParamsConfig);

  EXPECT_TRUE(result);
  ScAddr const & solution = iterationStrategy->getSolutionTreeManager()->createSolution(outputStructure, result);
  EXPECT_TRUE(solution.IsValid());
  EXPECT_TRUE(
      context.HelperCheckEdge(InferenceKeynodes::concept_success_solution, solution, ScType::EdgeAccessConstPosPerm));

  ScAddr const & tail = context.HelperFindBySystemIdtf("tail");
  ScAddr const & head = context.HelperFindBySystemIdtf("head");
  ScAddr const & nrelTailHead = context.HelperFindBySystemIdtf("nrel_tail_head");
  ScIterator5Ptr const & conclusionIterator = context.Iterator5(
        tail,
        ScType::EdgeDCommonConst,
        head,
        ScType::EdgeAccessConstPosPerm,
        nrelTailHead);

  EXPECT_TRUE(conclusionIterator->Next());
}

TEST_F(InferenceManagerBuilderTest, SnakesApplyInference)
{
  ScMemoryContext & context = *m_ctx;

  loader.loadScsFile(context, TEST_FILES_DIR_PATH + "snakeSingleArgumentApplyTest.scs");
  initialize();

  generateSnakes(*m_ctx, 1000, 100);

  ScAddr const & inputStructure1 = context.HelperResolveSystemIdtf(INPUT_STRUCTURE1);
  ScAddr const & inputStructure2 = context.HelperResolveSystemIdtf(INPUT_STRUCTURE2);
  ScAddrVector inputStructures{inputStructure1, inputStructure2};
  ScAddr const & argument = context.HelperResolveSystemIdtf(TAIL);
  ScAddrVector arguments {argument};
  ScAddr const & rulesSet = context.HelperResolveSystemIdtf(RULES_SET);
  ScAddr const & outputStructure = context.CreateNode(ScType::NodeConstStruct);

  InferenceFlowConfig const & inferenceFlowConfig {false, false, false};
  std::unique_ptr<inference::InferenceManagerAbstract> iterationStrategy =
      inference::InferenceManagerFactory::constructDirectInferenceManagerAll(&context, inferenceFlowConfig, inputStructures);

  InferenceParamsConfig const & inferenceParamsConfig {rulesSet, arguments, inputStructures, outputStructure};
  bool result = iterationStrategy->applyInference(inferenceParamsConfig);

  EXPECT_TRUE(result);
  ScAddr const & solution = iterationStrategy->getSolutionTreeManager()->createSolution(outputStructure, result);

  EXPECT_TRUE(solution.IsValid());
  EXPECT_TRUE(
      context.HelperCheckEdge(InferenceKeynodes::concept_success_solution, solution, ScType::EdgeAccessConstPosPerm));

  ScAddr const & tail = context.HelperFindBySystemIdtf("tail");
  ScAddr const & head = context.HelperFindBySystemIdtf("head");
  ScAddr const & nrelTailHead = context.HelperFindBySystemIdtf("nrel_tail_head");
  ScIterator5Ptr const & conclusionIterator = context.Iterator5(
        tail,
        ScType::EdgeDCommonConst,
        head,
        ScType::EdgeAccessConstPosPerm,
        nrelTailHead);

  EXPECT_TRUE(conclusionIterator->Next());
}

TEST_F(InferenceManagerBuilderTest, SnakesTailsApplyInference)
{
  ScMemoryContext & context = *m_ctx;

  loader.loadScsFile(context, TEST_FILES_DIR_PATH + "snakeSingleArgumentApplyTest.scs");
  initialize();

  for (size_t i = 0; i <20; i++)
  {
    ScAddr const & tail = context.HelperFindBySystemIdtf("tail");
    generateTails(*m_ctx, tail, 10, 1000);

    ScAddr const & inputStructure1 = context.HelperResolveSystemIdtf(INPUT_STRUCTURE1);
    ScAddr const & inputStructure2 = context.HelperResolveSystemIdtf(INPUT_STRUCTURE2);
    ScAddrVector inputStructures{inputStructure1, inputStructure2};
    ScAddr const & argument = context.HelperResolveSystemIdtf(TAIL);
    ScAddrVector arguments {argument};
    ScAddr const & rulesSet = context.HelperResolveSystemIdtf(RULES_SET);
    ScAddr const & outputStructure = context.CreateNode(ScType::NodeConstStruct);

    InferenceFlowConfig const & inferenceFlowConfig {false, false, false};
    std::unique_ptr<inference::InferenceManagerAbstract> iterationStrategy =
        inference::InferenceManagerFactory::constructDirectInferenceManagerAll(&context, inferenceFlowConfig, inputStructures);

    InferenceParamsConfig const & inferenceParamsConfig {rulesSet, arguments, inputStructures, outputStructure};
    bool result = iterationStrategy->applyInference(inferenceParamsConfig);

    EXPECT_TRUE(result);
    ScAddr const & solution = iterationStrategy->getSolutionTreeManager()->createSolution(outputStructure, result);

    EXPECT_TRUE(solution.IsValid());
    EXPECT_TRUE(
        context.HelperCheckEdge(InferenceKeynodes::concept_success_solution, solution, ScType::EdgeAccessConstPosPerm));
  }

}

TEST_F(InferenceManagerBuilderTest, SnakesTailsConjunctionApplyInference)
{
  ScMemoryContext & context = *m_ctx;

  loader.loadScsFile(context, TEST_FILES_DIR_PATH + "snakeSingleArgumentConjunctionApplyTest.scs");
  initialize();

  ScAddr const & tail = context.HelperFindBySystemIdtf(TAIL);
  generateTails(*m_ctx, tail, 10, 10000);

  ScAddr const & inputStructure1 = context.HelperResolveSystemIdtf(INPUT_STRUCTURE1);
  ScAddr const & inputStructure2 = context.HelperResolveSystemIdtf(INPUT_STRUCTURE2);
  ScAddrVector inputStructures{inputStructure1, inputStructure2};
  ScAddr const & argument = context.HelperResolveSystemIdtf(TAIL);
  ScAddrVector arguments {argument};

  ScAddr const & rulesSet = context.HelperResolveSystemIdtf(RULES_SET);
  ScAddr const & outputStructure = context.CreateNode(ScType::NodeConstStruct);

  InferenceFlowConfig const & inferenceFlowConfig {false, false, false};
  std::unique_ptr<inference::InferenceManagerAbstract> iterationStrategy =
      inference::InferenceManagerFactory::constructDirectInferenceManagerAll(&context, inferenceFlowConfig, inputStructures);

  InferenceParamsConfig const & inferenceParamsConfig {rulesSet, arguments, inputStructures, outputStructure};
  bool result = iterationStrategy->applyInference(inferenceParamsConfig);

  EXPECT_TRUE(result);
  ScAddr const & solution = iterationStrategy->getSolutionTreeManager()->createSolution(outputStructure, result);

  EXPECT_TRUE(solution.IsValid());
  EXPECT_TRUE(
      context.HelperCheckEdge(InferenceKeynodes::concept_success_solution, solution, ScType::EdgeAccessConstPosPerm));
}

TEST_F(InferenceManagerBuilderTest, MultipleSuccessApplyInference)
{
  ScMemoryContext & context = *m_ctx;

  loader.loadScsFile(context, TEST_FILES_DIR_PATH + "fiveTimesApplyTest.scs");
  initialize();

  ScAddr const & inputStructure1 = context.HelperResolveSystemIdtf(INPUT_STRUCTURE1);
  ScAddr const & inputStructure2 = context.HelperResolveSystemIdtf(INPUT_STRUCTURE2);
  ScAddrVector inputStructures{inputStructure1, inputStructure2};
  ScAddr const & argument = context.HelperResolveSystemIdtf(ARGUMENT);
  ScAddrVector arguments {argument};
  for (size_t i = 1; i < 6; i++)
  {
    arguments.push_back(context.HelperResolveSystemIdtf(ARGUMENT + to_string(i)));
  }
  ScAddr const & rulesSet = context.HelperResolveSystemIdtf(RULES_SET);
  ScAddr const & outputStructure = context.CreateNode(ScType::NodeConstStruct);

  InferenceFlowConfig const & inferenceFlowConfig {false, false, false};
  std::unique_ptr<inference::InferenceManagerAbstract> iterationStrategy =
      inference::InferenceManagerFactory::constructDirectInferenceManagerAll(&context, inferenceFlowConfig, inputStructures);

  InferenceParamsConfig const & inferenceParamsConfig {rulesSet, arguments, inputStructures, outputStructure};
  bool result = iterationStrategy->applyInference(inferenceParamsConfig);

  EXPECT_TRUE(result);
  ScAddr const & solution = iterationStrategy->getSolutionTreeManager()->createSolution(outputStructure, result);

  EXPECT_TRUE(solution.IsValid());
  EXPECT_TRUE(
      context.HelperCheckEdge(InferenceKeynodes::concept_success_solution, solution, ScType::EdgeAccessConstPosPerm));

  ScAddr const & targetClass = context.HelperFindBySystemIdtf(TARGET_NODE_CLASS);
  EXPECT_TRUE(targetClass.IsValid());

  // Test if all structures was generated during `inference strategy all` flow
  ScAddr const & argumentsClass = context.HelperResolveSystemIdtf(CURRENT_NODE_CLASS);
  ScAddrVector const & expectedElements = utils::IteratorUtils::getAllWithType(&context, argumentsClass, ScType::NodeConst);
  ScAddrVector foundElements;
  ScIterator3Ptr const & targetClassIterator = context.Iterator3(
        targetClass,
        ScType::EdgeAccessConstPosPerm,
        ScType::NodeConst);
  for (size_t count = 0; count < 5; count++)
  {
    EXPECT_TRUE(targetClassIterator->Next());
    foundElements.push_back(targetClassIterator->Get(2));
  }

  EXPECT_EQ(foundElements.size(), expectedElements.size());
}

TEST_F(InferenceManagerBuilderTest, SingleUnsuccessfulApplyInference)
{
  ScMemoryContext & context = *m_ctx;

  loader.loadScsFile(context, TEST_FILES_DIR_PATH + "singleUnsuccessfulApplyTest.scs");
  initialize();

  ScAddr const & inputStructure1 = context.HelperResolveSystemIdtf(INPUT_STRUCTURE1);
  ScAddr const & inputStructure2 = context.HelperResolveSystemIdtf(INPUT_STRUCTURE2);
  ScAddrVector inputStructures{inputStructure1, inputStructure2};
  ScAddr const & rulesSet = context.HelperResolveSystemIdtf(RULES_SET);
  ScAddr const & outputStructure = context.CreateNode(ScType::NodeConstStruct);

  InferenceFlowConfig const & inferenceFlowConfig {false, false, false};
  std::unique_ptr<inference::InferenceManagerAbstract> iterationStrategy =
      inference::InferenceManagerFactory::constructDirectInferenceManagerAll(&context, inferenceFlowConfig, inputStructures);

  InferenceParamsConfig const & inferenceParamsConfig {rulesSet, {}, inputStructures, outputStructure};
  bool result = iterationStrategy->applyInference(inferenceParamsConfig);

  EXPECT_FALSE(result);
  ScAddr const & solution = iterationStrategy->getSolutionTreeManager()->createSolution(outputStructure, result);

  EXPECT_TRUE(solution.IsValid());
  EXPECT_TRUE(
        context.HelperCheckEdge(InferenceKeynodes::concept_success_solution, solution, ScType::EdgeAccessConstNegPerm));

  ScAddr const & argument = context.HelperFindBySystemIdtf(ARGUMENT);
  EXPECT_TRUE(argument.IsValid());
  ScAddr const & targetClass = context.HelperFindBySystemIdtf(TARGET_NODE_CLASS);
  EXPECT_TRUE(targetClass.IsValid());

  EXPECT_FALSE(context.HelperCheckEdge(targetClass, argument, ScType::EdgeAccessConstPosPerm));
}

void generateTripleInStructure(ScMemoryContext & context, ScAddr const & set, ScAddr const & element, ScAddr const & structure)
{
  context.CreateEdge(ScType::EdgeAccessConstPosPerm, structure, element);
  ScAddr const & tailElementArc = context.CreateEdge(ScType::EdgeAccessConstPosPerm, set, element);
  context.CreateEdge(ScType::EdgeAccessConstPosPerm, structure, tailElementArc);
}

void generateBodyHeadInStructure(ScMemoryContext & context, ScAddr const & tailElement, ScAddr const & nrelBodyElements, ScAddr const & nrelHead, ScAddr const & structure)
{
  ScAddr const & bodyElementsTuple = context.CreateNode(ScType::NodeConstTuple);
  context.CreateEdge(ScType::EdgeAccessConstPosPerm, structure, bodyElementsTuple);

  ScAddr const & bodyElementsEdge = context.CreateEdge(ScType::EdgeDCommonConst, tailElement, bodyElementsTuple);
  context.CreateEdge(ScType::EdgeAccessConstPosPerm, structure, bodyElementsEdge);

  ScAddr const & bodyElementsArc = context.CreateEdge(ScType::EdgeAccessConstPosPerm, nrelBodyElements, bodyElementsEdge);
  context.CreateEdge(ScType::EdgeAccessConstPosPerm, structure, bodyElementsArc);

  ScAddr const & bodyElement = context.CreateNode(ScType::NodeConst);
  context.CreateEdge(ScType::EdgeAccessConstPosPerm, structure, bodyElement);

  ScAddr const & bodyElementArc = context.CreateEdge(ScType::EdgeAccessConstPosPerm, bodyElementsTuple, bodyElement);
  context.CreateEdge(ScType::EdgeAccessConstPosPerm, structure, bodyElementArc);

  ScAddr const & head = context.CreateNode(ScType::NodeConst);
  context.CreateEdge(ScType::EdgeAccessConstPosPerm, structure, head);

  ScAddr const & headEdge = context.CreateEdge(ScType::EdgeDCommonConst, bodyElement, head);
  context.CreateEdge(ScType::EdgeAccessConstPosPerm, structure, headEdge);

  ScAddr const & headArc = context.CreateEdge(ScType::EdgeAccessConstPosPerm, nrelHead, headEdge);
  context.CreateEdge(ScType::EdgeAccessConstPosPerm, structure, headArc);
}

void generateSnakes(ScMemoryContext & context, size_t const snakesCount, size_t const tailsElementsCount)
{
  ScAddr const & inputStructure = context.HelperFindBySystemIdtf("input_structure1");
  EXPECT_TRUE(inputStructure.IsValid());
  ScAddr const & classTail = context.HelperFindBySystemIdtf("class_tail");
  EXPECT_TRUE(classTail.IsValid());
  ScAddr const & nrelTailElements = context.HelperFindBySystemIdtf("nrel_tail_elements");
  EXPECT_TRUE(nrelTailElements.IsValid());
  ScAddr const & nrelBodyElements = context.HelperFindBySystemIdtf("nrel_body_elements");
  EXPECT_TRUE(nrelBodyElements.IsValid());
  ScAddr const & nrelHead = context.HelperFindBySystemIdtf("nrel_head");
  EXPECT_TRUE(nrelHead.IsValid());

  context.CreateEdge(ScType::EdgeAccessConstPosPerm, inputStructure, classTail);
  context.CreateEdge(ScType::EdgeAccessConstPosPerm, inputStructure, nrelTailElements);
  context.CreateEdge(ScType::EdgeAccessConstPosPerm, inputStructure, nrelBodyElements);
  context.CreateEdge(ScType::EdgeAccessConstPosPerm, inputStructure, nrelHead);

  for (size_t i = 0; i < snakesCount; i++)
  {
    ScAddr tailElement;
    ScAddr const & tail = context.CreateNode(ScType::NodeConst);
    context.CreateEdge(ScType::EdgeAccessConstPosPerm, inputStructure, tail);
    ScAddr const & classEdge = context.CreateEdge(ScType::EdgeAccessConstPosPerm, classTail, tail);
    context.CreateEdge(ScType::EdgeAccessConstPosPerm, inputStructure, classEdge);

    ScAddr const & tailElementsTuple = context.CreateNode(ScType::NodeConstTuple);
    context.CreateEdge(ScType::EdgeAccessConstPosPerm, inputStructure, tailElementsTuple);

    ScAddr const & tailElementsEdge = context.CreateEdge(ScType::EdgeDCommonConst, tail, tailElementsTuple);
    context.CreateEdge(ScType::EdgeAccessConstPosPerm, inputStructure, tailElementsEdge);

    ScAddr const & tailElementsArc = context.CreateEdge(ScType::EdgeAccessConstPosPerm, nrelTailElements, tailElementsEdge);
    context.CreateEdge(ScType::EdgeAccessConstPosPerm, inputStructure, tailElementsArc);

    for (size_t j = 0; j < tailsElementsCount; j++)
    {
      tailElement = context.CreateNode(ScType::NodeConst);
      generateTripleInStructure(context, tailElementsTuple, tailElement, inputStructure);
    }

    generateBodyHeadInStructure(context, tailElement, nrelBodyElements, nrelHead, inputStructure);
  }
}

void generateTails(ScMemoryContext & context, ScAddr const & tail, size_t const tailsCount, size_t const tailsElementsCount)
{
  ScAddr const & inputStructure = context.HelperFindBySystemIdtf("input_structure1");
  EXPECT_TRUE(inputStructure.IsValid());
  ScAddr const & classTail = context.HelperFindBySystemIdtf("class_tail");
  EXPECT_TRUE(classTail.IsValid());
  ScAddr const & nrelTailElements = context.HelperFindBySystemIdtf("nrel_tail_elements");
  EXPECT_TRUE(nrelTailElements.IsValid());
  ScAddr const & nrelBodyElements = context.HelperFindBySystemIdtf("nrel_body_elements");
  EXPECT_TRUE(nrelBodyElements.IsValid());
  ScAddr const & nrelHead = context.HelperFindBySystemIdtf("nrel_head");
  EXPECT_TRUE(nrelHead.IsValid());

  context.CreateEdge(ScType::EdgeAccessConstPosPerm, inputStructure, tail);
  context.CreateEdge(ScType::EdgeAccessConstPosPerm, inputStructure, classTail);
  context.CreateEdge(ScType::EdgeAccessConstPosPerm, inputStructure, nrelTailElements);
  context.CreateEdge(ScType::EdgeAccessConstPosPerm, inputStructure, nrelBodyElements);
  context.CreateEdge(ScType::EdgeAccessConstPosPerm, inputStructure, nrelHead);

  ScAddr const & classEdge = context.CreateEdge(ScType::EdgeAccessConstPosPerm, classTail, tail);
  context.CreateEdge(ScType::EdgeAccessConstPosPerm, inputStructure, classEdge);

  ScAddr const & tailElementsTuple = context.CreateNode(ScType::NodeConst);
  context.CreateEdge(ScType::EdgeAccessConstPosPerm, inputStructure, tailElementsTuple);

  ScAddr const & tailElementsEdge = context.CreateEdge(ScType::EdgeDCommonConst, tail, tailElementsTuple);
  context.CreateEdge(ScType::EdgeAccessConstPosPerm, inputStructure, tailElementsEdge);

  ScAddr const & tailElementsArc = context.CreateEdge(ScType::EdgeAccessConstPosPerm, nrelTailElements, tailElementsEdge);
  context.CreateEdge(ScType::EdgeAccessConstPosPerm, inputStructure, tailElementsArc);

  ScAddr tailElement;
  for (size_t i = 0; i < tailsCount; i++)
  {
    for (size_t j = 0; j < tailsElementsCount; j++)
    {
      tailElement = context.CreateNode(ScType::NodeConst);
      generateTripleInStructure(context, tailElementsTuple, tailElement, inputStructure);
    }

    generateBodyHeadInStructure(context, tailElement, nrelBodyElements, nrelHead, inputStructure);
  }
}

}  // namespace inferenceManagerBuilderTest
