#include <QCoreApplication>
#include <QtTest>

// add necessary includes here
#include "Global.h"
#include "ModemCommander.h"
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

  void test_ModemState();
  bool haveInternet();
  void testConnection();
  void testConnectionAutomation();
};

test_ModemConnectionManager::test_ModemConnectionManager()
{
  PF();
}

test_ModemConnectionManager::~test_ModemConnectionManager()
{
}

void test_ModemConnectionManager::initTestCase()
{
}

void test_ModemConnectionManager::cleanupTestCase()
{
}

void test_ModemConnectionManager::onStateChanged(const Modem::State &state)
{
  PF();
  const QStringList states = toStringList(state);
  for (const auto &item : states)
    D(item);
}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/
void test_ModemConnectionManager::test_ModemState()
{
  QByteArray port = "/dev/ttyUSB3";
  QByteArray baud = "115200";
  ModemCommander mc(port, baud.toInt());
  mc.sendCommand("ATI");
  mc.sendCommand("AT+CICCID");
  mc.sendCommand("AT+CSPN?");
  bool isOk = mc.modem().state.status();
  QVERIFY(isOk);
  if (!isOk)
    QSKIP("skipping all");
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
void test_ModemConnectionManager::testConnection()
{
  ModemConnectionManager modem;

  QVERIFY(!haveInternet());
  connect(&modem, &ModemConnectionManager::stateChanged, this, &test_ModemConnectionManager::onStateChanged,
          Qt::QueuedConnection);
  QSignalSpy spy(&modem, &ModemConnectionManager::stateChanged);
  modem.connection();

  while (modem.state().internet.LocalAddress.isEmpty() && (spy.count() < 20))
  {
    I("WAIT" << spy.count());
    spy.wait(3000);
  }

  QVERIFY(haveInternet());
  spy.clear();
  D("TERMINATE");
  onStateChanged(modem.state());
  modem.disconnection();
  QVERIFY(!haveInternet());
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

QTEST_MAIN(test_ModemConnectionManager)

#include "tst_tests.moc"
