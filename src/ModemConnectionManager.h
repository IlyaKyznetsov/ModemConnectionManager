#ifndef MODEMCONNECTIONMANAGER_H
#define MODEMCONNECTIONMANAGER_H

#include <ModemConnectionManager_global.h>
#include <QObject>

class QProcess;
class QTimer;
class QThread;

class MODEMCONNECTIONMANAGER_EXPORT ModemConnectionManager : public QObject
{
  Q_OBJECT
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
  ModemConnectionManager::State state() const;

  explicit ModemConnectionManager(const QString &path = QString(), QObject *parent = nullptr);
  ~ModemConnectionManager();

Q_SIGNALS:
  void stateChanged(const ModemConnectionManager::State state);

public Q_SLOTS:
  bool connection();
  void disconnection();
  bool modemHardReset();

private Q_SLOTS:
  void _pppdOutput();
  void _pppdFinished(int exitCode, int exitStatus);

private:
  ModemConnectionManager(const ModemConnectionManager &) = delete;
  ModemConnectionManager(ModemConnectionManager &&) = delete;
  ModemConnectionManager &operator=(const ModemConnectionManager &) = delete;
  ModemConnectionManager &operator=(ModemConnectionManager &&) = delete;

  int _connectionHopes = 0;
  int _reconnectionHope = 0;
  QTimer *_reconnectionTimer = nullptr;
  QProcess *_pppd = nullptr;
  QByteArray _pppdCommand;
  QByteArray _modemResetCommand;
  State _state;
  QByteArray modemChatConfiguration_SIM7600E_H(const QByteArray &phone, const QString &accessPoint) const;
  bool modemResponseParser_SIM7600E_H(const QByteArray &data);
};
Q_DECLARE_METATYPE(ModemConnectionManager::State)
QString toString(ModemConnectionManager::State::Network::registration status);
QString toString(ModemConnectionManager::State::Network::gprs status);
QStringList toStringList(const ModemConnectionManager::State &state);

#endif // MODEMCONNECTIONMANAGER_H
