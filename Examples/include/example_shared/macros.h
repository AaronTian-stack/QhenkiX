#pragma once

#include <stdexcept>

#define THROW_IF_FALSE(result)                                   \
    do                                                           \
    {                                                            \
        if (!result)                                             \
        {                                                        \
            throw std::runtime_error("Something went wrong!\n"); \
        }                                                        \
    } while (0)

#define THROW_IF_TRUE(result)                                    \
    do                                                           \
    {                                                            \
        if ((result))                                            \
        {                                                        \
            throw std::runtime_error("Something went wrong!\n"); \
        }                                                        \
    } while (0)
