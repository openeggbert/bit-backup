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

#include "BitBackup/Core/StorageProfiler.h"

using BitBackup::Core::StorageMedium;
using BitBackup::Core::StorageProfiler;

TEST(StorageProfilerTest, RotationalStorageUsesOneWorkerAutomatically) {
    const auto plan = StorageProfiler::makePlan(StorageMedium::Rotational, std::nullopt, 32);

    EXPECT_EQ(plan.workers, 1u);
    EXPECT_FALSE(plan.manualOverride);
}

TEST(StorageProfilerTest, UnknownStorageUsesConservativeOneWorker) {
    const auto plan = StorageProfiler::makePlan(StorageMedium::Unknown, std::nullopt, 32);

    EXPECT_EQ(plan.workers, 1u);
    EXPECT_FALSE(plan.manualOverride);
}

TEST(StorageProfilerTest, SolidStateStorageUsesAtMostFourWorkersAutomatically) {
    EXPECT_EQ(StorageProfiler::makePlan(StorageMedium::SolidState, std::nullopt, 2).workers, 2u);
    EXPECT_EQ(StorageProfiler::makePlan(StorageMedium::SolidState, std::nullopt, 32).workers, 4u);
    EXPECT_EQ(StorageProfiler::makePlan(StorageMedium::SolidState, std::nullopt, 0).workers, 4u);
}

TEST(StorageProfilerTest, NvmeStorageUsesAtMostSixteenWorkersAutomatically) {
    EXPECT_EQ(StorageProfiler::makePlan(StorageMedium::Nvme, std::nullopt, 8).workers, 8u);
    EXPECT_EQ(StorageProfiler::makePlan(StorageMedium::Nvme, std::nullopt, 32).workers, 16u);
    EXPECT_EQ(StorageProfiler::makePlan(StorageMedium::Nvme, std::nullopt, 0).workers, 4u);
}

TEST(StorageProfilerTest, ValidManualValueOverridesAutomaticPlan) {
    const auto plan = StorageProfiler::makePlan(StorageMedium::Rotational, "3", 32);

    EXPECT_EQ(plan.workers, 3u);
    EXPECT_TRUE(plan.manualOverride);
}

TEST(StorageProfilerTest, ManualValueIsCappedAtSixteenWorkers) {
    const auto plan = StorageProfiler::makePlan(StorageMedium::SolidState, "200", 32);

    EXPECT_EQ(plan.workers, StorageProfiler::MAX_HASH_WORKERS);
    EXPECT_TRUE(plan.manualOverride);
}

TEST(StorageProfilerTest, InvalidManualValueFallsBackToAutomaticPlan) {
    EXPECT_EQ(StorageProfiler::makePlan(StorageMedium::Rotational, "0", 32).workers, 1u);
    EXPECT_EQ(StorageProfiler::makePlan(StorageMedium::SolidState, "not-a-number", 32).workers, 4u);
    EXPECT_FALSE(StorageProfiler::makePlan(StorageMedium::SolidState, "4x", 32).manualOverride);
}

TEST(StorageProfilerTest, MissingPathHasUnknownStorageType) {
    EXPECT_EQ(
        StorageProfiler::detect("/definitely/not/a/real/bit-backup/path"),
        StorageMedium::Unknown);
}
