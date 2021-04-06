#ifndef SIM7600E_H_H
#define SIM7600E_H_H

#include <Modem.h>

class SIM7600E_H : public Modem
{
public:
  SIM7600E_H();

  // Modem interface
  virtual QByteArray name() const override;
  static bool reset();
  virtual QList<QByteArray> commands() const override;
  virtual bool parseResponse(const QByteArray &data) override;
  virtual quint32 baudRate() const override;
  virtual QByteArray portConnection() const override;
  virtual QByteArray portService() const override;
  virtual QByteArray portGps() const override;
};

#endif // SIM7600E_H_H
