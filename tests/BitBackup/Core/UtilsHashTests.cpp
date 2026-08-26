/*
 * MIT License
 * Copyright (c) 2023-2026 Robert Vokac
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <gtest/gtest.h>

#include "BitBackup/Core/Utils.h"

#include <filesystem>
#include <fstream>

namespace {

    class TemporaryHashFile {
    public:
        TemporaryHashFile() {
            path = std::filesystem::temp_directory_path() /
                "bit-backup-utils-hash-test.txt";
            std::ofstream(path, std::ios::binary) << "abc";
        }

        ~TemporaryHashFile() {
            std::error_code error;
            std::filesystem::remove(path, error);
        }

        std::filesystem::path path;
    };

}

TEST(UtilsHashTest, SequentialReaderKeepsSha512Result) {
    const TemporaryHashFile file;

    EXPECT_EQ(
        BitBackup::Core::Utils::calculateSHA512Hash(file.path),
        "ddaf35a193617abacc417349ae204131"
        "12e6fa4e89a97ea20a9eeee64b55d39a"
        "2192992a274fc1a836ba3c23a3feebbd"
        "454d4423643ce80e2a9ac94fa54ca49f");
}

TEST(UtilsHashTest, SequentialReaderKeepsSha256Result) {
    const TemporaryHashFile file;

    EXPECT_EQ(
        BitBackup::Core::Utils::calculateSHA256Hash(file.path),
        "ba7816bf8f01cfea414140de5dae2223"
        "b00361a396177a9cb410ff61f20015ad");
}
