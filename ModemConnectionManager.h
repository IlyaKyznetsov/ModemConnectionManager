#ifndef MODEMCONNECTIONMANAGER_H
#define MODEMCONNECTIONMANAGER_H

#include <QMap>
#include <QObject>
#include <QProcess>
#include <QRegularExpression>
#include <QSharedPointer>
#include <QTimer>

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
      int PID = 0;
      QString Interface;
      QString LocalAddress;
      QString RemoteAddress;
      QString PrimaryDNS;
      QString SecondaryDNS;
    } internet;
  };
  State state() const;

  explicit ModemConnectionManager(const QString &path = QString(), QObject *parent = nullptr);
  ~ModemConnectionManager();

public Q_SLOTS:
  bool connection();
  void disconnection();
  bool modemHardReset();

Q_SIGNALS:
  void stateChanged(const State state);

private Q_SLOTS:
  void pppdOutput();
  void pppdError();
  void pppdFinished(int exitCode, int exitStatus);

private:
  QByteArray _modemResetCommand;
  // QMap<QByteArray, QRegularExpression> _chat;
  QSharedPointer<QTimer> _reconnectionTimer;
  QSharedPointer<QProcess> _pppd;
  QStringList _pppdArguments;
  State _state;
  bool SIM7600E_H(const QByteArray &data);
};

#endif // MODEMCONNECTIONMANAGER_H
