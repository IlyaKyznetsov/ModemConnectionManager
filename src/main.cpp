#include "ModemConnectionAutomator.h"

#include <QCoreApplication>

#include "Global.h"

//void onStateChanged(const ModemConnectionManager::State &state)
//{
//  PF();
//  D("Modem Manufacturer:" << state.modem.Manufacturer);
//  D("Modem Model:" << state.modem.Model);
//  D("Modem Revision:" << state.modem.Revision);
//  D("Modem IMEI:" << state.modem.IMEI);
//  D("SIM ICCID:" << state.sim.ICCID);
//  D("SIM Operator:" << state.sim.Operator);
//  D("Registration:" << toString(state.network.Registration));
//  D("GPRS:" << toString(state.network.GPRS));
//  D("Internet PID" << state.internet.PID);
//  D("Internet Interface" << state.internet.Interface);
//  D("Internet LocalAddress" << state.internet.LocalAddress);
//  D("Internet RemoteAddress" << state.internet.RemoteAddress);
//  D("Internet PrimaryDNS" << state.internet.PrimaryDNS);
//  D("Internet SecondaryDNS" << state.internet.SecondaryDNS);
//}

int main(int argc, char *argv[])
{
  QCoreApplication a(argc, argv);

  ModemConnectionAutomator mca;
//  QObject::connect(&mca, &ModemConnectionAutomator::stateChanged, [](const ModemConnectionManager::State &state) {
//    PF();
//    D("Modem Manufacturer:" << state.modem.Manufacturer);
//    D("Modem Model:" << state.modem.Model);
//    D("Modem Revision:" << state.modem.Revision);
//    D("Modem IMEI:" << state.modem.IMEI);
//    D("SIM ICCID:" << state.sim.ICCID);
//    D("SIM Operator:" << state.sim.Operator);
//    D("Registration:" << toString(state.network.Registration));
//    D("GPRS:" << toString(state.network.GPRS));
//    D("Internet PID" << state.internet.PID);
//    D("Internet Interface" << state.internet.Interface);
//    D("Internet LocalAddress" << state.internet.LocalAddress);
//    D("Internet RemoteAddress" << state.internet.RemoteAddress);
//    D("Internet PrimaryDNS" << state.internet.PrimaryDNS);
//    D("Internet SecondaryDNS" << state.internet.SecondaryDNS);
//  });

  return a.exec();
}
