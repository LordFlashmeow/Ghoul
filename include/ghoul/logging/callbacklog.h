/**************************************************************************************************
 * GHOUL                                                                                          *
 * General Helpful Open Utility Library                                                           *
 *                                                                                                *
 * Copyright (c) 2012 Alexander Bock                                                              *
 *                                                                                                *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software  *
 * and associated documentation files (the "Software"), to deal in the Software without           *
 * restriction, including without limitation the rights to use, copy, modify, merge, publish,     *
 * distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the  *
 * Software is furnished to do so, subject to the following conditions:                           *
 *                                                                                                *
 * The above copyright notice and this permission notice shall be included in all copies or       *
 * substantial portions of the Software.                                                          *
 *                                                                                                *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING  *
 * BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND     *
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,   *
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, *
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.        *
 *************************************************************************************************/

#ifndef __CALLBACKLOG_H__
#define __CALLBACKLOG_H__

#include "log.h"

#include <functional>

namespace ghoul {
namespace logging {

/**
 * A concrete subclass of Log that passes logs to the provided callback function. The call back is
 * specified using the functional/bind mechanism found in the Technical Report 1 or C++11 standard.
 * Trying to log messages when the callback object has been deleted is undefined behavior.
 * The formatting of the log messages depends on the stamping settings. The various possibilities
 * are:
 * "[DATE | TIME] CATEGORY (LEVEL) MESSAGE"
 * "[DATE] CATEGORY (LEVEL) MESSAGE"
 * "[TIME] CATEGORY (LEVEL) MESSAGE"
 * And the remaining possibilities with CATEGORY and (LEVEL) missing.
 */

class CallbackLog : public Log {
public:
    /**
    * Constructor that calls Log::Log(bool, bool, bool, bool) constructor.
    * \param [in] callbackFunction The callback function that is called for each log message. The
    * callback function needs to be passed using the <code>bind</code> function found either in the
    * <code>std</code> or <code>std::tr1</code> namespace.
    * <url>http://en.cppreference.com/w/cpp/utility/functional/bind</url>
    * \param [in] timeStamping Determines if the log should print the time when a message is
    * logged in the log messages
    * \param [in] dateStamping Determines if the log should print the time when a message is
    * logged in the log messages
    * \param [in] categoryStamping Determines if the log should print the categories in the log
    * messages
    * \param [in] logLevelStamping Determines if the log should print the log level in the log
    * messages
    */
    CallbackLog(std::function<void (const std::string&)> callbackFunction, bool timeStamping = true,
        bool dateStamping = true, bool categoryStamping = true, bool logLevelStamping = true);

    /**
     * Method that logs a message with a given level and category to the console.
     * \param [in] level The log level with which the message shall be logged
     * \param [in] category The category of this message. Can be used by each subclass individually
     * \param [in] message The message body of the log message
     */
    void log(LogManager::LogLevel level, const std::string& category, const std::string& message);

protected:
    std::function<void (const std::string&)> _callbackFunction;
};

}
}

#endif
