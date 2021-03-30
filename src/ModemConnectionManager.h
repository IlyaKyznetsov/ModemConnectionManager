#ifndef MODEMCONNECTIONMANAGER_H
#define MODEMCONNECTIONMANAGER_H

#include <ModemConnectionManager_global.h>
#include <QObject>
#include <Modem.h>

class QSerialPort;
class QProcess;
class QTimer;
class QThread;

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
  QSerialPort *_serialPort;
  QTimer *_reconnectionTimer = nullptr;
  QProcess *_pppd = nullptr;
  QByteArray _pppdCommand;
  QByteArray _modemResetCommand;
  Modem _modem;
  QByteArray modemChatConfiguration_SIM7600E_H(const QByteArray &phone, const QString &accessPoint) const;
  bool modemResponseParser_SIM7600E_H(const QByteArray &data);
};

#endif // MODEMCONNECTIONMANAGER_H
