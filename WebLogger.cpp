#include <HttpServer.h>
#include <LoopScheduler.h>
#include <WebLogger.h>

#if (WEBLOGGER_LOG_LEVEL > 0)
#define TAG "WebLogger"
#endif

#include <DebugFuncs.h>

const char * logPage = R"====(
<!DOCTYPE HTML>
<html>
  <head>
    <title>Log page</title>
    <style>
      html {font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}
      body {max-width: 600px; margin-top: 50px; padding-bottom: 25px;}
      h2 {color: #444444;margin: 50px auto 30px;}
      h3 {color: #444444;margin-bottom: 50px;}
      p {font-size: 14px;color: #888;margin-bottom: 10px;}      
    </style>
  </head>
  <body>
    <h2>Log page</h2>
    <textarea id="text_area" name="text_area" rows="40" cols="100"></textarea>
    <script>
      window.onload = readLogs

      function readLogs() {
        var xhr = new XMLHttpRequest()
        xhr.timeout = 1000
        xhr.onload = () => {
          if (xhr.readyState === xhr.DONE) {
            if (xhr.status === 200) {
              document.getElementById('text_area').value += xhr.responseText
            }
          }

          setTimeout(readLogs, 250)
        }

        xhr.ontimeout = () => {readLogs()}

        xhr.open("GET", "/readLogs", true);
        xhr.send();
      }
    </script>
  </body>
</html>)====";

namespace Chicken
{

void WebLogger::setupServer(std::any httpServer)
{
    _httpServer = sharedPtr<HttpServer>(httpServer);

    _httpServer->addUriHandler("/logs", HTTP_GET, [](SHttpRequest request) -> esp_err_t
    {
        request->addToResponse(logPage, HTTPD_200, HTTPD_TYPE_TEXT, true);
        return ESP_OK;
    });

    _httpServer->addUriHandler("/readLogs", HTTP_GET, [](SHttpRequest request) -> esp_err_t
    {
      std::string * logBuffer = Logger::getLogger()->getLog();
      if (logBuffer != NULL) {
        request->addToResponse(logBuffer->c_str(), HTTPD_200, HTTPD_TYPE_TEXT, true);
        delete logBuffer;
      } else {
        request->addToResponse("", HTTPD_200, HTTPD_TYPE_TEXT, true);
      }
      return ESP_OK;
    });
}

void WebLogger::setupPowerManager(std::shared_ptr<Chicken::PowerManager> powerManager)
{
  this->_powerManager = powerManager;
}
} // namespace Chicken