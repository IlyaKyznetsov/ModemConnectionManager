#ifndef SIM7600E_H_H
#define SIM7600E_H_H

#include <Modem.h>

class SIM7600E_H : public Modem
{
public:
  SIM7600E_H() = default;

  // Modem interface
  virtual void reset() override;
  virtual QByteArray name() const override;
  virtual QList<QByteArray> commands() const override;
  virtual QByteArray chatConfiguration(const QByteArray &phone, const QString &accessPoint) const override;
  virtual CommandStatus parseResponse(const QByteArray &data, const QByteArray &command) override;
  virtual quint32 baudRate() const override;
  virtual QByteArray portConnection() const override;
  virtual QByteArray portService() const override;
  virtual QByteArray portGps() const override;
};

#endif // SIM7600E_H_H
