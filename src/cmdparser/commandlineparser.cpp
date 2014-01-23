/*****************************************************************************************
 *                                                                                       *
 * GHOUL                                                                                 *
 * General Helpful Open Utility Library                                                  *
 *                                                                                       *
 * Copyright (c) 2012 Alexander Bock                                                     *
 *                                                                                       *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this  *
 * software and associated documentation files (the "Software"), to deal in the Software *
 * without restriction, including without limitation the rights to use, copy, modify,    *
 * merge, publish, distribute, sublicense, and/or sell copies of the Software, and to    *
 * permit persons to whom the Software is furnished to do so, subject to the following   *
 * conditions:                                                                           *
 *                                                                                       *
 * The above copyright notice and this permission notice shall be included in all copies *
 * or substantial portions of the Software.                                              *
 *                                                                                       *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,   *
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A         *
 * PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT    *
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF  *
 * CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE  *
 * OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                                         *
 ****************************************************************************************/

#include <ghoul/cmdparser/commandlineparser.h>
#include <ghoul/cmdparser/commandlinecommand.h>
#include <ghoul/logging/logmanager.h>
#include <map>
#include <iostream>

namespace {

    const std::string _loggerCat = "CommandlineParser";

/**
 * Extracts multiple arguments from a single list. <br>
 * If <code>count</code> is <code>-2</code>, arguments will be extracted, as long as no
 * new commands is found. <code>in[begin]</code> itself will not be extracted if this is
 * the command.<br>
 * If <code>count</code> is <code>-1</code>, the rest of the line will be extracted.<br>
 * If <code>count</code> is <code>> 0</code>, that many arguments will be extracted and
 * returned.
 */
int extractArguments(const std::vector<std::string> in, std::vector<std::string>& out,
                     const size_t begin, const int count)
{
    int num = 0;
    if (count == -1) {
        for (size_t i = 0; (i < in.size()) && ((begin+1+i) < in.size()) ; ++i, ++num)
            out.push_back(in[begin+1+i]);
    }
    else
        if (count == -2) {
            // Extract arguments until a new command is found
            // The '-' restriction is enforced in the #addCommand method
            for (size_t i = begin; (i < in.size()) && (in[i][0] != '-'); ++i, ++num)
                out.push_back(in[i]);
        }
    else {
        for (int i = 0; (i < count) && ((begin+1+i) < in.size()) ; ++i, ++num)
            out.push_back(in[begin+1+i]);
    }
    return num;
}

} // namespace

namespace ghoul {
namespace cmdparser {

CommandlineParser::CommandlineParser(const std::string& programName)
  : _commandForNamelessArguments(nullptr)
  , _programName(programName)
{}

CommandlineParser::~CommandlineParser() {
    for (CommandlineCommand* i : _commands)
        delete i;
    _commands.clear();
}

const std::string& CommandlineParser::programPath() const {
    return _programPath;
}

void CommandlineParser::setCommandLine(int argc, char** argv) {
    // argv[0] = program name
    // argv[i] = i-th argument
    if (argc > 0 && argv && argv[0])
        _programPath = argv[0];
    else
        _programPath = "";

    // Might be possible that someone calls us multiple times
    _arguments.clear();

    // Just add the arguments to the vector
    for (int i = 1; i < argc; ++i)
        _arguments.push_back(argv[i]);
}

bool CommandlineParser::execute() {
    // There is only one argument and this is either "-h" or "--help"
    // so display the help
    const bool isHelpArgument = ((_arguments[0] == "-h") || (_arguments[0] == "--help"));
    if ((_arguments.size() == 1) && isHelpArgument) {
        displayHelp();
        return false;
    }

    std::vector<std::string> argumentsForNameless;

    // We store the commands and parameters in a map to be able to execute them without
    // parsing the commandline again
    std::multimap<CommandlineCommand*, std::vector<std::string> > parameterMap;

    for (size_t i = 0 ; i < _arguments.size(); ++i) {
        // In the beginning we assume that we just started the loop or finished reading
        // parameters for the last command

        // Test if the next argument is a command or a parameter for a nameless argument
        // The restriction for '-' is enforced in the #addCommand method
        if (_arguments[i][0] != '-') {
            // The rest of the commands until the next '-' are for the nameless command
            const int number = extractArguments(_arguments, argumentsForNameless, i, -2);
            i += (number - 1);
        }
        else {
            // We have found a command
            CommandlineCommand* currentCommand = getCommand(_arguments[i]);

            // currentCommand = nullptr, if there wasn't a command with that specific
            // name or shortName
            if (currentCommand == 0) {
                LFATAL(_arguments[i] + " is not a valid command");
                displayUsage();
                return false;
            }

            std::vector<std::string> parameters;
            int number = extractArguments(_arguments, parameters, i, currentCommand->argumentNumber());
            i += number;

            // don't insert if the command doesn't allow multiple calls and already is in the map
            if (!currentCommand->allowsMultipleCalls() &&
                parameterMap.find(currentCommand) != parameterMap.end())
            {
                LFATAL(currentCommand->name() <<
                        " doesn't allow multiple calls in a single line");
                displayUsage();
                return false;
            }

            parameterMap.insert(std::make_pair(currentCommand, parameters));
        }
    }
    // We now have all the commands with the respective parameters stored in the map and
    // all the parameters for the nameless command is avaiable as well.

    // First step: Test, if we have nameless arguments even if we don't have a nameless
    // command. If so, bail out
    if (!argumentsForNameless.empty() && (_commandForNamelessArguments == nullptr)) {
        LFATAL("No appropriate command available for nameless parameters");
        displayUsage();
        return false;
    }

    // Second step: Check if every command is happy with the parameters assigned to it
    for (auto it = parameterMap.begin(); it != parameterMap.end(); ++it) {
        bool correct = it->first->checkParameters(it->second);
        if (!correct) {
            LFATAL("Wrong arguments for " << it->first->name() << ": " <<
                   it->first->errorMessage());
            displayUsage();
            return false;
        }
    }

    // Second-and-a-halfs step: Display pairs for (command,argument) if verbosity is wanted
    std::stringstream s;
    for (const std::string& arg : argumentsForNameless)
        s << " " << arg;
    LDEBUG("(Nameless argument:" << s.str() << ")");
    
    for (auto it = parameterMap.cbegin(); it != parameterMap.cend(); ++it) {
        s.clear();
        for (const std::string& arg : it->second)
            s << " " << arg;
        LDEBUG("(" << it->first->name() << ":" << s.str() << ")");
    }

    // Third step: Execute the nameless command if there are any arguments available
    if (!argumentsForNameless.empty()) {
        bool c = _commandForNamelessArguments->checkParameters(argumentsForNameless);

        if (c)
            _commandForNamelessArguments->execute(argumentsForNameless);
        else {
            LFATAL("One of the parameters for the nameless command was not correct");
            displayUsage();
            return false;
        }
    }

    // Fourth step: Execute the commands (this step is only done if everyone is happy up
    // until now)
    for (auto it = parameterMap.begin(); it != parameterMap.end(); ++it) {
        bool correct = it->first->execute(it->second);
        if (!correct) {
            LFATAL("The execution for " + (*it).first->name() + " failed");
            displayUsage();
            return false;
        }
    }
    
    // If we made it this far it means that all commands have been executed successfully
    return true;
}

bool CommandlineParser::addCommand(CommandlineCommand* cmd) {
    const bool nameFormatCorrect = cmd->name()[0] == '-';
    if (!nameFormatCorrect) {
        LFATAL("The name for a command has to start with an -");
        return false;
    }

    // Check, if either the name or the shortname is already present in the parser
    const bool nameExists = getCommand(cmd->name()) != nullptr;
    if (nameExists) {
        LERROR("The name for the command '" << cmd->name() << "' already existed");
        return false;
    }
    const bool hasShortname = cmd->shortName() != "";
    if (hasShortname) {
        const bool shortNameExists = getCommand(cmd->shortName()) != nullptr;
        if (shortNameExists) {
            LFATAL("The short name for the command '" << cmd->name() <<
                                                    "' already existed");
            return false;
        }
        const bool shortNameFormatCorrect = cmd->shortName()[0] == '-';
        if (!shortNameFormatCorrect) {
            LFATAL("The short name for the command '" << cmd->name() <<
                                                    "' has to start with an -");
            return false;
        }
        
    }

    // If we got this far, the names are valid
    // We don't need to check for duplicate entries as the names would be duplicate, too
    _commands.push_back(cmd);
    return true;
}

void CommandlineParser::addCommandForNamelessArguments(CommandlineCommand* cmd) {
    delete _commandForNamelessArguments;
    _commandForNamelessArguments = cmd;
}

void CommandlineParser::displayUsage(const std::string& command, std::ostream& stream) {
    std::string usageString = "Usage: ";

    if (command.empty()) {
        if (_programName != "")
            usageString += _programName + " ";

        if (_commandForNamelessArguments != 0)
            usageString += _commandForNamelessArguments->usage() + " ";

        for (const CommandlineCommand* const it : _commands) {
            if (it)
                usageString += "\n" + it->usage() + " ";
        }
    } else {
        for (const CommandlineCommand* const it : _commands) {
            if (it && (it->name() == command || it->shortName() == command))
                usageString += "\n" + it->usage() + " ";
        }
    }

    // Display via the std-out because no Logger-Prefix is wanted with the output
    stream << usageString << std::endl;
}

void CommandlineParser::displayHelp(std::ostream& stream) {
    displayUsage();
    stream << std::endl << std::endl << "Help:" << std::endl << "-----" << std::endl;

    for (const CommandlineCommand* const it : _commands)
        stream << it->help() << std::endl;
}

CommandlineCommand* CommandlineParser::getCommand(const std::string& shortOrLongName) {
    if (shortOrLongName == "")
        return nullptr;
    for (CommandlineCommand* const it : _commands) {
        if ((it->name() == shortOrLongName) || (it->shortName() == shortOrLongName))
            return it;
    }
    return nullptr;
}

} // namespace cmdparser
} // namespace voreen
