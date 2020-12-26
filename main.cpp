#include "ModemConnectionManager.h"
#include <QCoreApplication>

int main(int argc, char *argv[])
{
  QCoreApplication a(argc, argv);

  // Read configuration
  ModemConnectionManager mcm;
  mcm.connection();

  return a.exec();
}
