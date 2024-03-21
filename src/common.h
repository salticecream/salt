#pragma once
#include <exception>
#include <string>
#include <vector>
#include <iostream>
#include <sstream>
#include <limits>
#include <cstdlib>
#include <fstream>

#define SALT_DEBUG_PRINT 1
#define SALT_DEBUG_PRINT_VERBOSE 0

#ifdef _WIN32
#define NOMINMAX 1
#define SALT_WINDOWS 1
namespace Windows {
#include <Windows.h>
}
#else
#error "saltc is currently only available for Windows!"
#endif

// Choose where salt::dbout (debug output) should be printed
#if SALT_DEBUG_PRINT || SALT_DEBUG_PRINT_VERBOSE

#define SALT_INTERNAL_DBOUT std::cout
#define SALT_INTERNAL_DBERR std::cerr

#else

#define SALT_INTERNAL_DBOUT salt::null_stream
#define SALT_INTERNAL_DBERR salt::null_stream

#endif // SALT_DEBUG_PRINT || SALT_DEBUG_PRINT_VERBOSE

// Global variables in global namespace
extern bool any_compile_error_occured;


namespace salt {

    class Exception : public std::exception {
    protected:
	    std::string what_;
    public:
        Exception(const char* w) : what_(w) {}
        Exception(const std::string& w) : what_(w) {}
        Exception(std::string&& w) : what_(w) {}
	    virtual const char* what() const noexcept override final { return what_.c_str(); }
        virtual ~Exception() = default;
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

    // like std::cout but ignores all input
    class NullStream : public std::ostream {
    public:
        NullStream() : std::ostream(nullptr) {}

        template<typename T>
        NullStream& operator<<(const T&) {
            // Do nothing
            return *this;
        }
    };

    extern NullStream null_stream;
    extern std::ostream& dbout; // If SALT_DEBUG_PRINT is 1 then std::cout, otherwise salt::null_stream
    extern std::ostream& dberr; // If SALT_DEBUG_PRINT is 1 then std::cerr, otherwise salt::null_stream

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
                throw Exception("tried to use uninitialized Result");
            if (is_ok()) 
                return std::move(*success_); 
            else 
                throw Exception(std::string("Tried to unwrap exception: ") + fail_.what()); 
        }

        T unwrap_or(T&& default_value) {
            if (!is_initialized_)
                throw Exception("tried to use uninitialized Result");
            if (is_ok()) 
                return std::move(*success_); 
            else 
                return default_value; 
        }

        salt::Exception&& unwrap_err() {
            if (!is_initialized_)
                throw Exception("tried to use uninitialized Result");
            if (!is_ok()) 
                return std::move(fail_); 
            else 
                throw Exception("tried to call unwrap_err() on an ok value"); 
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
                throw Exception(std::string("Tried to unwrap exception: ") + fail_.what());
        }

        salt::Exception&& unwrap_err() {
            if (!is_ok())
                return std::move(fail_);
            else
                throw Exception("tried to call unwrap_err() on an ok value");
        }

    };

    #ifdef SALT_WINDOWS
    class TextColor {
    private:
        Windows::WORD color_;
    public:
        explicit TextColor(Windows::WORD color) : color_(color) {}
        friend std::ostream& operator<<(std::ostream& os, const TextColor color);
        TextColor operator|(const TextColor tc) const { return TextColor(color_ | tc.color_); }
        static void set(const TextColor color);
    };
    #endif

    

}

// "this feature is not yet implemented!"
#define TODO() (throw std::exception((std::string("Not yet implemented in ") + __FUNCTION__ + " at " __FILE__ + ", line " + std::to_string(__LINE__)).c_str()))