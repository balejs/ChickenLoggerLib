#ifndef __WEB_LOGGER_H__
#define __WEB_LOGGER_H__

#include <Interfaces.h>

// TODO: these two dependencies aren't tracked by library.json
// split weblogger into its own library
#include <HttpServer.h>
#include <PowerManager.h>

namespace Chicken
{
    
class WebLogger: public SessionComponent
{
    public:
    WebLogger();

    protected:
    void setupServer(SHttpServer server);
    void setupPowerManager(SPowerManager powerManager);

    private:
    SHttpServer _server;
    SPowerManager _powerManager;
};
}

#endif //__WEB_LOGGER_H__