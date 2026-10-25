/*
 * Meshflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Rimora Studio
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "ClipboardFilesTests.h"

#include "meshflow/ClipboardFiles.h"

#include <fstream>

namespace fs = std::filesystem;
using meshflow::clipboard::ClipboardFilesError;
using meshflow::clipboard::packFiles;
using meshflow::clipboard::unpackFiles;

namespace {

void writeFile(const fs::path &path, const std::string &content)
{
  fs::create_directories(path.parent_path());
  std::ofstream f(path, std::ios::binary | std::ios::trunc);
  f.write(content.data(), static_cast<std::streamsize>(content.size()));
}

std::string readFile(const fs::path &path)
{
  std::ifstream f(path, std::ios::binary);
  return {std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>()};
}

} // namespace

void ClipboardFilesTests::initTestCase()
{
  m_log.setFilter(LogLevel::Level::Verbose);
}

void ClipboardFilesTests::roundTrip()
{
  const auto base = fs::path(m_dir.path().toStdWString()) / "roundTrip";
  const auto source = base / "source";
  const auto staging = base / "staging";

  writeFile(source / "hello.txt", "hello world");
  const auto packed = packFiles({source / "hello.txt"});
  QVERIFY(!packed.empty());

  const auto staged = unpackFiles(packed, staging);
  QCOMPARE(staged.size(), 1);
  QCOMPARE(readFile(staged.front()), "hello world");
}

void ClipboardFilesTests::directoryTree()
{
  const auto base = fs::path(m_dir.path().toStdWString()) / "tree";
  const auto source = base / "source";
  const auto staging = base / "staging";

  writeFile(source / "folder" / "sub" / "a.txt", "a");
  writeFile(source / "folder" / "b.txt", "bb");

  const auto packed = packFiles({source / "folder"});
  QVERIFY(!packed.empty());

  const auto staged = unpackFiles(packed, staging);
  QCOMPARE(staged.size(), 1);
  QCOMPARE(readFile(staging / "folder" / "sub" / "a.txt"), "a");
  QCOMPARE(readFile(staging / "folder" / "b.txt"), "bb");
}

void ClipboardFilesTests::emptyInput()
{
  QCOMPARE(packFiles({}), std::string{});
}

void ClipboardFilesTests::unsafeNames()
{
  const auto staging = fs::path(m_dir.path().toStdWString()) / "unsafe";

  // count=1, flags=0 (file), name="../evil.txt", size=0
  std::string evil;
  auto u32 = [&evil](uint32_t v) {
    for (int i = 3; i >= 0; --i)
      evil += static_cast<char>((v >> (i * 8)) & 0xff);
  };
  u32(1);
  evil += '\0';
  const std::string name = "../evil.txt";
  u32(static_cast<uint32_t>(name.size()));
  evil += name;
  u32(0);
  u32(0);

  QVERIFY_EXCEPTION_THROWN(unpackFiles(evil, staging), ClipboardFilesError);
  QVERIFY(!fs::exists(staging.parent_path() / "evil.txt"));
}

void ClipboardFilesTests::truncatedData()
{
  const auto base = fs::path(m_dir.path().toStdWString()) / "trunc";
  const auto source = base / "source";

  writeFile(source / "f.txt", "0123456789");
  const auto packed = packFiles({source / "f.txt"});
  QVERIFY(packed.size() > 4);

  const auto staging = base / "staging";
  QVERIFY_EXCEPTION_THROWN(
      unpackFiles(std::string_view(packed).substr(0, packed.size() - 5), staging), ClipboardFilesError
  );
}

void ClipboardFilesTests::sizeLimit()
{
  const auto base = fs::path(m_dir.path().toStdWString()) / "limit";
  const auto source = base / "source";

  writeFile(source / "big.bin", std::string(1024, 'x'));
  QVERIFY_EXCEPTION_THROWN(packFiles({source / "big.bin"}, 512), ClipboardFilesError);
}

QTEST_MAIN(ClipboardFilesTests)
