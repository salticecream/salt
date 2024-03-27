#pragma once
#include <exception>
#include <string>
#include <vector>
#include <iostream>
#include <sstream>
#include <limits>
#include <cstdlib>
#include <fstream>

#ifdef _WIN32
#define NOMINMAX 1
#define SALT_WINDOWS 1
namespace Windows {
#include <Windows.h>
}
#else
#error "saltc is currently only available for Windows!"
#endif

// Program may work in unexpected ways if these conditions are not satisfied
static_assert(sizeof(char) == 1);
static_assert(sizeof(int) == 4);
static_assert(sizeof(size_t) == 8);
static_assert(sizeof(long long) == 8);
static_assert(EXIT_SUCCESS == 0);
static_assert(EXIT_FAILURE == 1);

// Global variables in global namespace
extern bool any_compile_error_occured;


namespace salt {
    class TextColor; // defined below

    class Exception : public std::exception {
    protected:
	    std::string what_;
    public:
        Exception(const char* w) : what_(w) {}
        Exception(const std::string& w) : what_(w) {}
        Exception(std::string&& w) : what_(w) {}
	    virtual const char* what() const noexcept override final { return what_.c_str(); }
        virtual ~Exception() = default;
        operator std::string() const { return what_; }
    };

    template<typename T>
    std::string to_string(const std::vector<T>& vec) {
        std::stringstream ss;
        ss << "[";
        if (!vec.empty()) {
            ss << vec[0];
            for (size_t i = 1; i < vec.size(); i++) {
                ss << ", " << vec[i];
            }
        }
        ss << "]";
        return ss.str();
    }

    // like std::cout but MAY ignore all input (if deactivated)
    class MaybeStream {
    private:
        bool print_to_stdout_;
    public:
        void activate();
        void deactivate();
        bool is_active() const;

        MaybeStream() : print_to_stdout_(false) {}

        template<typename T>
        MaybeStream& operator<<(const T& t) {
            if (print_to_stdout_)
                std::cout << t;
            return *this;
        }

        MaybeStream& operator<<(std::ostream& (*manipulator)(std::ostream&)) {
            if (print_to_stdout_)
                manipulator(std::cout);
            return *this;
        }


    };

    extern MaybeStream dbout;
    extern MaybeStream dberr;
    extern MaybeStream dboutv;
    extern MaybeStream dberrv;

    // expects a null-terminated string that corresponds to a number. anything other than digits, a '.' or a leading '-' will cause an error.
    long long atoll(const char* s);
    unsigned long long atoull(const char* s);
    int atoi(const char* s);
    std::string reverse(const std::string& s);
    std::string f_string(const char* format, ...);
    long long llrand();


    enum class Result_e {
        OK = 0,
        ERR = 1
    };

    void print_colored(const std::string& str, const TextColor tc);
    void print_warning(const std::string& warning);
    void print_error(const std::string& error);

    [[noreturn]]
    void print_fatal(const std::string& error, int exit_code = 1);

    /// @todo: maybe one day make a better Result class
    /* 
    * A class that represents either a successful operation or an exception.
    * Allows a function to either return a literal T or a literal Exception instead of throwing it.
    * Additionally, evaluates to True iff the Result contains a T, or False otherwise.
    * This allows for if (Result res = ...) expressions.
    */

    template <typename T>
    class Result {
    private:
        std::unique_ptr<T> success_;
        salt::Exception fail_;
        bool is_ok_;
        bool is_initialized_;
    public:
        Result(Result_e result, T&& success) {
            success_ = std::make_unique<T>(std::forward<T>(success));
            is_ok_ = result == Result_e::OK;
            is_initialized_ = true;
            fail_ = "Uninitialized exception in Result";
        }

        Result(T&& success) : fail_("Uninitialized exception in Result") {
            success_ = std::make_unique<T>(std::forward<T>(success));
            is_ok_ = true;
            is_initialized_ = true;
        }

        Result(Exception&& fail) : fail_(fail){
            success_ = nullptr;
            is_ok_ = false;
            is_initialized_ = true;
        }

        /*
        Result(Result_e result, const T&& success) {
            success_ = std::make_unique<T>(success);
            is_ok_ = result == Result_e::OK;
            is_initialized_ = true;
        }
        */

        Result(Result_e result, Exception&& fail) {
            success_ = nullptr;
            fail_ = fail;
            is_ok_ = false;
            is_initialized_ = true;
        }

        Result(Result_e result, const Exception& fail) {
            success_ = nullptr;
            fail_ = fail;
            is_ok_ = false;
            is_initialized_ = true;
        }

        Result() {
            success_ = nullptr;
            is_initialized_ = false;
        }
        
        // Trying to use this function gives me a severe brain ache
        // If you get a memory.h line 3465 bug, this is the culprit
        Result(const Result& res) {
            is_ok_ = res.is_ok_;
            is_initialized_ = res.is_initialized_;
            if (res.is_initialized_) {
                success_ = res.success_ ? std::make_unique<T>(*res.success_) : nullptr;
                fail_ = res.fail_;
            }
            
        }

        bool is_ok() const noexcept { return is_ok_; }
        operator bool() const noexcept { return is_ok(); }

        T unwrap() {
            if (!is_initialized_)
                print_fatal("tried to use uninitialized Result");
            if (is_ok()) 
                return std::move(*success_); 
            else 
                print_fatal(std::string("Tried to unwrap exception: ") + fail_.what()); 
        }

        T unwrap_or(T&& default_value) {
            if (!is_initialized_)
                print_fatal("tried to use uninitialized Result");
            if (is_ok()) 
                return std::move(*success_); 
            else 
                return default_value; 
        }

        salt::Exception&& unwrap_err() {
            if (!is_initialized_)
                print_fatal("tried to use uninitialized Result");
            if (!is_ok()) 
                return std::move(fail_); 
            else 
                print_fatal("tried to call unwrap_err() on an ok value"); 
        }

    };

    template <>
    class Result<void> {
    private:
        salt::Exception fail_;
        bool is_ok_;
    public:
        Result(void) : fail_("Uninitialized exception in Result<void>") {
            is_ok_ = true;
        }

        Result(Result_e res) : fail_("Unknown exception in Result<void>") {
            is_ok_ = true;
        }

        Result(Exception&& fail) : fail_(fail) {
            is_ok_ = false;
        }

        Result(const Exception& fail) : fail_(fail) {
            is_ok_ = false;
        }

        // Trying to use this function gives me a severe brain ache
        // If you get a memory.h line 3465 bug, this is the culprit

        bool is_ok() const noexcept { return is_ok_; }
        operator bool() const noexcept { return is_ok(); }

        void unwrap() {
            if (!is_ok())
                print_fatal(std::string("Tried to unwrap exception: ") + fail_.what());
        }

        salt::Exception&& unwrap_err() {
            if (!is_ok())
                return std::move(fail_);
            else
                print_fatal("tried to call unwrap_err() on an ok value");
        }

    };

    


    #ifdef SALT_WINDOWS
    enum class Color : Windows::WORD {
        BLACK = 0,
        BLUE = 1,
        GREEN = 2,
        CYAN = 3,
        RED = 4,
        MAGENTA = 5,
        YELLOW = 6,
        WHITE = 7,
        GRAY = 8,
        LIGHT_BLUE = 9,
        LIGHT_GREEN = 10,
        LIGHT_CYAN = 11,
        LIGHT_RED = 12,
        LIGHT_MAGENTA = 13,
        LIGHT_YELLOW = 14,
        BRIGHT_WHITE = 15
    };

    class TextColor {
    private:
        Windows::WORD color_;

    public:
        explicit TextColor(Windows::WORD color) : color_(color) {}
        TextColor(Color color) : color_(Windows::WORD(color)) {}
        friend std::ostream& operator<<(std::ostream& os, const TextColor color);
        TextColor operator|(const TextColor tc) const { return TextColor(color_ | tc.color_); }
        static void set(const TextColor color);

        friend std::ostringstream& operator<<(std::ostringstream& oss, const TextColor& color) = delete;
    };

    #endif /* ifdef SALT_WINDOWS */


}

template <typename T>
T&& cast_to_rvalue(T& t) {
    return (T&&) t;
}

// "this feature is not yet implemented!"
#define TODO() (salt::print_fatal((std::string("Not yet implemented in ") + __FUNCTION__ + " at " __FILE__ + ", line " + std::to_string(__LINE__)).c_str()))