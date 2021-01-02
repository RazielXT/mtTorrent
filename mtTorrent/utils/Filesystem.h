#pragma once

#ifdef _WIN32
constexpr char pathSeparator = '\\';
#else
constexpr char pathSeparator = '/';
#endif