#ifndef MODEMCONNECTIONMANAGER_H
#define MODEMCONNECTIONMANAGER_H

#include <QMap>
#include <QObject>
#include <QProcess>
#include <QRegularExpression>
#include <QSharedPointer>

class ModemConnectionManager : public QObject
{
  Q_OBJECT
  Q_DISABLE_COPY(ModemConnectionManager)
public:
  struct State
  {
    struct Modem
    {
      QString Manufacturer;
      QString Model;
      QString Revision;
      QString IMEI;
    } modem;

    struct SIM
    {
      QString ICCID;
      QString Operator;
    } sim;

    struct Network
    {
      enum class registration
      {
        NotRegistered = 0,
        RegisteredHomeNetwork = 1,
        Searching = 2,
        RegistrationDenied = 3,
        Unknown = 4,
        RegisteredRoaming = 5
      } Registration;
      enum class gprs
      {
        NotRegistered = 0,
        RegisteredHomeNetwork = 1,
        Searching = 2,
        RegistrationDenied = 3,
        Unknown = 4,
        RegisteredRoaming = 5
      } GPRS;
    } network;

    struct Internet
    {
      int PID;
      QString Interface;
      QString LocalAddress;
      QString RemoteAddress;
      QString PrimaryDNS;
      QString SecondaryDNS;
    } internet;
    void debug() const;
  };

  explicit ModemConnectionManager(const QString &path = QString(), QObject *parent = nullptr);
  ~ModemConnectionManager();
  bool connection();

Q_SIGNALS:
  void stateChanged(const State state);

private Q_SLOTS:
  void pppdOutput();
  void pppdError();
  void pppdFinished(int exitCode, int exitStatus);

private:
  struct Configuration
  {
    QMap<QByteArray, QRegularExpression> chat;
    QByteArray modemResetCommand;
  } _configuration;
  State _state;
  QSharedPointer<QProcess> _pppd;
  QStringList _pppdArguments;
};

#endif // MODEMCONNECTIONMANAGER_H
