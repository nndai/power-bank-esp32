#ifndef _LOG_H_
#define _LOG_H_

#include <Arduino.h>
#include <LittleFS.h>
#include <WebSocketsServer.h>

// =================== Base Logger ===================
class Logger {
public:
    enum LogLevel {
        LOG_INFO,
        LOG_WARNING,
        LOG_ERROR,
        LOG_DEBUG,
        NO_LEVEL,
    };

protected:
    String getLevelString(LogLevel level) {
        switch (level) {
        case LOG_INFO: return "INFO";
        case LOG_WARNING: return "WARNING";
        case LOG_ERROR: return "ERROR";
        case LOG_DEBUG: return "DEBUG";
        case NO_LEVEL: return "";
        default: return "UNKNOWN";
        }
    }

public:
    virtual void log(LogLevel level, const String& message, String endchar = "\n") = 0;
    virtual ~Logger() = default;
};

// =================== Serial Logger ===================
class SerialLogger : public Logger {
private:
    static bool _isSerialBegin;

public:
    SerialLogger(unsigned long baud = 115200);

    void log(LogLevel level, const String& message, String endchar = "\n") override;
};

// =================== WebSocket Logger ===================
class WebSocketLogger : public Logger {
private:
    WebSocketsServer* _websocket = nullptr;
    uint8_t _idClient = 255;
public:
    WebSocketLogger(WebSocketsServer* websocket, uint8_t idClient);

    void log(LogLevel level, const String& message, String endchar = "\n") override;
    uint8_t getIdClientWebSockets() const;
};


// =================== File Logger ===================
class FileLogger : public Logger {
private:
    static bool _isLittleFSBegin;
    String _filename;
    bool _isError;
public:
    FileLogger(String filename);

    void log(LogLevel level, const String& message, String endchar = "\n") override;
    bool getIsError() const;
};


// =================== Logger Manager ===================
class LoggerManager {
public:
    enum LogMode {
        MODE_SERIAL,
        MODE_WEBSOCKETS
    };

private:
    Logger* _currentLogger = nullptr;
    LogMode _mode;
public:
    LoggerManager();
    ~LoggerManager();

    void setLogModeSerial(unsigned long baud = 115200);
    void setLogModeWebSockets(WebSocketsServer* websocket, uint8_t idClient);
    Logger* getLogger();
    LogMode getLogMode() const;

    void log(Logger::LogLevel level, const String& message, const char* fileName, int32_t lineNumber, String endchar = "\n");

    template<typename T>
    void info(T message, const char* fileName, int32_t lineNumber, String endchar = "\n") {
        log(Logger::LOG_INFO, String(message), fileName, lineNumber, endchar);
    }
    template<typename T>
    void warning(T message, const char* fileName, int32_t lineNumber, String endchar = "\n") {
        log(Logger::LOG_WARNING, String(message), fileName, lineNumber, endchar);
    }
    template<typename T>
    void error(T message, const char* fileName, int32_t lineNumber, String endchar = "\n") {
        log(Logger::LOG_ERROR, String(message), fileName, lineNumber, endchar);
    }
    template<typename T>
    void debug(T message, const char* fileName, int32_t lineNumber, String endchar = "\n") {
        log(Logger::LOG_DEBUG, String(message), fileName, lineNumber, endchar);
    }
    template<typename T>
    void logNoLevel(T message, const char* fileName, int32_t lineNumber, String endchar = "\n") {
        log(Logger::NO_LEVEL, String(message), fileName, lineNumber, endchar);
    }
};



extern LoggerManager mainLog;

// =================== Macros ===================
#ifdef ENABLE_APP_LOG
#define app_log_i(message, ...) mainLog.info(message,__FILE__, __LINE__, ##__VA_ARGS__)
#define app_log_e(message, ...) mainLog.error(message,__FILE__, __LINE__, ##__VA_ARGS__)
#define app_log_d(message, ...) mainLog.debug(message,__FILE__, __LINE__, ##__VA_ARGS__)
#define app_log_w(message, ...) mainLog.warning(message,__FILE__, __LINE__, ##__VA_ARGS__)
#define app_log(message, ...)   mainLog.logNoLevel(message, __FILE__, __LINE__, ##__VA_ARGS__)
#else
#define app_log_i(...)
#define app_log_e(...)
#define app_log_d(...)
#define app_log_w(...)
#define app_log(...)
#endif


#endif // _LOG_H_
