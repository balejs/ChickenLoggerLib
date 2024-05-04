#define CHICKEN_LOG_LEVEL 4

#include <Logger.h>
#include <iostream> // This is for test debug only
#include <queue>

#include <unity.h> // goes last else it clashes with c ++ definitions

class TestLoggerListener: public LoggerListener
{
    public:

    virtual void newLogEntry(SLoggerEntry entry)
    {
        _entries.push(entry);
        std::cout << entry;
    }

    SLoggerEntry getEntry()
    {
        if (_entries.size() == 0)
        {
            return nullptr;
        }

        auto entry = _entries.front();
        _entries.pop();
        return entry;
    }

    private:
    std::queue<SLoggerEntry> _entries;
};

static std::shared_ptr<TestLoggerListener> testLoggerListener;

void setUp()
{
  Logger::initLogs();
  testLoggerListener = std::make_shared<TestLoggerListener>();
  Logger::getLogger()->addListener(testLoggerListener);
}

void tearDown()
{
  // clean stuff up here
}

void test_logger()
{
    auto logger = Logger::getLogger();

    TEST_ASSERT_NOT_EQUAL_MESSAGE(NULL, logger, "Unable to get logger");

    _logi("Test log string");
    SLoggerEntry entry = testLoggerListener->getEntry();

    TEST_ASSERT_NOT_EQUAL_MESSAGE(nullptr, entry, "No log buffer");
    TEST_ASSERT_NOT_EQUAL_MESSAGE(entry->npos, entry->find("Test log string"), "Entry does not match");
}

extern "C" void app_main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_logger);
    UNITY_END();
}

// It seems as if native needs this one, too
extern "C" int main(void)
{
    app_main();
    return 0;
}