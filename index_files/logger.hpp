#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <iostream>
#include <fstream>
#include <mutex>

using std::string;

namespace logging
{

class Logger
{
public:
    explicit Logger(const string& title, const string& filename, bool verbose = true):
        title(title), verbose(verbose)
    {
        fileStream.open(filename);
    }

    explicit Logger(const string& title, bool verbose = true): title(title), verbose(verbose)
    {
    }

    ~Logger()
    {
        if (fileStream.is_open())
        {
            fileStream.close();
        }
    }

    void setVerbose(bool verbose)
    {
        this->verbose = verbose;
    }

    template <typename ... Types>
    void operator()(const Types& ... messages)
    {
        log(messages ...);
    }

    template <typename ... Types>
    void log(const Types& ... messages)
    {
        std::lock_guard<std::mutex> lock(logMutex);
        if (verbose)
        {
            std::cerr << title << ": ";
        }
        if (fileStream.is_open())
        {
            fileStream << title << ": ";
        }

        logImpl(messages ...);

        if (verbose)
        {
            std::cerr << std::endl;
        }
        if (fileStream.is_open())
        {
            fileStream << std::endl;
        }
    }

private:
    void logImpl()
    {
    }

    template <typename FirstType, typename ...Types>
    void logImpl(const FirstType& first, const Types& ... messages)
    {
        if (verbose)
        {
            std::cerr << first;
        }
        if (fileStream.is_open())
        {
            fileStream << first;
        }
        logImpl(messages ...);
    }

    std::ofstream fileStream;
    string title;
    bool verbose;
    static std::mutex logMutex;

};

class Log
{
public:
    static Logger debug;
    static Logger trace;
    static Logger info;
    static Logger warn;
    static Logger error;
};

} // namespace logging

#endif // LOGGER_HPP
