/**
 * ESP32 Logging
 * Copyright (c) 2020 clausgf@github. See LICENSE.md for legal information.
 */

#include <stdint.h>
#include <cstdio>
#include <cstdarg>

#include <IPAddress.h>

#ifdef ESP32
#include <FreeRTOS.h>
extern "C" {
#include <lwip/sockets.h>
#include <lwip/netdb.h>
#include <errno.h>
}
#include <esp_log.h>
#else
    #error "Architecture/Framework is not supported. Supported: ESP32 (IDF and Arduino)"
#endif

#include "logging.h"

static const char* module_tag = "logging";

// ***************************************************************************

DefaultLogFormatter::DefaultLogFormatter(const char *tag): _tag(tag) {
}

int DefaultLogFormatter::format(char *buffer, int bufLen, int level, const char *message) {
    unsigned long ms = 0;
#ifdef ESP32
    ms = xTaskGetTickCount() * (1000 / configTICK_RATE_HZ);
#endif
    int len = snprintf(buffer, bufLen, "%s:%02d:%lu.%03lu:%s\n", _tag, level, ms / 1000, ms % 1000, message);
    return len;
}

// ***************************************************************************

SerialLogHandler::SerialLogHandler(unsigned long baudRate, bool enableDebugOutput) {
}

void SerialLogHandler::write(const char *message, int len) {
    printf("%s", message);
}

// ***************************************************************************

UdpLogHandler::UdpLogHandler(const char *host, uint16_t port): _host(host), _ip(uint32_t(0)), _port(port), _udp_server(-1) {
}

UdpLogHandler::UdpLogHandler(IPAddress ip, uint16_t port): _host(nullptr), _ip(ip), _port(port), _udp_server(-1) {
}

void UdpLogHandler::write(const char *buffer, int buflen) {
    // DNS resolution if needed
    if (uint32_t(_ip) == 0 && _host != nullptr) {
        struct hostent *server;
        server = gethostbyname(_host);
        if (server == NULL){
            ESP_LOGE(module_tag, "could not get host from dns: %s->%d", _host, errno);
        } else {
            _ip = IPAddress((const uint8_t *)(server->h_addr_list[0]));
            _recipient.sin_addr.s_addr = (uint32_t)_ip;
            _recipient.sin_family = AF_INET;
            _recipient.sin_port = htons(_port);
        }
    }

    // create socket if needed, set socket options
    if (_udp_server == -1) {
        _udp_server = socket(AF_INET, SOCK_DGRAM, 0);
        if (_udp_server == -1) {
            ESP_LOGE(module_tag, "could not create socket: %d", errno);
        } else {
            fcntl(_udp_server, F_SETFL, O_NONBLOCK);
        }
    }

    // send the data
    if (_udp_server != -1)
    {
        int sent = sendto(_udp_server, buffer, buflen, 0, (struct sockaddr*) &_recipient, sizeof(_recipient));
        if (sent < 0) {
            ESP_LOGE(module_tag, "could not send data: %d", errno);
        }
    }

    printf("%s", buffer);
}

// ***************************************************************************

Logger::Logger(LogFormatter* logFormatterPtr, LogHandler* logHandlerPtr): 
    _logFormatterPtr(logFormatterPtr), _logHandlerPtr(logHandlerPtr) {
}

void Logger::setFormatter(LogFormatter* logFormatterPtr) {
    _logFormatterPtr = logFormatterPtr;
}

void Logger::setHandler(LogHandler* logHandlerPtr) {
    _logHandlerPtr = logHandlerPtr;
}

void Logger::setLevel(LogLevel level) {
    _level = level;
}

void Logger::logv(LogLevel level, const char* format, va_list ap) {
    constexpr int buflen = 256;
    char buffer[buflen];

    vsnprintf(buffer, buflen-1, format, ap);
    if ((level >= _level) && (_logFormatterPtr != nullptr) && (_logHandlerPtr != nullptr)) {
        char formattedBuffer[buflen];
        int len = _logFormatterPtr->format(formattedBuffer, buflen, level, buffer);
        _logHandlerPtr->write(formattedBuffer, len);
    }
}

void Logger::logf(LogLevel level, const char* format...) {
    va_list args;
    va_start(args, format);
    logv(level, format, args);
    va_end(args);
}

void Logger::critical(const char* format...) {
    va_list args;
    va_start(args, format);
    logv(CRITICAL, format, args);
    va_end(args);
}

void Logger::error(const char* format...) {
    va_list args;
    va_start(args, format);
    logv(ERROR, format, args);
    va_end(args);
}

void Logger::warn(const char* format...) {
    va_list args;
    va_start(args, format);
    logv(WARNING, format, args);
    va_end(args);
}

void Logger::info(const char* format...) {
    va_list args;
    va_start(args, format);
    logv(INFO, format, args);
    va_end(args);
}

void Logger::debug(const char* format...) {
    va_list args;
    va_start(args, format);
    logv(DEBUG, format, args);
    va_end(args);
}

// ***************************************************************************

