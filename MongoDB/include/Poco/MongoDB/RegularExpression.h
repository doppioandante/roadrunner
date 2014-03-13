//
// RegularExpression.h
//
// $Id$
//
// Library: MongoDB
// Package: MongoDB
// Module:  RegularExpression
//
// Definition of the RegularExpression class.
//
// Copyright (c) 2012, Applied Informatics Software Engineering GmbH.
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


#ifndef MongoDB_RegularExpression_INCLUDED
#define MongoDB_RegularExpression_INCLUDED


#include "Poco/RegularExpression.h"
#include "Poco/MongoDB/MongoDB.h"
#include "Poco/MongoDB/Element.h"


namespace Poco {
namespace MongoDB {


class MongoDB_API RegularExpression
	/// Represents a regular expression in BSON format
{
public:
	typedef SharedPtr<RegularExpression> Ptr;

	RegularExpression();
		/// Constructor

	RegularExpression(const std::string& pattern, const std::string& options);
		/// Constructor

	virtual ~RegularExpression();
		/// Destructor

	SharedPtr<Poco::RegularExpression> createRE() const;
		/// Tries to create a Poco::RegularExpression

	std::string getOptions() const;
		/// Returns the options

	void setOptions(const std::string& options);
		/// Sets the options

	std::string getPattern() const;
		/// Returns the pattern

	void setPattern(const std::string& pattern);
		/// Sets the pattern

private:
	std::string _pattern;
	std::string _options;
};


inline std::string RegularExpression::getPattern() const
{
	return _pattern;
}


inline void RegularExpression::setPattern(const std::string& pattern)
{
	_pattern = pattern;
}


inline std::string RegularExpression::getOptions() const
{
	return _options;
}


inline void RegularExpression::setOptions(const std::string& options)
{
	_options = options;
}


// BSON Regex
// spec: cstring cstring
template<>
struct ElementTraits<RegularExpression::Ptr>
{
	enum { TypeId = 0x0B };

	static std::string toString(const RegularExpression::Ptr& value, int indent = 0)
	{
		//TODO
		return "RE: not implemented yet";
	}
};


template<>
inline void BSONReader::read<RegularExpression::Ptr>(RegularExpression::Ptr& to)
{
	std::string pattern = readCString();
	std::string options = readCString();

	to = new RegularExpression(pattern, options);
}


template<>
inline void BSONWriter::write<RegularExpression::Ptr>(RegularExpression::Ptr& from)
{
	writeCString(from->getPattern());
	writeCString(from->getOptions());
}


} } // namespace Poco::MongoDB


#endif //  MongoDB_RegularExpression_INCLUDED
