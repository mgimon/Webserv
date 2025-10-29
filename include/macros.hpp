#ifndef MACROS_HPP
#define MACROS_HPP

#define RESET   "\033[0m"
#define RED     "\033[31m"
#define GREEN   "\033[32m"
#define YELLOW  "\033[33m"
#define BLUE    "\033[34m"
#define GRAY    "\033[90m"
#define PINK    "\033[35m"
#define CYAN    "\033[36m"
#define WHITE   "\033[37m"

enum HttpStatusCode {
    STATUS_OK = 200,
    STATUS_CREATED = 201,
    STATUS_NO_CONTENT = 204,
    STATUS_BAD_REQUEST = 400,
    STATUS_UNAUTHORIZED = 401,
    STATUS_FORBIDDEN = 403,
    STATUS_NOT_FOUND = 404,
    STATUS_INTERNAL_ERROR = 500,
    STATUS_NOT_IMPLEMENTED = 501
};

#endif
