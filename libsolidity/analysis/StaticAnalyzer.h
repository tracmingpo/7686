// SPDX-License-Identifier: GPL-3.0
/**
 * @author Federico Bond <federicobond@gmail.com>
 * @date 2016
 * Static analyzer and checker.
 */

#pragma once

#include <libsolidity/analysis/TypeChecker.h>
#include <libsolidity/ast/Types.h>
#include <libsolidity/ast/ASTAnnotations.h>
#include <libsolidity/ast/ASTForward.h>
#include <libsolidity/ast/ASTVisitor.h>

namespace solidity::langutil
{
class ErrorReporter;
}

namespace solidity::frontend
{

class ConstructorUsesAssembly;


/**
 * The module that performs static analysis on the AST.
 * In this context, static analysis is anything that can produce warnings which can help
 * programmers write cleaner code. For every warning generated here, it has to be possible to write
 * equivalent code that does not generate the warning.
 */
class StaticAnalyzer: private ASTConstVisitor
{
public:
	/// @param _errorReporter provides the error logging functionality.
	explicit StaticAnalyzer(langutil::ErrorReporter& _errorReporter);
	~StaticAnalyzer() override;

	/// Performs static analysis on the given source unit and all of its sub-nodes.
	/// @returns true iff all checks passed. Note even if all checks passed, errors() can still contain warnings
	bool analyze(SourceUnit const& _sourceUnit);

private:

	bool visit(ContractDefinition const& _contract) override;
	void endVisit(ContractDefinition const& _contract) override;

	bool visit(FunctionDefinition const& _function) override;
	void endVisit(FunctionDefinition const& _function) override;

	bool visit(ExpressionStatement const& _statement) override;
	bool visit(VariableDeclaration const& _variable) override;
	bool visit(Identifier const& _identifier) override;
	bool visit(Return const& _return) override;
	bool visit(MemberAccess const& _memberAccess) override;
	bool visit(InlineAssembly const& _inlineAssembly) override;
	bool visit(BinaryOperation const& _operation) override;
	bool visit(FunctionCall const& _functionCall) override;

	struct TypeComp
	{
		bool operator()(Type const* lhs, Type const* rhs) const
		{
			solAssert(lhs && rhs, "");
			return lhs->richIdentifier() < rhs->richIdentifier();
		}
	};
	using TypeSet = std::set<Type const*, TypeComp>;

	/// @returns the size of this type in storage, including all sub-types.
	static bigint structureSizeEstimate(
		Type const& _type,
		std::set<StructDefinition const*>& _structsSeen,
		TypeSet& _oversizedSubTypes
	);

	langutil::ErrorReporter& m_errorReporter;

	/// Flag that indicates whether the current contract definition is a library.
	bool m_library = false;

	/// Number of uses of each (named) local variable in a function, counter is initialized with zero.
	/// Pairs of AST ids and pointers are used as keys to ensure a deterministic order
	/// when traversing.
	std::map<std::pair<size_t, VariableDeclaration const*>, int> m_localVarUseCount;

	/// Cache that holds information about whether a contract's constructor
	/// uses inline assembly.
	std::unique_ptr<ConstructorUsesAssembly> m_constructorUsesAssembly;

	FunctionDefinition const* m_currentFunction = nullptr;

	/// Flag that indicates a constructor.
	bool m_constructor = false;

	/// Current contract.
	ContractDefinition const* m_currentContract = nullptr;
};

}
