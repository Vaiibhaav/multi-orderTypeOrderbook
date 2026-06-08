// Constants.h
#pragma once
#include <cstdint>
#include <memory>
#include <list>
#include <vector>
// Alias simple int types
using Price = std::int32_t;
using Quantity = std::uint32_t;
using OrderId = std::uint64_t;
// Forward declare Order so we can create pointers to it without including its whole definition
class Order;
// Order aliases
// since we are going to store a asingle order in multpile datastructures in our orderbook, we w'll be using refernece semantics to make things a lot easirer,
// order can be stored in both a order dictionaries and a buy or asked based dictionary

// list gives itr which cannot be invalidated,should we use vector?
// You absolutely should use std::list here, and your reasoning is 100% correct. In an order book, people cancel their orders all the time. This means you frequently have to delete an order from the middle of the line.
// If you used std::vector, deleting an order in the middle forces the computer to shift every single order behind it forward by one spot. This is very slow, and it breaks any saved iterators you have.
// With std::list (a linked list), you can pluck an order out of the middle instantly without disturbing anyone else in line.
using OrderPointer = std::shared_ptr<Order>;
using OrderPointers = std::list<OrderPointer>;

