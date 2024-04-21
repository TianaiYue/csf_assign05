/*
 * A5MS1
 * The Value Stack class
 * Cassie Zhang xzhan304
 * Tianai Yue tyue4
 */

#include "value_stack.h"
#include "exceptions.h"

ValueStack::ValueStack()
    // TODO: initialize member variable(s) (if necessary)
{
}

ValueStack::~ValueStack()
{
}

bool ValueStack::is_empty() const
{
    // TODO: implement
    return stack.empty(); 
}

void ValueStack::push( const std::string &value )
{
    // TODO: implement
    stack.push_back(value); 
}

std::string ValueStack::get_top() const
{
    // TODO: implement
    if (stack.empty()) {
        throw OperationException("Cannot access the top of an empty stack.");
    }
    return stack.back(); 
}

void ValueStack::pop()
{
    // TODO: implement
    if (stack.empty()) {
        throw OperationException("Cannot pop from an empty stack.");
    }
    stack.pop_back(); 
}
