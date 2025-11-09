#ifndef ERROR_HPP
#define ERROR_HPP

#include <string>
#include <exception>
#include <optional>
#include <glog/logging.h>

namespace kaiten {

// Base class for all Kaiten-related errors
class KaitenError : public std::exception {
public:
    explicit KaitenError(std::string message) : message_(std::move(message)) {}
    
    const char* what() const noexcept override {
        return message_.c_str();
    }

    virtual const char* type() const noexcept {
        return "KaitenError";
    }

protected:
    std::string message_;
};

// HTTP-related errors
class HttpError : public KaitenError {
public:
    HttpError(int status_code, std::string message)
        : KaitenError(std::move(message)), status_code_(status_code) {}
    
    int status_code() const noexcept { return status_code_; }
    
    const char* type() const noexcept override {
        return "HttpError";
    }

private:
    int status_code_;
};

// API-specific errors
class ApiError : public KaitenError {
public:
    explicit ApiError(std::string message) : KaitenError(std::move(message)) {}
    
    const char* type() const noexcept override {
        return "ApiError";
    }
};

// Validation errors
class ValidationError : public KaitenError {
public:
    explicit ValidationError(std::string message) : KaitenError(std::move(message)) {}
    
    const char* type() const noexcept override {
        return "ValidationError";
    }
};

// Rate limiting errors
class RateLimitError : public HttpError {
public:
    RateLimitError(int status_code, std::string message, int retry_after)
        : HttpError(status_code, std::move(message)), retry_after_(retry_after) {}
    
    int retry_after() const noexcept { return retry_after_; }
    
    const char* type() const noexcept override {
        return "RateLimitError";
    }

private:
    int retry_after_;
};

// Authentication errors
class AuthError : public HttpError {
public:
    AuthError(int status_code, std::string message)
        : HttpError(status_code, std::move(message)) {}
    
    const char* type() const noexcept override {
        return "AuthError";
    }
};

// Generic Result type for error handling
template<typename T>
class Result {
public:
    // Constructors for success case
    explicit Result(T value) : value_(std::move(value)) {}
    
    // Constructors for error case
    explicit Result(std::unique_ptr<KaitenError> error)
        : error_(std::move(error)) {}
    
    // Check if the result contains a value
    bool has_value() const { return value_.has_value(); }
    
    // Check if the result contains an error
    bool has_error() const { return error_ != nullptr; }
    
    // Get the value (throws if there's an error)
    const T& value() const {
        if (error_) {
            throw *error_;
        }
        return *value_;
    }
    
    // Get the value or a default
    T value_or(T default_value) const {
        if (error_) {
            return default_value;
        }
        return *value_;
    }
    
    // Get the error (throws if there's a value)
    const KaitenError& error() const {
        if (!error_) {
            throw std::runtime_error("Result contains a value, not an error");
        }
        return *error_;
    }

    // Map function to transform the value while preserving the error handling
    template<typename U>
    Result<U> map(std::function<U(const T&)> f) const {
        if (error_) {
            return Result<U>(std::unique_ptr<KaitenError>(error_->clone()));
        }
        return Result<U>(f(*value_));
    }

private:
    std::optional<T> value_;
    std::unique_ptr<KaitenError> error_;
};

// Helper functions to create Results
template<typename T>
Result<T> make_success(T value) {
    return Result<T>(std::move(value));
}

template<typename T>
Result<T> make_error(std::unique_ptr<KaitenError> error) {
    return Result<T>(std::move(error));
}

template<typename T>
Result<T> make_http_error(int status_code, const std::string& message) {
    return Result<T>(std::make_unique<HttpError>(status_code, message));
}

template<typename T>
Result<T> make_api_error(const std::string& message) {
    return Result<T>(std::make_unique<ApiError>(message));
}

template<typename T>
Result<T> make_validation_error(const std::string& message) {
    return Result<T>(std::make_unique<ValidationError>(message));
}

} // namespace kaiten

#endif // ERROR_HPP