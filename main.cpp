#include "Global.h"
#include "ModemConnectionManager.h"
#include <QCoreApplication>
#include <QRegularExpression>

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/
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
/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/
void debugState(const ModemConnectionManager::State &state)
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

int main(int argc, char *argv[])
{
  QCoreApplication a(argc, argv);

  // Read configuration
  QObject::connect(&ModemConnectionManager::instance(), &ModemConnectionManager::stateChanged, &debugState);
  ModemConnectionManager::instance().connection();
  QTimer test;
  test.setInterval(30000);
  test.setSingleShot(true);
  QObject::connect(&test, &QTimer::timeout, []() {
    D("### Disconnection ###");
    ModemConnectionManager::instance().disconnection();
    //      D("### Hard reset modem ###");
    //      ModemConnectionManager::instance().modemHardReset();
    //      D("### Connection ###");
    //      ModemConnectionManager::instance().connection();
  });
  //  test.start();

  return a.exec();
}
