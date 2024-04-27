#ifndef __WEB_LOGGER_H__
#define __WEB_LOGGER_H__

#include <Interfaces.h>

// TODO: these two dependencies aren't tracked by library.json
// split weblogger into its own library
#include <HttpServer.h>
#include <PowerManager.h>
#include <SessionListener.h>

namespace Chicken
{
    
class WebLogger: public SessionComponent
{
    public:

    protected:
    void setupServer(Any server);
    void setupPowerManager(SPowerManager powerManager);

    private:
    SHttpServer _httpServer;
    SPowerManager _powerManager;
};
}

#endif //__WEB_LOGGER_H__