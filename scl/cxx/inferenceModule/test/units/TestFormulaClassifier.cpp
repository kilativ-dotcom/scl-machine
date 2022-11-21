/*
 * This source file is part of an OSTIS project. For the latest info, see http://ostis.net
 * Distributed under the MIT License
 * (See accompanying file COPYING.MIT or copy at http://opensource.org/licenses/MIT)
 */

#include "sc_test.hpp"
#include "scs_loader.hpp"

#include "sc-agents-common/keynodes/coreKeynodes.hpp"

#include "keynodes/InferenceKeynodes.hpp"

#include "classifier/FormulaClassifier.hpp"

using namespace inference;

namespace formulaClassifierTest
{
ScsLoader loader;
std::string const TEST_FILES_DIR_PATH = TEMPLATE_SEARCH_MODULE_TEST_SRC_PATH "/testStructures/FormulaClassifierModule/";

using FormulaClassifierTest = ScMemoryTest;

void initialize()
{
  InferenceKeynodes::InitGlobal();
  scAgentsCommon::CoreKeynodes::InitGlobal();
}

TEST_F(FormulaClassifierTest, RuleIsImplication)
{
  ScMemoryContext context(sc_access_lvl_make_min, "implication_detected");
  FormulaClassifier fc(&context);

  loader.loadScsFile(context, TEST_FILES_DIR_PATH + "inferenceLogicTrueComplexRuleTest.scs");
  initialize();

  ScAddr testRule = context.HelperResolveSystemIdtf("inference_logic_test_rule");
  ScAddr rrel_main_key_sc_element = context.HelperResolveSystemIdtf("rrel_main_key_sc_element");
  ScIterator5Ptr iter5 = context.Iterator5(
        testRule,
        ScType::EdgeAccessConstPosPerm,
        ScType::Unknown,
        ScType::EdgeAccessConstPosPerm,
        rrel_main_key_sc_element);
  if (iter5->Next())
  {
    ScAddr formula = iter5->Get(2);
    EXPECT_EQ(fc.typeOfFormula(formula), FormulaClassifier::IMPLICATION_EDGE);
    ScAddr begin;
    ScAddr end;
    context.GetEdgeInfo(formula, begin, end);
    EXPECT_EQ(fc.typeOfFormula(begin), FormulaClassifier::CONJUNCTION);
    EXPECT_EQ(fc.typeOfFormula(end), FormulaClassifier::ATOM);
  }
  else
    SC_LOG_DEBUG("Cannot find main key sc element");

  context.Destroy();
}

}  // namespace formulaClassifierTest