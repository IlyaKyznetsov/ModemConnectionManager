#ifndef MODEMCOMMANDER_H
#define MODEMCOMMANDER_H

#include <Modem.h>
#include <QObject>

class QSerialPort;
class ModemCommander : public QObject
{
  Q_OBJECT
public:
  explicit ModemCommander(const QByteArray &servicePort, const qint32 baud, QObject *parent = nullptr);
  ~ModemCommander();
  bool sendCommand(const QByteArray &ATCommand);
  const Modem &modem() const;

private:
  QSerialPort *_serialPort;
  Modem _SIM7600E_H;
};

#endif // MODEMCOMMANDER_H
