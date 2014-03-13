//
// Preparator.cpp
//
// $Id: //poco/Main/Data/testsuite/src/Preparator.cpp#3 $
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


#include "Preparator.h"
#include "Poco/Data/LOB.h"
#include "Poco/Exception.h"


namespace Poco {
namespace Data {
namespace Test {


Preparator::Preparator()
{
}


Preparator::~Preparator()
{
}


void Preparator::prepare(std::size_t pos, const Poco::Int8&)
{
}


void Preparator::prepare(std::size_t pos, const Poco::UInt8&)
{
}


void Preparator::prepare(std::size_t pos, const Poco::Int16&)
{
}


void Preparator::prepare(std::size_t pos, const Poco::UInt16&)
{
}


void Preparator::prepare(std::size_t pos, const Poco::Int32&)
{
}


void Preparator::prepare(std::size_t pos, const Poco::UInt32&)
{
}


void Preparator::prepare(std::size_t pos, const Poco::Int64&)
{
}


void Preparator::prepare(std::size_t pos, const Poco::UInt64&)
{
}


#ifndef POCO_LONG_IS_64_BIT
void Preparator::prepare(std::size_t pos, const long&)
{
}


void Preparator::prepare(std::size_t pos, const unsigned long&)
{
}
#endif


void Preparator::prepare(std::size_t pos, const bool&)
{
}


void Preparator::prepare(std::size_t pos, const float&)
{
}


void Preparator::prepare(std::size_t pos, const double&)
{
}


void Preparator::prepare(std::size_t pos, const char&)
{
}


void Preparator::prepare(std::size_t pos, const std::string&)
{
}


void Preparator::prepare(std::size_t pos, const Poco::Data::BLOB&)
{
}


void Preparator::prepare(std::size_t pos, const Poco::Data::CLOB&)
{
}


void Preparator::prepare(std::size_t pos, const Poco::Data::Date&)
{
}


void Preparator::prepare(std::size_t pos, const Poco::Data::Time&)
{
}


void Preparator::prepare(std::size_t pos, const Poco::DateTime&)
{
}


void Preparator::prepare(std::size_t pos, const Poco::Any&)
{
}


void Preparator::prepare(std::size_t pos, const Poco::Dynamic::Var&)
{
}


} } } // namespace Poco::Data::Test
