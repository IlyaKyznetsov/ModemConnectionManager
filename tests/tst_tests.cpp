#include <QCoreApplication>
#include <QtTest>

// add necessary includes here
#include "Global.h"
#include "ModemConnectionAutomator.h"
#include "ModemConnectionManager.h"

class test_ModemConnectionManager : public QObject
{
  Q_OBJECT

public:
  test_ModemConnectionManager();
  ~test_ModemConnectionManager();

private slots:
  void initTestCase();
  void cleanupTestCase();
  void onStateChanged(const Modem::State &state);

  bool haveInternet();
  void testConnectionAutomation();
  //void SimpleExample();
};

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/
test_ModemConnectionManager::test_ModemConnectionManager()
{
  PF();
}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/
test_ModemConnectionManager::~test_ModemConnectionManager()
{
}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/
void test_ModemConnectionManager::initTestCase()
{
}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/
void test_ModemConnectionManager::cleanupTestCase()
{
}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/
void test_ModemConnectionManager::onStateChanged(const Modem::State &state)
{
  PF();
  const QStringList states = toStringList(state);
  for (const auto &item : states)
    D(item);
}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/
bool test_ModemConnectionManager::haveInternet()
{
  QProcess ping;
  ping.start("ping", {"8.8.8.8", "-c", "1"});
  ping.waitForFinished(2000);
  return ping.exitCode() == 0;
}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/
void test_ModemConnectionManager::testConnectionAutomation()
{
  ModemConnectionAutomator mca;
  QVERIFY(!haveInternet());
  connect(&mca, &ModemConnectionAutomator::stateChanged, this, &test_ModemConnectionManager::onStateChanged,
          Qt::QueuedConnection);
  QSignalSpy spy(&mca, &ModemConnectionAutomator::stateChanged);

  while (mca.state().internet.LocalAddress.isEmpty() && (spy.count() < 20))
  {
    I("WAIT" << spy.count());
    spy.wait(3000);
  }

  QVERIFY(haveInternet());
  spy.clear();
  D("TERMINATE");
  onStateChanged(mca.state());
  mca.disconnection();
  QVERIFY(!haveInternet());
}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/
// void test_ModemConnectionManager::SimpleExample()
//{
//  ModemConnectionAutomator mca;
//  connect(&mca, &ModemConnectionAutomator::stateChanged, this, &test_ModemConnectionManager::onStateChanged,
//          Qt::QueuedConnection);
//  qApp->exec();
//}

QTEST_MAIN(test_ModemConnectionManager)

#include "tst_tests.moc"
