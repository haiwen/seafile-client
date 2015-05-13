#include "test_stl.h"
#include <QSet>
#include <QtTest/QtTest>
#include <vector>
#include <cstring>
#include <cwchar>

#include "../src/utils/stl.h"

void STLTest::testBufferArry() {
  using utils::BufferArray;
  // constructor
  std::string test1("123456789abcdefg");
  BufferArray buffer(test1);
  QVERIFY(test1.size() + 1 == buffer.size());
  QVERIFY(buffer.capacity() == buffer.size());
  QVERIFY(strncmp(test1.data(), buffer.data(), test1.size()) == 0);

  // operator[]
  QVERIFY(test1[0] == '1');
  QVERIFY(test1[1] == '2');
  QVERIFY(test1[2] == '3');

  // reserve
  buffer.resize(30);
  QVERIFY(buffer.capacity() == 30);

  // shrink_to_fit
  buffer.shrink_to_fit();
  QVERIFY(buffer.capacity() == buffer.size());

  // resize
  buffer.resize(9);
  std::string test2("12345678");
  QVERIFY(test2.size() + 1 == buffer.size());
  QVERIFY(buffer.capacity() > buffer.size());
  QVERIFY(strncmp(test2.data(), buffer.data(), test2.size()) == 0);

  // shrink_to_fit
  buffer.shrink_to_fit();
  QVERIFY(buffer.capacity() == buffer.size());

#ifdef UTILS_CXX11_MODE
  // move
  std::string test3("gqjdiw913abc_123d");
  BufferArray other_buffer(test3);
  buffer = std::move(other_buffer);
  QVERIFY(test3.size() + 1 == buffer.size());
  QVERIFY(buffer.capacity() == buffer.size());
  QVERIFY(strncmp(test3.data(), buffer.data(), test3.size()) == 0);

  // constructor2
  const char test_string[] = "abcdefg";
  size_t test_size = sizeof(test_string);
  buffer = BufferArray(test_string);
  QVERIFY(test_size == buffer.size());
  QVERIFY(buffer.capacity() == buffer.size());
  QVERIFY(memcmp(test_string, buffer.data(), test_size) == 0);
#endif
}

void STLTest::testWBufferArry() {
  using utils::WBufferArray;
  // constructor
  std::wstring test1(L"123456789abcdefg");
  WBufferArray buffer(test1);
  QVERIFY(test1.size() + 1 == buffer.size());
  QVERIFY(buffer.capacity() == buffer.size());
  QVERIFY(wcsncmp(test1.data(), buffer.data(), test1.size()) == 0);

  // operator[]
  QVERIFY(test1[0] == L'1');
  QVERIFY(test1[1] == L'2');
  QVERIFY(test1[2] == L'3');

  // reserve
  buffer.resize(30);
  QVERIFY(buffer.capacity() == 30);

  // shrink_to_fit
  buffer.shrink_to_fit();
  QVERIFY(buffer.capacity() == buffer.size());

  // resize
  buffer.resize(9);
  std::wstring test2(L"12345678");
  QVERIFY(test2.size() + 1 == buffer.size());
  QVERIFY(buffer.capacity() > buffer.size());
  QVERIFY(wcsncmp(test2.data(), buffer.data(), test2.size()) == 0);

#ifdef UTILS_CXX11_MODE
  // move
  std::wstring test3(L"gqjdiw913abc_123d");
  WBufferArray other_buffer(test3);
  buffer = std::move(other_buffer);
  QVERIFY(test3.size() + 1 == buffer.size());
  QVERIFY(buffer.capacity() == buffer.size());
  QVERIFY(wcsncmp(test3.data(), buffer.data(), test3.size()) == 0);

  // constructor2
  const wchar_t test_string[] = L"abcdefg";
  size_t test_size = sizeof(test_string) / sizeof(wchar_t);
  buffer = WBufferArray(test_string);
  QVERIFY(test_size == buffer.size());
  QVERIFY(buffer.capacity() == buffer.size());
  QVERIFY(memcmp(test_string, buffer.data(), test_size) == 0);
#endif
}


QTEST_APPLESS_MAIN(STLTest)
