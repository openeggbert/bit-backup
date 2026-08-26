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

#include "BitBackup/Core/StorageProfiler.h"

#include <algorithm>
#include <cctype>
#include <charconv>
#include <fstream>
#include <unordered_set>

#ifdef __linux__
#include <sys/stat.h>
#include <sys/sysmacros.h>
#endif

namespace BitBackup::Core {

    namespace {
#ifdef __linux__
        bool hasNvmePathComponent(const std::filesystem::path& path) {
            std::error_code error;
            const std::filesystem::path canonical =
                std::filesystem::weakly_canonical(path, error);
            const std::filesystem::path& inspected = error ? path : canonical;

            for (const auto& component : inspected) {
                const std::string name = component.string();
                if (name.size() > 4 && name.starts_with("nvme") &&
                    std::isdigit(static_cast<unsigned char>(name[4]))) {
                    return true;
                }
            }
            return false;
        }

        StorageMedium detectBlockDevice(
            const std::filesystem::path& sysDevice,
            std::unordered_set<std::string>& visited) {

            std::error_code canonicalError;
            const std::filesystem::path canonical =
                std::filesystem::weakly_canonical(sysDevice, canonicalError);
            const std::string key =
                (canonicalError ? sysDevice : canonical).generic_string();
            if (!visited.insert(key).second) {
                return StorageMedium::Unknown;
            }

            int rotationalValue = -1;
            std::ifstream rotational(sysDevice / "queue/rotational");
            (void) (rotational >> rotationalValue);

            bool childIsRotational = false;
            bool childIsNvme = false;
            bool childIsSolidState = false;
            std::error_code iteratorError;
            std::filesystem::directory_iterator it(
                sysDevice / "slaves",
                std::filesystem::directory_options::skip_permission_denied,
                iteratorError);
            const std::filesystem::directory_iterator end;
            while (!iteratorError && it != end) {
                const StorageMedium child = detectBlockDevice(it->path(), visited);
                childIsRotational |= child == StorageMedium::Rotational;
                childIsNvme |= child == StorageMedium::Nvme;
                childIsSolidState |= child == StorageMedium::SolidState;
                it.increment(iteratorError);
            }

            // A mixed stack containing any rotational device must retain the
            // conservative HDD profile. Otherwise preserve NVMe information
            // through dm-crypt, LVM, md and similar virtual block layers.
            if (rotationalValue == 1 || childIsRotational) {
                return StorageMedium::Rotational;
            }
            if (hasNvmePathComponent(sysDevice) || childIsNvme) {
                return StorageMedium::Nvme;
            }
            if (rotationalValue == 0 || childIsSolidState) {
                return StorageMedium::SolidState;
            }
            return StorageMedium::Unknown;
        }
#endif
    }

    StorageMedium StorageProfiler::detect(const std::filesystem::path& path) {
#ifdef __linux__
        struct stat pathStat {};
        if (::stat(path.c_str(), &pathStat) != 0) {
            return StorageMedium::Unknown;
        }

        const std::filesystem::path sysDevice =
            std::filesystem::path("/sys/dev/block") /
            (std::to_string(major(pathStat.st_dev)) + ":" +
             std::to_string(minor(pathStat.st_dev)));
        std::unordered_set<std::string> visited;
        return detectBlockDevice(sysDevice, visited);
#else
        (void) path;
#endif
        return StorageMedium::Unknown;
    }

    HashingPlan StorageProfiler::makePlan(
        StorageMedium medium,
        std::optional<std::string_view> threadsValue,
        unsigned hardwareConcurrency) {

        if (threadsValue.has_value()) {
            unsigned requested = 0;
            const std::string_view value = *threadsValue;
            const auto [end, error] = std::from_chars(
                value.data(), value.data() + value.size(), requested);
            if (error == std::errc{} && end == value.data() + value.size() && requested >= 1) {
                return {
                    medium,
                    std::min(requested, MAX_HASH_WORKERS),
                    true
                };
            }
        }

        const unsigned availableCpus = hardwareConcurrency == 0 ? 4 : hardwareConcurrency;
        unsigned workers = 1;
        if (medium == StorageMedium::SolidState) {
            workers = std::min(availableCpus, MAX_AUTO_SSD_WORKERS);
        } else if (medium == StorageMedium::Nvme) {
            workers = std::min(availableCpus, MAX_AUTO_NVME_WORKERS);
        }
        return {medium, workers, false};
    }

    std::string StorageProfiler::name(StorageMedium medium) {
        switch (medium) {
            case StorageMedium::Rotational: return "rotational-hdd";
            case StorageMedium::SolidState: return "solid-state";
            case StorageMedium::Nvme: return "nvme-ssd";
            case StorageMedium::Unknown: return "unknown";
        }
        return "unknown";
    }

}
