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
  };

  State state;
  bool status() const;
  virtual QByteArray name() const = 0;
  virtual quint32 baudRate() const = 0;
  virtual QByteArray portConnection() const = 0;
  virtual QByteArray portService() const = 0;
  virtual QByteArray portGps() const = 0;
//  virtual bool reset() = 0;
  virtual QList<QByteArray> commands() const = 0;
  virtual bool parseResponse(const QByteArray &data) = 0;
  virtual QByteArray chatConfiguration(const QByteArray &phone, const QString &accessPoint) const;
};
QString toString(Modem::State::Network::registration status);
QString toString(Modem::State::Network::gprs status);
QStringList toStringList(const Modem::State &state);

#endif // MODEM_H
