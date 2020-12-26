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

  const QByteArray data{
      "ATI^M^M Manufacturer: SIMCOM INCORPORATED^M Model: SIMCOM_SIM7600E-H^M Revision: SIM7600M22_V1.1^M IMEI: "
      "867584033254140^M +GCAP: +CGSM,+DS,+ES^M ^M OK -- got it send (ATI^M)"};
  QRegularExpression rxCommand("got it send \\((.*)\\^M\\)$");
  QRegularExpressionMatch matchCommand = rxCommand.match(data);
  D(rxCommand.isValid() << matchCommand.isValid());
  D(matchCommand.hasMatch());
  QString command = matchCommand.captured(1);
  D(command << matchCommand.capturedTexts());
  //  for (int index = 1; index != rxm.lastCapturedIndex(); ++index)
  //  {
  //    D(rxm.captured(index));
  //  }

  QString arg = "(*.)";
  QString test = ("^" + command + "\\^M\\^M " + "(.*)");
  D("HHHHHHHHH:" << test);
  QRegularExpression rxResponse(test);
  QRegularExpressionMatch matchResponse = rxResponse.match(data);
  D(rxResponse.isValid() << matchResponse.isValid());
  D(matchResponse.hasMatch());
  QString response = matchResponse.captured(1);
  D(response << matchCommand.capturedTexts());

  //  exit(0);

  // Read configuration
  ModemConnectionManager mcm;
  QObject::connect(&mcm, &ModemConnectionManager::stateChanged, &debugState);
  mcm.connection();

  return a.exec();
}
