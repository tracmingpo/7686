// SPDX-License-Identifier: GPL-3.0
/** @file Whiskers.cpp
 * @author Chris <chis@ethereum.org>
 * @date 2017
 *
 * Moustache-like templates.
 */

#include <libsolutil/Whiskers.h>

#include <libsolutil/Assertions.h>

#include <regex>

using namespace std;
using namespace solidity::util;

Whiskers::Whiskers(string _template):
	m_template(move(_template))
{
}

Whiskers& Whiskers::operator()(string _parameter, string _value)
{
	checkParameterValid(_parameter);
	checkParameterUnknown(_parameter);
	m_parameters[move(_parameter)] = move(_value);
	return *this;
}

Whiskers& Whiskers::operator()(string _parameter, bool _value)
{
	checkParameterValid(_parameter);
	checkParameterUnknown(_parameter);
	m_conditions[move(_parameter)] = _value;
	return *this;
}

Whiskers& Whiskers::operator()(
	string _listParameter,
	vector<map<string, string>> _values
)
{
	checkParameterValid(_listParameter);
	checkParameterUnknown(_listParameter);
	for (auto const& element: _values)
		for (auto const& val: element)
			checkParameterValid(val.first);
	m_listParameters[move(_listParameter)] = move(_values);
	return *this;
}

string Whiskers::render() const
{
	return replace(m_template, m_parameters, m_conditions, m_listParameters);
}

void Whiskers::checkParameterValid(string const& _parameter) const
{
	static regex validParam("^" + paramRegex() + "$");
	assertThrow(
		regex_match(_parameter, validParam),
		WhiskersError,
		"Parameter" + _parameter + " contains invalid characters."
	);
}

void Whiskers::checkParameterUnknown(string const& _parameter) const
{
	assertThrow(
		!m_parameters.count(_parameter),
		WhiskersError,
		_parameter + " already set as value parameter."
	);
	assertThrow(
		!m_conditions.count(_parameter),
		WhiskersError,
		_parameter + " already set as condition parameter."
	);
	assertThrow(
		!m_listParameters.count(_parameter),
		WhiskersError,
		_parameter + " already set as list parameter."
	);
}

namespace
{
template<class ReplaceCallback>
string regex_replace(
	string const& _source,
	regex const& _pattern,
	ReplaceCallback _replace,
	regex_constants::match_flag_type _flags = regex_constants::match_default
)
{
	sregex_iterator curMatch(_source.begin(), _source.end(), _pattern, _flags);
	sregex_iterator matchEnd;
	string::const_iterator lastMatchedPos(_source.cbegin());
	string result;
	while (curMatch != matchEnd)
	{
		result.append(curMatch->prefix().first, curMatch->prefix().second);
		result.append(_replace(*curMatch));
		lastMatchedPos = (*curMatch)[0].second;
		++curMatch;
	}
	result.append(lastMatchedPos, _source.cend());
	return result;
}
}

string Whiskers::replace(
	string const& _template,
	StringMap const& _parameters,
	map<string, bool> const& _conditions,
	map<string, vector<StringMap>> const& _listParameters
)
{
	static regex listOrTag(
		"<(" + paramRegex() + ")>|"
		"<#(" + paramRegex() + ")>((?:.|\\r|\\n)*?)</\\2>|"
		"<\\?(\\+?" + paramRegex() + ")>((?:.|\\r|\\n)*?)(<!\\4>((?:.|\\r|\\n)*?))?</\\4>"
	);
	return regex_replace(_template, listOrTag, [&](match_results<string::const_iterator> _match) -> string
	{
		string tagName(_match[1]);
		string listName(_match[2]);
		string conditionName(_match[4]);
		if (!tagName.empty())
		{
			assertThrow(
				_parameters.count(tagName),
				WhiskersError,
				"Value for tag " + tagName + " not provided.\n" +
				"Template:\n" +
				_template
			);
			return _parameters.at(tagName);
		}
		else if (!listName.empty())
		{
			string templ(_match[3]);
			assertThrow(
				_listParameters.count(listName),
				WhiskersError, "List parameter " + listName + " not set."
			);
			string replacement;
			for (auto const& parameters: _listParameters.at(listName))
				replacement += replace(templ, joinMaps(_parameters, parameters), _conditions);
			return replacement;
		}
		else
		{
			assertThrow(!conditionName.empty(), WhiskersError, "");
			bool conditionValue = false;
			if (conditionName[0] == '+')
			{
				string tag = conditionName.substr(1);
				assertThrow(
					_parameters.count(tag),
					WhiskersError, "Tag " + tag + " used as condition but was not set."
				);
				conditionValue = !_parameters.at(tag).empty();
			}
			else
			{
				assertThrow(
					_conditions.count(conditionName),
					WhiskersError, "Condition parameter " + conditionName + " not set."
				);
				conditionValue = _conditions.at(conditionName);
			}
			return replace(
				conditionValue ? _match[5] : _match[7],
				_parameters,
				_conditions,
				_listParameters
			);
		}
	});
}

Whiskers::StringMap Whiskers::joinMaps(
	Whiskers::StringMap const& _a,
	Whiskers::StringMap const& _b
)
{
	Whiskers::StringMap ret = _a;
	for (auto const& x: _b)
		assertThrow(
			ret.insert(x).second,
			WhiskersError,
			"Parameter collision"
		);
	return ret;
}

