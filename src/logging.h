/**
 * ESP32 Logging
 * Copyright (c) 2020 clausgf@github. See LICENSE.md for legal information.
 * 
 * Simple logging layer for printf() style logging.
 * 
 * Every logging configuration consists a Logger configured with a LogFormatter
 * and a LogHandler (for writing logs to the output). A default logger
 * named l is declared but not defined. It must be initialized externally
 * (e.g. in main.cpp), e.g.:
 * 
 * auto logHandler = SerialLogHandler(115200, true);
 * auto logFormatter = DefaultLogFormatter("mytag_eg_app_name");
 * Logger l = Logger(&logFormatter, &logHandler);
 * 
 * It is then available in every module which includes this file.
 */

#ifndef LOGGING_H
#define LOGGING_H

#include <cstdio>
#include <cstdarg>

#include <IPAddress.h>
#include <lwip/sockets.h>


// ***************************************************************************

/**
 * The LogFormatter writes formatted log message to a buffer. It can optionally
 * add information like a timestamp, a tag, the log level, ... and apply some 
 * colorization or formatting.
 */
class LogFormatter {
public:
    /**
     * Write message to buffer.
     */
    virtual int format(char *buffer, int bufLen, int level, const char *message) = 0;
    virtual ~LogFormatter() {}
};

// ***************************************************************************

/**
 * Simple log formatter adding ms timestamp, a tag, and the log level.
 */
class DefaultLogFormatter: public LogFormatter {
public:
    DefaultLogFormatter(const char *tag);
    virtual int format(char *buffer, int bufLen, int level, const char *message);
private:
    const char *_tag;
};

// ***************************************************************************

/**
 * The LogHandler writes a preformatted log buffer to the output, e.g. a 
 * serial port or a network log server.
 */
class LogHandler {
public:
    virtual void write(const char *message, int len) = 0;
    virtual ~LogHandler() {}
};

// ***************************************************************************

/**
 * Write logs 
 */
class SerialLogHandler: public LogHandler {
public:
    SerialLogHandler(unsigned long baudRate = 115200, bool enableDebugOutput = false);
    virtual void write(const char *message, int len);
};

// ***************************************************************************

class UdpLogHandler: public LogHandler {
public:
    UdpLogHandler(const char *host, uint16_t port);
    UdpLogHandler(IPAddress ip, uint16_t port);
    virtual void write(const char *buffer, int buflen);
private:
    const char *_host;
    IPAddress _ip;
    struct sockaddr_in _recipient;
    uint16_t _port;
    int _udp_server;
};

// ***************************************************************************

/**
 * Logger class providing log levels and user friendly log functions.
 */
class Logger {
public:
    enum LogLevel { CRITICAL = 50, ERROR = 40, WARNING = 30, INFO = 20, DEBUG = 10, NOTSET = 0 };

    Logger(LogFormatter* logFormatterPtr, LogHandler* logHandlerPtr);
    void setFormatter(LogFormatter* logFormatterPtr);
    void setHandler(LogHandler* logHandlerPtr);

    /**
     * Set log level, all output with a lower level is discarded.
     */
    void setLevel(LogLevel level);

    /**
     * Log output with given level, format and arguments referenced by ap.
     */
    void logv(LogLevel level, const char* format, va_list ap);

    /**
     * Log output with given level, format and printf()-style arguments.
     */
    void logf(LogLevel level, const char* format...);

    // --- logging helpers ---
    void critical(const char* format...);
    void error(const char* format...);
    void warn(const char* format...);
    void info(const char* format...);
    void debug(const char* format...);

private:
    LogLevel _level = NOTSET;
    LogFormatter* _logFormatterPtr;
    LogHandler* _logHandlerPtr;
};


extern Logger l;

#endif
