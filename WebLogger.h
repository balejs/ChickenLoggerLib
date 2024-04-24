#ifndef __WEB_LOGGER_H__
#define __WEB_LOGGER_H__

#include <Interfaces.h>

class WebLogger: public SessionComponent
{
    public:
    WebLogger();

    protected:
    void setupServer(std::shared_ptr<Chicken::HttpServer> server);
    void setupPowerManager(std::shared_ptr<PowerManager> powerManager);

    private:
    std::shared_ptr<Chicken::HttpServer> server;
    std::shared_ptr<PowerManager> pm;
};

#endif //__WEB_LOGGER_H__