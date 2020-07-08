// SPDX-License-Identifier: GPL-3.0
/**
 * Optimiser component that changes the code so that it consists of a block starting with
 * a single block followed only by function definitions and with no functions defined
 * anywhere else.
 */

#include <libyul/optimiser/FunctionHoister.h>
#include <libyul/optimiser/OptimizerUtilities.h>
#include <libyul/AsmData.h>

#include <libsolutil/CommonData.h>

using namespace std;
using namespace solidity;
using namespace solidity::yul;

void FunctionHoister::operator()(Block& _block)
{
	bool topLevel = m_isTopLevel;
	m_isTopLevel = false;
	for (auto&& statement: _block.statements)
	{
		std::visit(*this, statement);
		if (holds_alternative<FunctionDefinition>(statement))
		{
			m_functions.emplace_back(std::move(statement));
			statement = Block{_block.location, {}};
		}
	}
	removeEmptyBlocks(_block);
	if (topLevel)
		_block.statements += std::move(m_functions);
}
