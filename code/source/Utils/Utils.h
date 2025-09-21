#pragma once
#include <functional>
#include <utility>
#include <memory>
#include <string>
#include <stdexcept>
#include <vector>
#include "Core/CoreDataTypes.h"

template<typename Class, typename rType, typename... Args>
std::function<rType(Args...)> GetMember(Class* instance, rType(Class::* member)(Args...))
{
    std::function<rType(Args...)> func = [instance, member](Args... args)
    {
        return (instance->*member)(std::forward<Args>(args)...);
    };

    return func;
}

float string_to_float(const std::string& string);

float optional_string_to_float(std::vector<std::string>& tokens, int index, float defaultValue);

int HexToInt(char hex);
bool HexToInt(char hexOne, char hexTwo, int& value);

Color string_to_color(const std::string& string);