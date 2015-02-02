#include <QtTest/QTest>
#include "telescope/indidouble.h"

class TestINDIDouble : public QObject {
Q_OBJECT
private slots:
  void testInvalidFormat();
  void testDoubleToStringPrintf();
};

/* some valid test cases...
number=0; format="%010.6m";
number=-12722.5; format="%010.6m";
number=0; format="%010.6m";
number=0; format="%010.6m";
number=0; format="%g";
number=203; format="%g";
number=2000; format="%g";
number=203; format="%g";
number=2000; format="%g";
number=0; format="%g";
number=0; format="%g";
number=0; format="%g";
number=0; format="%g";
number=0.3; format="%g";
number=0.3; format="%g";
number=15; format="%010.6m";
number=15; format="%010.6m";
number=0; format="%010.6m";
number=-12724.5; format="%010.6m";
number=0; format="%010.6m";
*/

void TestINDIDouble::testInvalidFormat()
{
  auto result = INDIDouble(1.5, "abcq123");
  QVERIFY2(! result, "Result should be not valid");
}


void TestINDIDouble::testDoubleToStringPrintf()
{
  QCOMPARE(INDIDouble(1.44, "%.2f").text(), QString("1.44"));
}

QTEST_MAIN(TestINDIDouble)


#include "testindidouble.moc"
