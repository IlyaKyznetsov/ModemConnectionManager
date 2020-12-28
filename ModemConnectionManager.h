#ifndef MODEMCONNECTIONMANAGER_H
#define MODEMCONNECTIONMANAGER_H

#include <QMutex>
#include <QObject>
#include <QProcess>
#include <QSettings>
#include <QSharedPointer>
#include <QThread>
#include <QTimer>
#include <QWaitCondition>

class ModemConnectionManager : public QObject
{
  Q_OBJECT
public:
  static ModemConnectionManager &instance(const QString &path = QString());
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
  void connection();
  void disconnection();
  void modemHardReset();

Q_SIGNALS:
  void stateChanged(const State state);

private Q_SLOTS:
  void _postConstructOwner();
  void _preDestroyOwner();
  void _pppdOutput();
  void _pppdError();
  void _pppdFinished(int exitCode, int exitStatus);
  bool _connection();
  void _disconnection();
  bool _modemHardReset();

private:
  ModemConnectionManager() = delete;
  ModemConnectionManager(const ModemConnectionManager &) = delete;
  ModemConnectionManager(ModemConnectionManager &&) = delete;
  ModemConnectionManager &operator=(const ModemConnectionManager &) = delete;
  ModemConnectionManager &operator=(ModemConnectionManager &&) = delete;

  const QString _configurationPath;
  QThread _thread;
  QMutex _mutex;
  QWaitCondition _condition;
  QByteArray _modemResetCommand;
  int _reconnectTimeout;
  int _resetConnectionHopes;
  int _reconnectionHope;
  QSharedPointer<QTimer> _reconnectionTimer;
  QSharedPointer<QProcess> _pppd;
  QStringList _pppdArguments;
  State _state;
  bool SIM7600E_H(const QByteArray &data);
};

#endif // MODEMCONNECTIONMANAGER_H
