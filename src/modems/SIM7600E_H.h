#ifndef SIM7600E_H_H
#define SIM7600E_H_H

#include <Modem.h>

class SIM7600E_H : public Modem
{
public:
  SIM7600E_H();

  /*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/
  bool reset() override;

  /*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/
  bool parseResponse(const QByteArray &data) override;
};

#endif // SIM7600E_H_H
