/*
* This source file is part of an OSTIS project. For the latest info, see http://ostis.net
* Distributed under the MIT License
* (See accompanying file COPYING.MIT or copy at http://opensource.org/licenses/MIT)
 */

#pragma once
#include "sc-memory/sc_addr.hpp"
#include "sc-memory/sc_object.hpp"
#include "SolutionKeynodes.generated.hpp"
namespace solutionModule
{
class SolutionKeynodes : public ScObject
{
  SC_CLASS()
  SC_GENERATED_BODY()
public:
  SC_PROPERTY(Keynode("action_delete_solution"), ForceCreate)
  static ScAddr action_delete_solution;
};
}  // namespace solutionModule
