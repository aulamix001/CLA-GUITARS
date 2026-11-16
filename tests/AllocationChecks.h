#pragma once
#include <atomic>
#include <cstdlib>
#include <new>
static std::atomic<bool> watch{false};
static std::atomic<size_t> allocations{0};
void* operator new(std::size_t n) { if(watch)++allocations; if(auto* p=std::malloc(n?n:1))return p;throw std::bad_alloc(); }
void* operator new[](std::size_t n) {return ::operator new(n);}
void operator delete(void* p) noexcept {std::free(p);}
void operator delete[](void* p) noexcept {std::free(p);}
void operator delete(void* p,std::size_t) noexcept {std::free(p);}
void operator delete[](void* p,std::size_t) noexcept {std::free(p);}
