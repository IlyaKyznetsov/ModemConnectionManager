#ifndef MODEMCONNECTIONMANAGER_H
#define MODEMCONNECTIONMANAGER_H

#include <Modem.h>
#include <ModemConnectionManager_global.h>
#include <QObject>
#include <QSharedPointer>

class QProcess;
class QTimer;
class QThread;
class QUdevDevice;
class MODEMCONNECTIONMANAGER_EXPORT ModemConnectionManager : public QObject
{
  Q_OBJECT
public:
  Modem::State state() const;
  explicit ModemConnectionManager(const QString &path = QString(), QObject *parent = nullptr);
  ~ModemConnectionManager();

Q_SIGNALS:
  void stateChanged(const Modem::State state);

public Q_SLOTS:
  bool connection();
  void disconnection();
  bool reset();

private Q_SLOTS:
  void _qudevDeviceEvent(const QUdevDevice &event);
  void _pppdOutput();
  void _pppdFinished(int exitCode, int exitStatus);

private:
  ModemConnectionManager(const ModemConnectionManager &) = delete;
  ModemConnectionManager(ModemConnectionManager &&) = delete;
  ModemConnectionManager &operator=(const ModemConnectionManager &) = delete;
  ModemConnectionManager &operator=(ModemConnectionManager &&) = delete;

  int _connectionHopes = 0;
  int _reconnectionHope = 0;
  QSharedPointer<Modem> _modem;
  QTimer *_reconnectionTimer = nullptr;
  QProcess *_pppd = nullptr;
  QByteArray _options, _phone, _accessPoint, _user;
  QByteArray _pppdCommand;
};

#endif // MODEMCONNECTIONMANAGER_H
