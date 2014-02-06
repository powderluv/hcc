// RUN: %amp_device -D__GPU__ %s -m32 -emit-llvm -c -S -O2 -o %t.ll && mkdir -p %t
// RUN: %clamp-device %t.ll %t/kernel.cl
// RUN: pushd %t && objcopy -B i386:x86-64 -I binary -O elf64-x86-64 kernel.cl kernel.o && popd
// RUN: %gtest_amp %t/kernel.o %s -o %t.out && %t.out

#include <amp.h>
#include <stdlib.h>
#include <iostream>
#ifndef __GPU__
#include <gtest/gtest.h>
#endif

class myVecAdd {
 public:
  // CPU-side constructor. Written by the user
  myVecAdd(Concurrency::array_view<int>& a,
    Concurrency::array_view<int> &b,
    Concurrency::array_view<int> &c):
    a_(a), b_(b), c_(c) {
  }
  void operator() (Concurrency::index<1> idx) restrict(amp) {
    c_[idx] = a_[idx]+b_[idx];
  }
 private:
  Concurrency::array_view<int> a_, b_, c_;
};
void bar(void) restrict(amp,cpu) {
  int foo = reinterpret_cast<intptr_t>(&myVecAdd::__cxxamp_trampoline);
}
#ifndef __GPU__
TEST(Design, Final) {
  const int vecSize = 100;

  // Alloc & init input data
  Concurrency::extent<1> e(vecSize);
  Concurrency::array<int, 1> a(vecSize);
  Concurrency::array<int, 1> b(vecSize);
  Concurrency::array<int, 1> c(vecSize);
  int sum = 0;
  for (Concurrency::index<1> i(0); i[0] < vecSize; i++) {
    a[i] = 100.0f * rand() / RAND_MAX;
    b[i] = 100.0f * rand() / RAND_MAX;
    sum += a[i] + b[i];
  }

  Concurrency::array_view<int> ga(a);
  Concurrency::array_view<int> gb(b);
  Concurrency::array_view<int> gc(c);
  myVecAdd mf(ga, gb, gc);
  Concurrency::parallel_for_each(
    e,
    mf);

  int error = 0;
  for(unsigned i = 0; i < vecSize; i++) {
    error += gc[i] - (ga[i] + gb[i]);
  }
  EXPECT_EQ(error, 0);
}
#endif
