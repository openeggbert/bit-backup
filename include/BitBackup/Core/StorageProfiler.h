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

#ifndef STORAGEPROFILER_H
#define STORAGEPROFILER_H

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

namespace BitBackup::Core {

    enum class StorageMedium {
        Rotational,
        SolidState,
        Nvme,
        Unknown
    };

    struct HashingPlan {
        StorageMedium medium;
        unsigned workers;
        bool manualOverride;
    };

    class StorageProfiler {
    public:
        static constexpr unsigned MAX_HASH_WORKERS = 16;
        static constexpr unsigned MAX_AUTO_SSD_WORKERS = 4;
        static constexpr unsigned MAX_AUTO_NVME_WORKERS = 16;

        StorageProfiler() = delete;

        static StorageMedium detect(const std::filesystem::path& path);

        // threadsValue is the value of the optional CLI threads=N argument.
        // A valid value explicitly overrides the storage-aware automatic plan.
        static HashingPlan makePlan(
            StorageMedium medium,
            std::optional<std::string_view> threadsValue,
            unsigned hardwareConcurrency);

        static std::string name(StorageMedium medium);
    };

}

#endif // STORAGEPROFILER_H
