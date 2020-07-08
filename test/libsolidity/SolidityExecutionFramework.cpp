// SPDX-License-Identifier: GPL-3.0
/**
 * @author Christian <c@ethdev.com>
 * @date 2016
 * Framework for executing Solidity contracts and testing them against C++ implementation.
 */

#include <cstdlib>
#include <iostream>
#include <boost/test/framework.hpp>
#include <test/libsolidity/SolidityExecutionFramework.h>

using namespace solidity;
using namespace solidity::test;
using namespace solidity::frontend;
using namespace solidity::frontend::test;
using namespace std;

bytes SolidityExecutionFramework::multiSourceCompileContract(
	map<string, string> const& _sourceCode,
	string const& _contractName,
	map<string, Address> const& _libraryAddresses
)
{
	map<string, string> sourcesWithPreamble = _sourceCode;
	for (auto& entry: sourcesWithPreamble)
		entry.second = addPreamble(entry.second);

	m_compiler.reset();
	m_compiler.setSources(sourcesWithPreamble);
	m_compiler.setLibraries(_libraryAddresses);
	m_compiler.setRevertStringBehaviour(m_revertStrings);
	m_compiler.setEVMVersion(m_evmVersion);
	m_compiler.setOptimiserSettings(m_optimiserSettings);
	m_compiler.enableIRGeneration(m_compileViaYul);
	m_compiler.setRevertStringBehaviour(m_revertStrings);
	if (!m_compiler.compile())
	{
		langutil::SourceReferenceFormatter formatter(std::cerr);

		for (auto const& error: m_compiler.errors())
			formatter.printErrorInformation(*error);
		BOOST_ERROR("Compiling contract failed");
	}
	std::string contractName(_contractName.empty() ? m_compiler.lastContractName() : _contractName);
	evmasm::LinkerObject obj;
	if (m_compileViaYul)
	{
		yul::AssemblyStack asmStack(
			m_evmVersion,
			yul::AssemblyStack::Language::StrictAssembly,
			// Ignore optimiser settings here because we need Yul optimisation to
			// get code that does not exhaust the stack.
			OptimiserSettings::full()
		);
		bool analysisSuccessful = asmStack.parseAndAnalyze("", m_compiler.yulIROptimized(contractName));
		solAssert(analysisSuccessful, "Code that passed analysis in CompilerStack can't have errors");

		asmStack.optimize();
		obj = std::move(*asmStack.assemble(yul::AssemblyStack::Machine::EVM).bytecode);
	}
	else
		obj = m_compiler.object(contractName);
	BOOST_REQUIRE(obj.linkReferences.empty());
	if (m_showMetadata)
		cout << "metadata: " << m_compiler.metadata(contractName) << endl;
	return obj.bytecode;
}

bytes SolidityExecutionFramework::compileContract(
	string const& _sourceCode,
	string const& _contractName,
	map<string, Address> const& _libraryAddresses
)
{
	return multiSourceCompileContract(
		{{"", _sourceCode}},
		_contractName,
		_libraryAddresses
	);
}

string SolidityExecutionFramework::addPreamble(string const& _sourceCode)
{
	// Silence compiler version warning
	string preamble = "pragma solidity >=0.0;\n";
	if (
		solidity::test::CommonOptions::get().useABIEncoderV2 &&
		_sourceCode.find("pragma experimental ABIEncoderV2;") == string::npos
	)
		preamble += "pragma experimental ABIEncoderV2;\n";
	return preamble + _sourceCode;
}
