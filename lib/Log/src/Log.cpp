#include "log.h"


bool SerialLogger::_isSerialBegin = false;

SerialLogger::SerialLogger(unsigned long baud) {
    if (!_isSerialBegin || Serial.baudRate() != baud) {
        Serial.begin(baud);
        _isSerialBegin = true;
    }
}

void SerialLogger::log(LogLevel level, const String& message, String endchar) {
    String totalMessage;
    if (level == NO_LEVEL) totalMessage = message + endchar;
    else totalMessage = '[' + getLevelString(level) + "]: " + message + endchar;
    Serial.print(totalMessage);
}


WebSocketLogger::WebSocketLogger(WebSocketsServer* websocket, uint8_t idClient)
    : _websocket(websocket), _idClient(idClient) {
}

void WebSocketLogger::log(LogLevel level, const String& message, String endchar) {
    if (_websocket && _idClient != 255) {
        String totalMessage;
        if (level == NO_LEVEL) totalMessage = message + endchar;
        else totalMessage = '[' + getLevelString(level) + "]: " + message + endchar;
        _websocket->sendTXT(_idClient, totalMessage);
    }
}

uint8_t WebSocketLogger::getIdClientWebSockets() const {
    return _idClient;
}


LoggerManager::LoggerManager() {
    setLogModeSerial();
}
LoggerManager::~LoggerManager() {
    if (_currentLogger) {
        delete _currentLogger;
    }
}

void LoggerManager::setLogModeSerial(unsigned long baud) {
    if (_currentLogger) {
        delete _currentLogger;
    }
    _currentLogger = new SerialLogger(baud);
    _mode = MODE_SERIAL;
    app_log_i("Logger set to Serial mode.");
}

void LoggerManager::setLogModeWebSockets(WebSocketsServer* websocket, uint8_t idClient) {
    if (_currentLogger) {
        delete _currentLogger;
    }
    _currentLogger = new WebSocketLogger(websocket, idClient);
    _mode = MODE_WEBSOCKETS;
    app_log_i("Logger set to WebSocket mode.");
}

Logger* LoggerManager::getLogger() {
    return _currentLogger;
}
LoggerManager::LogMode LoggerManager::getLogMode() const {
    return _mode;
}

void LoggerManager::log(Logger::LogLevel level, const String& message, const char* fileName, int32_t lineNumber, String endchar) {
    String totalMessage = '[' + String(fileName) + "][" + String(lineNumber) + "]: " + String(message);
    if (_currentLogger) {
        _currentLogger->log(level, totalMessage, endchar);
    }
}


bool FileLogger::_isLittleFSBegin = false;

FileLogger::FileLogger(String filename) : _filename(filename) {
    if (!_isLittleFSBegin) {
        if (!LittleFS.begin()) {
            app_log_e("An Error has occurred while mounting LittleFS");
            _isLittleFSBegin = true;
            _isError = true;
        }
        else {
            _isError = false;
            _isLittleFSBegin = false;
        }
    }
}

void FileLogger::log(LogLevel level, const String& message, String endchar) {
    File file = LittleFS.open(_filename, "a");
    if (!file) {
        app_log_e("Failed to open file for writing");
        _isError = true;
        return;
    }
    String totalMessage = message + endchar;
    if (level != NO_LEVEL) {
        totalMessage = '[' + getLevelString(level) + "]: " + totalMessage;
    }
    file.print(totalMessage);
    file.close();
    _isError = false;
}

bool FileLogger::getIsError() const{
    return _isError;
}

LoggerManager mainLog;
