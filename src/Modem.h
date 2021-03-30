#ifndef MODEM_H
#define MODEM_H

#include <QString>
class Modem
{
public:
  struct State
  {
    struct Modem
    {
      bool status = true;
      QString Manufacturer;
      QString Model;
      QString Revision;
      QString IMEI;
    } modem;

    struct SIM
    {
      bool status = true;
      QString ICCID;
      QString Operator;
    } sim;

    struct Network
    {
      bool status = true;
      enum class registration
      {
        NotRegistered = 0,
        RegisteredHomeNetwork = 1,
        Searching = 2,
        RegistrationDenied = 3,
        Unknown = 4,
        RegisteredRoaming = 5
      } Registration = registration::NotRegistered;
      enum class gprs
      {
        NotRegistered = 0,
        RegisteredHomeNetwork = 1,
        Searching = 2,
        RegistrationDenied = 3,
        Unknown = 4,
        RegisteredRoaming = 5
      } GPRS = gprs::NotRegistered;
    } network;

    struct Internet
    {
      bool status = true;
      int PID = 0;
      QString Interface;
      QString LocalAddress;
      QString RemoteAddress;
      QString PrimaryDNS;
      QString SecondaryDNS;
    } internet;
    bool status() const { return modem.status && sim.status && network.status && internet.status; }
  };

  State state;
  static bool modemReset_SIM7600E_H();
  bool modemResponseParser_SIM7600E_H(const QByteArray &data);
};
QString toString(Modem::State::Network::registration status);
QString toString(Modem::State::Network::gprs status);
QStringList toStringList(const Modem::State &state);

#endif // MODEM_H
