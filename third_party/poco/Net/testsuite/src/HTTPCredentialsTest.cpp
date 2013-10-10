//
// HTTPCredentialsTest.cpp
//
// $Id: //poco/1.4/Net/testsuite/src/HTTPCredentialsTest.cpp#3 $
//
// Copyright (c) 2005-2006, Applied Informatics Software Engineering GmbH.
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


#include "HTTPCredentialsTest.h"
#include "CppUnit/TestCaller.h"
#include "CppUnit/TestSuite.h"
#include "Poco/Net/HTTPRequest.h"
#include "Poco/Net/HTTPResponse.h"
#include "Poco/Net/HTTPBasicCredentials.h"
#include "Poco/Net/HTTPAuthenticationParams.h"
#include "Poco/Net/HTTPDigestCredentials.h"
#include "Poco/Net/HTTPCredentials.h"
#include "Poco/Net/NetException.h"
#include "Poco/URI.h"


using Poco::Net::HTTPRequest;
using Poco::Net::HTTPResponse;
using Poco::Net::HTTPBasicCredentials;
using Poco::Net::HTTPAuthenticationParams;
using Poco::Net::HTTPDigestCredentials;
using Poco::Net::HTTPCredentials;
using Poco::Net::NotAuthenticatedException;


HTTPCredentialsTest::HTTPCredentialsTest(const std::string& name): CppUnit::TestCase(name)
{
}


HTTPCredentialsTest::~HTTPCredentialsTest()
{
}


void HTTPCredentialsTest::testBasicCredentials()
{
	HTTPRequest request;
	assert (!request.hasCredentials());
	
	HTTPBasicCredentials cred("user", "secret");
	cred.authenticate(request);
	assert (request.hasCredentials());
	std::string scheme;
	std::string info;
	request.getCredentials(scheme, info);
	assert (scheme == "Basic");
	assert (info == "dXNlcjpzZWNyZXQ=");
	
	HTTPBasicCredentials cred2(request);
	assert (cred2.getUsername() == "user");
	assert (cred2.getPassword() == "secret");
}


void HTTPCredentialsTest::testProxyBasicCredentials()
{
	HTTPRequest request;
	assert (!request.hasProxyCredentials());
	
	HTTPBasicCredentials cred("user", "secret");
	cred.proxyAuthenticate(request);
	assert (request.hasProxyCredentials());
	std::string scheme;
	std::string info;
	request.getProxyCredentials(scheme, info);
	assert (scheme == "Basic");
	assert (info == "dXNlcjpzZWNyZXQ=");
}


void HTTPCredentialsTest::testBadCredentials()
{
	HTTPRequest request;
	
	std::string scheme;
	std::string info;
	try
	{
		request.getCredentials(scheme, info);
		fail("no credentials - must throw");
	}
	catch (NotAuthenticatedException&)
	{
	}
	
	request.setCredentials("Test", "SomeData");
	request.getCredentials(scheme, info);
	assert (scheme == "Test");
	assert (info == "SomeData");
	
	try
	{
		HTTPBasicCredentials cred(request);
		fail("bad scheme - must throw");
	}
	catch (NotAuthenticatedException&)
	{
	}
}


void HTTPCredentialsTest::testAuthenticationParams()
{
	const std::string authInfo("nonce=\"212573bb90170538efad012978ab811f%lu\", realm=\"TestDigest\", response=\"40e4889cfbd0e561f71e3107a2863bc4\", uri=\"/digest/\", username=\"user\"");
	HTTPAuthenticationParams params(authInfo);
	
	assert (params["nonce"] == "212573bb90170538efad012978ab811f%lu");
	assert (params["realm"] == "TestDigest");
	assert (params["response"] == "40e4889cfbd0e561f71e3107a2863bc4");
	assert (params["uri"] == "/digest/");
	assert (params["username"] == "user");
	assert (params.size() == 5);
	assert (params.toString() == authInfo);
	
	params.clear();
	HTTPRequest request;
	request.set("Authorization", "Digest " + authInfo);
	params.fromRequest(request);

	assert (params["nonce"] == "212573bb90170538efad012978ab811f%lu");
	assert (params["realm"] == "TestDigest");
	assert (params["response"] == "40e4889cfbd0e561f71e3107a2863bc4");
	assert (params["uri"] == "/digest/");
	assert (params["username"] == "user");
	assert (params.size() == 5);

	params.clear();
	HTTPResponse response;
	response.set("WWW-Authenticate", "Digest realm=\"TestDigest\", nonce=\"212573bb90170538efad012978ab811f%lu\"");	
	params.fromResponse(response);
	
	assert (params["realm"] == "TestDigest");
	assert (params["nonce"] == "212573bb90170538efad012978ab811f%lu");
	assert (params.size() == 2);
}


void HTTPCredentialsTest::testDigestCredentials()
{
	HTTPDigestCredentials creds("user", "s3cr3t");
	HTTPRequest request(HTTPRequest::HTTP_GET, "/digest/");
	HTTPResponse response;
	response.set("WWW-Authenticate", "Digest realm=\"TestDigest\", nonce=\"212573bb90170538efad012978ab811f%lu\"");	
	creds.authenticate(request, response);
	assert (request.get("Authorization") == "Digest nonce=\"212573bb90170538efad012978ab811f%lu\", realm=\"TestDigest\", response=\"40e4889cfbd0e561f71e3107a2863bc4\", uri=\"/digest/\", username=\"user\"");
}


void HTTPCredentialsTest::testDigestCredentialsQoP()
{
	HTTPDigestCredentials creds("user", "s3cr3t");
	HTTPRequest request(HTTPRequest::HTTP_GET, "/digest/");
	HTTPResponse response;
	response.set("WWW-Authenticate", "Digest realm=\"TestDigest\", nonce=\"212573bb90170538efad012978ab811f%lu\", opaque=\"opaque\", qop=\"auth,auth-int\"");	
	creds.authenticate(request, response);
	
	HTTPAuthenticationParams params(request);
	assert (params["nonce"] == "212573bb90170538efad012978ab811f%lu");
	assert (params["realm"] == "TestDigest");
	assert (params["response"] != "40e4889cfbd0e561f71e3107a2863bc4");
	assert (params["uri"] == "/digest/");
	assert (params["username"] == "user");
	assert (params["opaque"] == "opaque");
	assert (params["cnonce"] != "");
	assert (params["nc"] == "00000001");
	assert (params["qop"] == "auth");
	assert (params.size() == 9);
	
	std::string cnonce = params["cnonce"];
	std::string aresp = params["response"];
	
	params.clear();
	
	creds.updateAuthInfo(request);
	params.fromRequest(request);
	assert (params["nonce"] == "212573bb90170538efad012978ab811f%lu");
	assert (params["realm"] == "TestDigest");
	assert (params["response"] != aresp);
	assert (params["uri"] == "/digest/");
	assert (params["username"] == "user");
	assert (params["opaque"] == "opaque");
	assert (params["cnonce"] == cnonce);
	assert (params["nc"] == "00000002");
	assert (params["qop"] == "auth");
	assert (params.size() == 9);
}


void HTTPCredentialsTest::testCredentialsBasic()
{
	HTTPCredentials creds("user", "s3cr3t");
	HTTPRequest request(HTTPRequest::HTTP_GET, "/basic/");
	HTTPResponse response;
	response.set("WWW-Authenticate", "Basic realm=\"TestBasic\"");	
	creds.authenticate(request, response);	
	assert (request.get("Authorization") == "Basic dXNlcjpzM2NyM3Q=");
}


void HTTPCredentialsTest::testProxyCredentialsBasic()
{
	HTTPCredentials creds("user", "s3cr3t");
	HTTPRequest request(HTTPRequest::HTTP_GET, "/basic/");
	HTTPResponse response;
	response.set("Proxy-Authenticate", "Basic realm=\"TestBasic\"");	
	creds.proxyAuthenticate(request, response);	
	assert (request.get("Proxy-Authorization") == "Basic dXNlcjpzM2NyM3Q=");
}


void HTTPCredentialsTest::testCredentialsDigest()
{
	HTTPCredentials creds("user", "s3cr3t");
	HTTPRequest request(HTTPRequest::HTTP_GET, "/digest/");
	HTTPResponse response;
	response.set("WWW-Authenticate", "Digest realm=\"TestDigest\", nonce=\"212573bb90170538efad012978ab811f%lu\"");	
	creds.authenticate(request, response);	
	assert (request.get("Authorization") == "Digest nonce=\"212573bb90170538efad012978ab811f%lu\", realm=\"TestDigest\", response=\"40e4889cfbd0e561f71e3107a2863bc4\", uri=\"/digest/\", username=\"user\"");
}


void HTTPCredentialsTest::testProxyCredentialsDigest()
{
	HTTPCredentials creds("user", "s3cr3t");
	HTTPRequest request(HTTPRequest::HTTP_GET, "/digest/");
	HTTPResponse response;
	response.set("Proxy-Authenticate", "Digest realm=\"TestDigest\", nonce=\"212573bb90170538efad012978ab811f%lu\"");	
	creds.proxyAuthenticate(request, response);	
	assert (request.get("Proxy-Authorization") == "Digest nonce=\"212573bb90170538efad012978ab811f%lu\", realm=\"TestDigest\", response=\"40e4889cfbd0e561f71e3107a2863bc4\", uri=\"/digest/\", username=\"user\"");
}


void HTTPCredentialsTest::testExtractCredentials()
{
	Poco::URI uri("http://user:s3cr3t@host.com/");
	std::string username;
	std::string password;
	HTTPCredentials::extractCredentials(uri, username, password);
	assert (username == "user");
	assert (password == "s3cr3t");
}


void HTTPCredentialsTest::testVerifyAuthInfo()
{
	HTTPDigestCredentials creds("user", "s3cr3t");
	HTTPRequest request(HTTPRequest::HTTP_GET, "/digest/");
	HTTPResponse response;
	response.set("WWW-Authenticate", "Digest realm=\"TestDigest\", nonce=\"212573bb90170538efad012978ab811f%lu\"");	
	creds.authenticate(request, response);
	assert (creds.verifyAuthInfo(request));
	
	request.set("Authorization", "Digest nonce=\"212573bb90170538efad012978ab811f%lu\", realm=\"TestDigest\", response=\"xxe4889cfbd0e561f71e3107a2863bc4\", uri=\"/digest/\", username=\"user\"");
	assert (!creds.verifyAuthInfo(request));
}


void HTTPCredentialsTest::testVerifyAuthInfoQoP()
{
	HTTPDigestCredentials creds("user", "s3cr3t");
	HTTPRequest request(HTTPRequest::HTTP_GET, "/digest/");
	HTTPResponse response;
	response.set("WWW-Authenticate", "Digest realm=\"TestDigest\", nonce=\"212573bb90170538efad012978ab811f%lu\", opaque=\"opaque\", qop=\"auth,auth-int\"");	
	creds.authenticate(request, response);
	assert (creds.verifyAuthInfo(request));
	
	request.set("Authorization", "Digest cnonce=\"f9c80ffd1c3bc4ee47ed92b704ba75a4\", nc=00000001, nonce=\"212573bb90170538efad012978ab811f%lu\", opaque=\"opaque\", qop=\"auth\", realm=\"TestDigest\", response=\"ff0e90b9aa019120ea0ed6e23ce95d9a\", uri=\"/digest/\", username=\"user\"");
	assert (!creds.verifyAuthInfo(request));
}


void HTTPCredentialsTest::setUp()
{
}


void HTTPCredentialsTest::tearDown()
{
}


CppUnit::Test* HTTPCredentialsTest::suite()
{
	CppUnit::TestSuite* pSuite = new CppUnit::TestSuite("HTTPCredentialsTest");

	CppUnit_addTest(pSuite, HTTPCredentialsTest, testBasicCredentials);
	CppUnit_addTest(pSuite, HTTPCredentialsTest, testProxyBasicCredentials);
	CppUnit_addTest(pSuite, HTTPCredentialsTest, testBadCredentials);
	CppUnit_addTest(pSuite, HTTPCredentialsTest, testAuthenticationParams);
	CppUnit_addTest(pSuite, HTTPCredentialsTest, testDigestCredentials);
	CppUnit_addTest(pSuite, HTTPCredentialsTest, testDigestCredentialsQoP);
	CppUnit_addTest(pSuite, HTTPCredentialsTest, testCredentialsBasic);
	CppUnit_addTest(pSuite, HTTPCredentialsTest, testProxyCredentialsBasic);
	CppUnit_addTest(pSuite, HTTPCredentialsTest, testCredentialsDigest);
	CppUnit_addTest(pSuite, HTTPCredentialsTest, testProxyCredentialsDigest);
	CppUnit_addTest(pSuite, HTTPCredentialsTest, testExtractCredentials);
	CppUnit_addTest(pSuite, HTTPCredentialsTest, testVerifyAuthInfo);
	CppUnit_addTest(pSuite, HTTPCredentialsTest, testVerifyAuthInfoQoP);

	return pSuite;
}