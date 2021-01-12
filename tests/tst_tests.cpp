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
  void onStateChanged(const ModemConnectionManager::State &state);

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

QString toString(ModemConnectionManager::State::Network::registration status)
{
  switch (status)
  {
    case ModemConnectionManager::State::Network::registration::NotRegistered: return "not registered";
    case ModemConnectionManager::State::Network::registration::RegisteredHomeNetwork: return "registered, home network";
    case ModemConnectionManager::State::Network::registration::Searching: return "not registered, searching";
    case ModemConnectionManager::State::Network::registration::RegistrationDenied: return "registration denied";
    case ModemConnectionManager::State::Network::registration::Unknown: return "unknown";
    case ModemConnectionManager::State::Network::registration::RegisteredRoaming: return "registered, roaming";
  }
  return QString();
}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/
QString toString(ModemConnectionManager::State::Network::gprs status)
{
  switch (status)
  {
    case ModemConnectionManager::State::Network::gprs::NotRegistered: return "not registered";
    case ModemConnectionManager::State::Network::gprs::RegisteredHomeNetwork: return "registered, home network";
    case ModemConnectionManager::State::Network::gprs::Searching:
      return "not registered, trying to attach or searching";
    case ModemConnectionManager::State::Network::gprs::RegistrationDenied: return "registration denied";
    case ModemConnectionManager::State::Network::gprs::Unknown: return "unknown";
    case ModemConnectionManager::State::Network::gprs::RegisteredRoaming: return "registered, roaming";
  }
  return QString();
}

void test_ModemConnectionManager::onStateChanged(const ModemConnectionManager::State &state)
{
  PF();
  D("Modem Manufacturer:" << state.modem.Manufacturer);
  D("Modem Model:" << state.modem.Model);
  D("Modem Revision:" << state.modem.Revision);
  D("Modem IMEI:" << state.modem.IMEI);
  D("SIM ICCID:" << state.sim.ICCID);
  D("SIM Operator:" << state.sim.Operator);
  D("Registration:" << toString(state.network.Registration));
  D("GPRS:" << toString(state.network.GPRS));
  D("Internet PID" << state.internet.PID);
  D("Internet Interface" << state.internet.Interface);
  D("Internet LocalAddress" << state.internet.LocalAddress);
  D("Internet RemoteAddress" << state.internet.RemoteAddress);
  D("Internet PrimaryDNS" << state.internet.PrimaryDNS);
  D("Internet SecondaryDNS" << state.internet.SecondaryDNS);
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
