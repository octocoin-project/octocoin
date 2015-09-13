// Copyright (c) 2009-2014 The Octocoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "uritests.h"

#include "guiutil.h"
#include "walletmodel.h"

#include <QUrl>

void URITests::uriTests()
{
    SendCoinsRecipient rv;
    QUrl uri;
    uri.setUrl(QString("octocoin:8T4o9PzBXbp17KRjVQpY4VktbVAZRvDGuw?req-dontexist="));
    QVERIFY(!GUIUtil::parseBitcoinURI(uri, &rv));

    uri.setUrl(QString("octocoin:8T4o9PzBXbp17KRjVQpY4VktbVAZRvDGuw?dontexist="));
    QVERIFY(GUIUtil::parseBitcoinURI(uri, &rv));
    QVERIFY(rv.address == QString("8T4o9PzBXbp17KRjVQpY4VktbVAZRvDGuw"));
    QVERIFY(rv.label == QString());
    QVERIFY(rv.amount == 0);

    uri.setUrl(QString("octocoin:8T4o9PzBXbp17KRjVQpY4VktbVAZRvDGuw?label=SecurePayment.CC Example Address"));
    QVERIFY(GUIUtil::parseBitcoinURI(uri, &rv));
    QVERIFY(rv.address == QString("8T4o9PzBXbp17KRjVQpY4VktbVAZRvDGuw"));
    QVERIFY(rv.label == QString("SecurePayment.CC Example Address"));
    QVERIFY(rv.amount == 0);

    uri.setUrl(QString("octocoin:8T4o9PzBXbp17KRjVQpY4VktbVAZRvDGuw?amount=0.001"));
    QVERIFY(GUIUtil::parseBitcoinURI(uri, &rv));
    QVERIFY(rv.address == QString("8T4o9PzBXbp17KRjVQpY4VktbVAZRvDGuw"));
    QVERIFY(rv.label == QString());
    QVERIFY(rv.amount == 100000);

    uri.setUrl(QString("octocoin:8T4o9PzBXbp17KRjVQpY4VktbVAZRvDGuw?amount=1.001"));
    QVERIFY(GUIUtil::parseBitcoinURI(uri, &rv));
    QVERIFY(rv.address == QString("8T4o9PzBXbp17KRjVQpY4VktbVAZRvDGuw"));
    QVERIFY(rv.label == QString());
    QVERIFY(rv.amount == 100100000);

    uri.setUrl(QString("octocoin:8T4o9PzBXbp17KRjVQpY4VktbVAZRvDGuw?amount=100&label=SecurePayment.CC Example"));
    QVERIFY(GUIUtil::parseBitcoinURI(uri, &rv));
    QVERIFY(rv.address == QString("8T4o9PzBXbp17KRjVQpY4VktbVAZRvDGuw"));
    QVERIFY(rv.amount == 10000000000LL);
    QVERIFY(rv.label == QString("SecurePayment.CC Example"));

    uri.setUrl(QString("octocoin:8T4o9PzBXbp17KRjVQpY4VktbVAZRvDGuw?message=SecurePayment.CC Example Address"));
    QVERIFY(GUIUtil::parseBitcoinURI(uri, &rv));
    QVERIFY(rv.address == QString("8T4o9PzBXbp17KRjVQpY4VktbVAZRvDGuw"));
    QVERIFY(rv.label == QString());

    QVERIFY(GUIUtil::parseBitcoinURI("octocoin://8T4o9PzBXbp17KRjVQpY4VktbVAZRvDGuw?message=SecurePayment.CC Example Address", &rv));
    QVERIFY(rv.address == QString("8T4o9PzBXbp17KRjVQpY4VktbVAZRvDGuw"));
    QVERIFY(rv.label == QString());

    uri.setUrl(QString("octocoin:8T4o9PzBXbp17KRjVQpY4VktbVAZRvDGuw?req-message=SecurePayment.CC Example Address"));
    QVERIFY(GUIUtil::parseBitcoinURI(uri, &rv));

    uri.setUrl(QString("octocoin:8T4o9PzBXbp17KRjVQpY4VktbVAZRvDGuw?amount=1,000&label=SecurePayment.CC Example"));
    QVERIFY(!GUIUtil::parseBitcoinURI(uri, &rv));

    uri.setUrl(QString("octocoin:8T4o9PzBXbp17KRjVQpY4VktbVAZRvDGuw?amount=1,000.0&label=SecurePayment.CC Example"));
    QVERIFY(!GUIUtil::parseBitcoinURI(uri, &rv));
}
