//
// AttributesParser.h
//
// $Id: //poco/1.4/CppParser/include/Poco/CppParser/AttributesParser.h#2 $
//
// Library: CppParser
// Package: Attributes
// Module:  AttributesParser
//
// Definition of the AttributesParser class.
//
// Copyright (c) 2006, Applied Informatics Software Engineering GmbH.
// and Contributors.
//
// Permission is hereby granted, free of charge, to any person or organization
// obtaining a copy of the software and accompanying documentation covered by
// this license (the "Software") to use, reproduce, display, distribute,
// execute, and transmit the Software, and to prepare derivative works of the
// Software, and to permit third-parties to whom the Software is furnished to
// do so, all subject to the following:
// 
// The copyright notices in the Software and this entire statement, including
// the above license grant, this restriction and the following disclaimer,
// must be included in all copies of the Software, in whole or in part, and
// all derivative works of the Software, unless such copies or derivative
// works are solely in the form of machine-executable object code generated by
// a source language processor.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
// SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
// FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
// ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
//


#ifndef CppParser_AttributesParser_INCLUDED
#define CppParser_AttributesParser_INCLUDED


#include "Poco/CppParser/CppParser.h"
#include "Poco/CppParser/Tokenizer.h"
#include "Poco/CppParser/Attributes.h"


namespace Poco {
namespace CppParser {


class CppParser_API AttributesParser
	/// A parser for POCO-style C++ attributes.
	///
	/// Using a special comment syntax, C++ declarations for
	/// structs/classes, functions, types, etc. can be annotated
	/// with attributes.
	///
	/// Attributes always come immediately before the symbol that 
	/// is being annotated, and are written inside special comments
	/// with the syntax:
	///     //@ <attrDecl>[,<attrDec>...]
	/// where <attrDecl> is
	///     <name>[=<value>]
	/// <name> is a valid C++ identifier, or two identifiers separated by 
	/// a period (struct accessor notation).
	/// <value> is a string, integer, identifier, bool literal, or a complex value
	/// in the form
	///    {<name>=<value>[,<name>=<value>...]}
{
public:
	AttributesParser(Attributes& attrs, std::istream& istr);
		/// Creates the AttributesParser.

	~AttributesParser();
		/// Destroys the AttributesParser.

	void parse();
		/// Parses attributes.

protected:
	void setAttribute(const std::string& name, const std::string& value);
	const Poco::Token* parseAttributes(const Poco::Token* pNext);
	const Poco::Token* parseAttribute(const Poco::Token* pNext);
	const Poco::Token* parseComplexAttribute(const Token* pNext, const std::string& id);
	const Poco::Token* parseIdentifier(const Poco::Token* pNext, std::string& id);
	const Poco::Token* next();
	static bool isIdentifier(const Poco::Token* pToken);
	static bool isOperator(const Poco::Token* pToken, int kind);
	static bool isLiteral(const Poco::Token* pToken);
	static bool isEOF(const Poco::Token* pToken);
	
private:
	Attributes& _attrs;
	Tokenizer   _tokenizer;
	std::string _id;
};


//
// inlines
//
inline const Poco::Token* AttributesParser::next()
{
	return _tokenizer.next();
}


inline bool AttributesParser::isEOF(const Poco::Token* pToken)
{
	return pToken->is(Token::EOF_TOKEN);
}


inline bool AttributesParser::isIdentifier(const Poco::Token* pToken)
{
	return pToken->is(Poco::Token::IDENTIFIER_TOKEN) || pToken->is(Poco::Token::KEYWORD_TOKEN);
}


inline bool AttributesParser::isOperator(const Poco::Token* pToken, int kind)
{
	return pToken->is(Poco::Token::OPERATOR_TOKEN) && pToken->asInteger() == kind;
}


inline bool AttributesParser::isLiteral(const Poco::Token* pToken)
{
	return pToken->is(Poco::Token::STRING_LITERAL_TOKEN) || pToken->is(Poco::Token::INTEGER_LITERAL_TOKEN);
}


} } // namespace Poco::CppParser


#endif // CppParser_AttributesParser_INCLUDED