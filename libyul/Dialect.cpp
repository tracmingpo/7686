// SPDX-License-Identifier: GPL-3.0
/**
 * Yul dialect.
 */

#include <libyul/Dialect.h>
#include <libyul/AsmData.h>

using namespace solidity::yul;
using namespace std;
using namespace solidity::langutil;

Literal Dialect::zeroLiteralForType(solidity::yul::YulString _type) const
{
	if (_type == boolType && _type != defaultType)
		return {SourceLocation{}, LiteralKind::Boolean, "false"_yulstring, _type};
	return {SourceLocation{}, LiteralKind::Number, "0"_yulstring, _type};
}

bool Dialect::validTypeForLiteral(LiteralKind _kind, YulString, YulString _type) const
{
	if (_kind == LiteralKind::Boolean)
		return _type == boolType;
	else
		return true;
}

Dialect const& Dialect::yulDeprecated()
{
	static unique_ptr<Dialect> dialect;
	static YulStringRepository::ResetCallback callback{[&] { dialect.reset(); }};

	if (!dialect)
	{
		// TODO will probably change, especially the list of types.
		dialect = make_unique<Dialect>();
		dialect->defaultType = "u256"_yulstring;
		dialect->boolType = "bool"_yulstring;
		dialect->types = {
			"bool"_yulstring,
			"u8"_yulstring,
			"s8"_yulstring,
			"u32"_yulstring,
			"s32"_yulstring,
			"u64"_yulstring,
			"s64"_yulstring,
			"u128"_yulstring,
			"s128"_yulstring,
			"u256"_yulstring,
			"s256"_yulstring
		};
	};

	return *dialect;
}
