/*
** CrabeLoader
** File description:
** exception
*/

#ifndef EXCEPTION_HPP_
#define EXCEPTION_HPP_

#include <exception>
#include <string>

// Severity of an exception. Drives how log() behaves:
//  - LOG     -> informational log line
//  - WARNING -> warning log line
//  - ERROR   -> error log line
//  - CRASH   -> error log line, then terminates the program
enum class Severity {
    LOG,
    WARNING,
    ERR,
    CRASH
};

class HookException : public std::exception {
    public:
        explicit HookException(Severity severity, std::string message);

        const char* what() const noexcept override;

        Severity severity() const noexcept;

        // Emits this exception to the global Logger at the matching level
        // (LOG/WARNING/ERR/CRASH); CRASH terminates the program afterwards.
        void log() const;

    private:
        Severity _severity;
        std::string _message;
};

#endif /* !EXCEPTION_HPP_ */
