/*-*- mode:c;indent-tabs-mode:nil;c-basic-offset:2;tab-width:8;coding:utf-8 -*-│
│vi: set net ft=c ts=2 sts=2 sw=2 fenc=utf-8                                :vi│
╞══════════════════════════════════════════════════════════════════════════════╡
│ Copyright 2023 Justine Alexandra Roberts Tunney                              │
│                                                                              │
│ Permission to use, copy, modify, and/or distribute this software for         │
│ any purpose with or without fee is hereby granted, provided that the         │
│ above copyright notice and this permission notice appear in all copies.      │
│                                                                              │
│ THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL                │
│ WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED                │
│ WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE             │
│ AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL         │
│ DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR        │
│ PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER               │
│ TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR             │
│ PERFORMANCE OF THIS SOFTWARE.                                                │
╚─────────────────────────────────────────────────────────────────────────────*/
#include "libc/calls/calls.h"
#include "libc/calls/ucontext.h"
#include "libc/limits.h"
#include "libc/mem/gc.internal.h"
#include "libc/runtime/runtime.h"
#include "libc/runtime/stack.h"
#include "libc/runtime/symbols.internal.h"
#include "libc/str/str.h"
#include "libc/sysv/consts/sig.h"
#include "libc/testlib/subprocess.h"
#include "libc/testlib/testlib.h"
#include "libc/thread/thread.h"
#include "libc/x/x.h"
#include "third_party/libcxx/math.h"

ucontext_t uc;
char testlib_enable_tmp_setup_teardown;

void itsatrap(int x, int y) {
  *(int *)(intptr_t)x = scalbn(x, y);
}

TEST(makecontext, test) {
  SPAWN(fork);
  getcontext(&uc);
  uc.uc_link = 0;
  uc.uc_stack.ss_sp = NewCosmoStack();
  uc.uc_stack.ss_size = GetStackSize();
  makecontext(&uc, exit, 1, 42);
  setcontext(&uc);
  EXITS(42);
}

TEST(makecontext, backtrace) {
  SPAWN(fork);
  ASSERT_SYS(0, 0, close(2));
  ASSERT_SYS(0, 2, creat("log", 0644));
  getcontext(&uc);
  uc.uc_link = 0;
  uc.uc_stack.ss_sp = NewCosmoStack();
  uc.uc_stack.ss_size = GetStackSize();
  makecontext(&uc, itsatrap, 2, 123, 456);
  setcontext(&uc);
  EXITS(128 + SIGSEGV);
  if (!GetSymbolTable()) return;
  char *log = gc(xslurp("log", 0));
  EXPECT_NE(0, strstr(log, "itsatrap"));
  EXPECT_NE(0, strstr(log, "runcontext"));
  EXPECT_NE(0, strstr(log, "makecontext_backtrace"));
}