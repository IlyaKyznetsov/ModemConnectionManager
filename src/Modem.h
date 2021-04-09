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

  enum class CommandStatus
  {
    None,
    Success,
    Error
  };

  State state;
  Modem() = default;
  virtual ~Modem() = default;
  bool initialize();
  virtual void reset() = 0;
  virtual QByteArray name() const = 0;
  virtual quint32 baudRate() const = 0;
  virtual QByteArray portConnection() const = 0;
  virtual QByteArray portService() const = 0;
  virtual QByteArray portGps() const = 0;
  virtual QList<QByteArray> commands() const = 0;
  virtual QByteArray chatConfiguration(const QByteArray &phone, const QString &accessPoint) const;
  virtual CommandStatus parseResponse(const QByteArray &data, const QByteArray &command = QByteArray()) = 0;

protected:
  QByteArray baseChatConfiguration(const QByteArray &phone, const QString &accessPoint,
                                   const QList<QByteArray> &chatATCommands) const;

private:
  Modem(const Modem &) = delete;
  Modem &operator=(const Modem &) = delete;
  Modem(Modem &&) = delete;
  Modem &operator=(Modem &&) = delete;
  bool modemCommand(const QByteArray &ATCommand);
};
QString toString(Modem::CommandStatus status);
QString toString(Modem::State::Network::registration status);
QString toString(Modem::State::Network::gprs status);
QStringList toStringList(const Modem::State &state);

#endif // MODEM_H
