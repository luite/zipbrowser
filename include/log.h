#ifndef __LOG_H__
#define __LOG_H__

/**
 * File Name  : log.h
 *
 * Description: Macros for debug printing
 *
 * The value of ERREGLOG_X_ON (with X one of LOGGING, WARNING or ERROR)
 * determines whether a message will be printed. The value as defined
 * in this file can be overruled by defining them before the inclusion of this file.
 *
 * This file is best included in c files only.
 */

/*
 * This file is part of erbrowser.
 *
 * erbrowser is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * erbrowser is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * Copyright (C) 2009 iRex Technologies B.V.
 * All rights reserved.
 */

#include <stdio.h>
#include "config.h"

#define LOG_PREFIX PACKAGE_NAME

#ifndef USE_SYSLOG
#define USE_SYSLOG 0
#endif

#ifndef LOGGING_ON
#define LOGGING_ON 0
#endif

#ifndef WARNING_ON
#define WARNING_ON 1
#endif

#ifndef ERROR_ON
#define ERROR_ON 1
#endif


#if (USE_SYSLOG)
#include <syslog.h>
#define LOG_OPEN(X) openlog(X, LOG_PID | LOG_NDELAY, LOG_USER)
#define LOG_CLOSE() closelog()
#else
#define LOG_OPEN(X) do {} while (0)
#define LOG_CLOSE() do {} while (0)
#endif

#if (LOGGING_ON)
#if (USE_SYSLOG)
#define LOGPRINTF(format, args...) syslog(LOG_INFO | LOG_USER, __FILE__ ":%d,%s() " format "\n", __LINE__, __func__ , ##args)
#else
#define LOGPRINTF(format, args...) fprintf(stderr, "(" LOG_PREFIX "_L)" __FILE__ ":%d,%s() " format "\n", __LINE__, __func__ , ##args)
#endif
#else
#define LOGPRINTF(format, args...) do {} while (0)
#endif

#if (WARNING_ON)
#if (USE_SYSLOG)
#define WARNPRINTF(format, args...) syslog(LOG_WARNING | LOG_USER, __FILE__ ":%d,%s() " format "\n", __LINE__, __func__ , ##args)
#else
#define WARNPRINTF(format, args...) fprintf(stderr, "(" LOG_PREFIX "_W)" __FILE__ ":%d,%s() " format "\n", __LINE__, __func__ , ##args)
#endif
#else
#define WARNPRINTF(format, args...) do {} while (0)
#endif

#if (ERROR_ON)
#include <errno.h>
#include <string.h>
#if (USE_SYSLOG)
#define ERRORPRINTF(format, args...) syslog(LOG_ERR | LOG_USER, __FILE__ ":%d,%s() --- " format "\n", __LINE__, __func__ , ##args)
#define ERRNOPRINTF(format, args...) syslog(LOG_ERR | LOG_USER, __FILE__ ":%d,%s() --- " format ", errno [%d] [%s]\n", __LINE__, __func__ , ##args, errno, strerror(errno))
#else
#define ERRORPRINTF(format, args...) fprintf(stderr, "(" LOG_PREFIX "_E)" __FILE__ ":%d,%s() --- " format "\n", __LINE__, __func__ , ##args)
#define ERRNOPRINTF(format, args...) fprintf(stderr, "(" LOG_PREFIX "_E)" __FILE__ ":%d,%s() --- " format ", errno [%d] [%s]\n", __LINE__, __func__ , ##args, errno, strerror(errno))
#endif
#else
#define ERRORPRINTF(format, args...) do {} while (0)
#define ERRNOPRINTF(format, args...) do {} while (0)
#endif

#endif // __LOG_H__
