#include <gtest/gtest.h>

#include "containers/memory.h"
#include "containers/deque.h"
#include "containers/string.h"
#include "containers/unordered_map.h"
#include "containers/unordered_set.h"
#include "containers/vector.h"
#include "memory/manager.h"
#include "reflection/builtin_properties.h"
#include "reflection/class_registry.h"
#include "reflection/construct.h"

class MemoryManagerTests
: public ::testing::Test
{
protected:
    void SetUp() override
    {
        memory::initialize();
    }

    void TearDown() override
    {
        memory::shutdown();
    }
};

TEST_F(MemoryManagerTests, InitialStats)
{
    auto stats = memory::stats();
    EXPECT_EQ(stats.allocate_calls, 1);    // frame bump is allocated
    EXPECT_EQ(stats.bytes_live, memory::default_bump_size);
    EXPECT_EQ(stats.bytes_peak, memory::default_bump_size);
    EXPECT_EQ(stats.bytes_total_allocated, memory::default_bump_size);
    EXPECT_EQ(stats.deallocate_calls, 0);

    for(std::size_t i = 0; i < stats.bytes_per_tag.size(); ++i)
    {
        if(i == static_cast<std::size_t>(memory::MemoryTag::Bump))
        {
            EXPECT_EQ(stats.bytes_per_tag[i], memory::default_bump_size);
        }
        else
        {
            EXPECT_EQ(stats.bytes_per_tag[i], 0);
        }
    }
}

TEST_F(MemoryManagerTests, Deque)
{
    auto stats = memory::stats();
    EXPECT_EQ(stats.bytes_per_tag[std::to_underlying(memory::MemoryTag::Deque)], 0);

    {
        auto ptr = swr::deque<int>{};
        ptr.push_back(42);

        auto stats = memory::stats();
        EXPECT_GT(stats.allocate_calls, 1);
        EXPECT_EQ(stats.deallocate_calls, 0);

        for(std::size_t i = 0; i < stats.bytes_per_tag.size(); ++i)
        {
            if(i == static_cast<std::size_t>(memory::MemoryTag::Deque))
            {
                EXPECT_GT(stats.bytes_per_tag[i], 0);
            }
            else if(i == static_cast<std::size_t>(memory::MemoryTag::Bump))
            {
                EXPECT_EQ(stats.bytes_per_tag[i], memory::default_bump_size);
            }
            else
            {
                EXPECT_EQ(stats.bytes_per_tag[i], 0);
            }
        }
    }

    stats = memory::stats();
    EXPECT_GT(stats.allocate_calls, 1);
    EXPECT_GT(stats.deallocate_calls, 0);

    EXPECT_EQ(stats.bytes_per_tag[std::to_underlying(memory::MemoryTag::Deque)], 0);
}

TEST_F(MemoryManagerTests, UniquePtr)
{
    auto stats = memory::stats();
    EXPECT_EQ(stats.bytes_per_tag[std::to_underlying(memory::MemoryTag::UniquePtr)], 0);

    {
        auto ptr = swr::make_unique<int>(42);
        ptr.reset();

        auto stats = memory::stats();
        EXPECT_GT(stats.allocate_calls, 1);
        EXPECT_GT(stats.deallocate_calls, 0);

        for(std::size_t i = 0; i < stats.bytes_per_tag.size(); ++i)
        {
            if(i == static_cast<std::size_t>(memory::MemoryTag::Bump))
            {
                EXPECT_EQ(stats.bytes_per_tag[i], memory::default_bump_size);
            }
            else
            {
                EXPECT_EQ(stats.bytes_per_tag[i], 0);
            }
        }
    }

    {
        auto ptr = swr::make_unique<int>(42);

        auto stats = memory::stats();
        EXPECT_GT(stats.allocate_calls, 2);
        EXPECT_GT(stats.deallocate_calls, 0);

        for(std::size_t i = 0; i < stats.bytes_per_tag.size(); ++i)
        {
            if(i == static_cast<std::size_t>(memory::MemoryTag::UniquePtr))
            {
                EXPECT_GT(stats.bytes_per_tag[i], 0);
            }
            else if(i == static_cast<std::size_t>(memory::MemoryTag::Bump))
            {
                EXPECT_EQ(stats.bytes_per_tag[i], memory::default_bump_size);
            }
            else
            {
                EXPECT_EQ(stats.bytes_per_tag[i], 0);
            }
        }
    }

    stats = memory::stats();
    EXPECT_GT(stats.allocate_calls, 2);
    EXPECT_GT(stats.deallocate_calls, 1);

    EXPECT_EQ(stats.bytes_per_tag[std::to_underlying(memory::MemoryTag::UniquePtr)], 0);
}

TEST_F(MemoryManagerTests, String)
{
    auto stats = memory::stats();
    EXPECT_EQ(stats.bytes_per_tag[std::to_underlying(memory::MemoryTag::String)], 0);

    {
        auto ptr = swr::string{"LongStringToTriggerAllocation"};

        auto stats = memory::stats();
        EXPECT_GT(stats.allocate_calls, 1);
        EXPECT_EQ(stats.deallocate_calls, 0);

        for(std::size_t i = 0; i < stats.bytes_per_tag.size(); ++i)
        {
            if(i == static_cast<std::size_t>(memory::MemoryTag::String))
            {
                EXPECT_GT(stats.bytes_per_tag[i], 0);
            }
            else if(i == static_cast<std::size_t>(memory::MemoryTag::Bump))
            {
                EXPECT_EQ(stats.bytes_per_tag[i], memory::default_bump_size);
            }
            else
            {
                EXPECT_EQ(stats.bytes_per_tag[i], 0);
            }
        }
    }

    stats = memory::stats();
    EXPECT_GT(stats.allocate_calls, 1);
    EXPECT_GT(stats.deallocate_calls, 0);

    EXPECT_EQ(stats.bytes_per_tag[std::to_underlying(memory::MemoryTag::String)], 0);
}

TEST_F(MemoryManagerTests, UnorderedMap)
{
    auto stats = memory::stats();
    EXPECT_EQ(stats.bytes_per_tag[std::to_underlying(memory::MemoryTag::UnorderedMap)], 0);

    {
        auto ptr = swr::unordered_map<int, int>{
          {1, 2}};

        auto stats = memory::stats();
        EXPECT_GT(stats.allocate_calls, 1);
        EXPECT_EQ(stats.deallocate_calls, 0);

        for(std::size_t i = 0; i < stats.bytes_per_tag.size(); ++i)
        {
            if(i == static_cast<std::size_t>(memory::MemoryTag::UnorderedMap))
            {
                EXPECT_GT(stats.bytes_per_tag[i], 0);
            }
            else if(i == static_cast<std::size_t>(memory::MemoryTag::Bump))
            {
                EXPECT_EQ(stats.bytes_per_tag[i], memory::default_bump_size);
            }
            else
            {
                EXPECT_EQ(stats.bytes_per_tag[i], 0);
            }
        }
    }

    stats = memory::stats();
    EXPECT_GT(stats.allocate_calls, 1);
    EXPECT_GT(stats.deallocate_calls, 0);

    EXPECT_EQ(stats.bytes_per_tag[std::to_underlying(memory::MemoryTag::UnorderedMap)], 0);
}

TEST_F(MemoryManagerTests, UnorderedSet)
{
    auto stats = memory::stats();
    EXPECT_EQ(stats.bytes_per_tag[std::to_underlying(memory::MemoryTag::UnorderedSet)], 0);

    {
        auto ptr = swr::unordered_set<int>{1, 2};

        auto stats = memory::stats();
        EXPECT_GT(stats.allocate_calls, 1);
        EXPECT_EQ(stats.deallocate_calls, 0);

        for(std::size_t i = 0; i < stats.bytes_per_tag.size(); ++i)
        {
            if(i == static_cast<std::size_t>(memory::MemoryTag::UnorderedSet))
            {
                EXPECT_GT(stats.bytes_per_tag[i], 0);
            }
            else if(i == static_cast<std::size_t>(memory::MemoryTag::Bump))
            {
                EXPECT_EQ(stats.bytes_per_tag[i], memory::default_bump_size);
            }
            else
            {
                EXPECT_EQ(stats.bytes_per_tag[i], 0);
            }
        }
    }

    stats = memory::stats();
    EXPECT_GT(stats.allocate_calls, 1);
    EXPECT_GT(stats.deallocate_calls, 0);

    EXPECT_EQ(stats.bytes_per_tag[std::to_underlying(memory::MemoryTag::UnorderedSet)], 0);
}

TEST_F(MemoryManagerTests, Vector)
{
    auto stats = memory::stats();
    EXPECT_EQ(stats.bytes_per_tag[std::to_underlying(memory::MemoryTag::Vector)], 0);

    {
        auto vec = swr::vector<int>{
          0, 1, 2, 3, 4, 5, 6, 7, 8, 9};

        EXPECT_EQ(vec.size(), 10);
        EXPECT_GE(vec.capacity(), 10);

        auto stats = memory::stats();
        EXPECT_GT(stats.allocate_calls, 1);
        EXPECT_EQ(stats.deallocate_calls, 0);

        for(std::size_t i = 0; i < stats.bytes_per_tag.size(); ++i)
        {
            if(i == static_cast<std::size_t>(memory::MemoryTag::Vector))
            {
                EXPECT_GT(stats.bytes_per_tag[i], 0);
            }
            else if(i == static_cast<std::size_t>(memory::MemoryTag::Bump))
            {
                EXPECT_EQ(stats.bytes_per_tag[i], memory::default_bump_size);
            }
            else
            {
                EXPECT_EQ(stats.bytes_per_tag[i], 0);
            }
        }
    }

    stats = memory::stats();
    EXPECT_GT(stats.allocate_calls, 1);
    EXPECT_GT(stats.deallocate_calls, 0);

    EXPECT_EQ(stats.bytes_per_tag[std::to_underlying(memory::MemoryTag::Vector)], 0);
}

struct PropertyTestStruct
{
    float x{0};
};

TEST_F(MemoryManagerTests, PropertyConstruction)
{
    auto initial_stats = memory::stats();

    {
        auto descriptor = swr::make_unique<
          reflect::PropertyDescriptor>(
          "property_name",
          "Property Label",
          reflect::PropertyFlags::None,
          &reflect::detail::construct_member_erased<&PropertyTestStruct::x>,
          nullptr,
          nullptr,
          nullptr);

        auto stats = memory::stats();

        EXPECT_GT(stats.bytes_live, initial_stats.bytes_live);
        EXPECT_GE(stats.bytes_peak, initial_stats.bytes_peak);
        EXPECT_GT(stats.bytes_total_allocated, initial_stats.bytes_total_allocated);
        EXPECT_GT(stats.allocate_calls, initial_stats.allocate_calls);
        EXPECT_EQ(stats.deallocate_calls, initial_stats.deallocate_calls);
    }

    auto stats = memory::stats();

    EXPECT_EQ(stats.bytes_live, initial_stats.bytes_live);
    EXPECT_GE(stats.bytes_peak, initial_stats.bytes_peak);
    EXPECT_GT(stats.bytes_total_allocated, initial_stats.bytes_total_allocated);
    EXPECT_GT(stats.allocate_calls, initial_stats.allocate_calls);
    EXPECT_GT(stats.deallocate_calls, initial_stats.deallocate_calls);
}

TEST_F(MemoryManagerTests, PropertyInVector)
{
    auto initial_stats = memory::stats();
    swr::vector<
      swr::unique_ptr<
        reflect::Property>>
      properties;

    PropertyTestStruct pts;

    {
        auto descriptor = swr::make_unique<
          reflect::PropertyDescriptor>(
          "property_name",
          "Property Label",
          reflect::PropertyFlags::None,
          &reflect::detail::construct_member_erased<&PropertyTestStruct::x>,
          nullptr,
          nullptr,
          nullptr);

        properties.emplace_back(
          descriptor->construct(
            &pts,
            descriptor->name,
            descriptor->label,
            descriptor->flags,
            descriptor->constraint));

        auto stats = memory::stats();

        EXPECT_GT(stats.bytes_live, initial_stats.bytes_live);
        EXPECT_GE(stats.bytes_peak, initial_stats.bytes_peak);
        EXPECT_GT(stats.bytes_total_allocated, initial_stats.bytes_total_allocated);
        EXPECT_GT(stats.allocate_calls, initial_stats.allocate_calls);
        EXPECT_EQ(stats.deallocate_calls, initial_stats.deallocate_calls);
    }

    properties.clear();
    properties.shrink_to_fit();

    auto stats = memory::stats();

    EXPECT_EQ(stats.bytes_live, initial_stats.bytes_live);
    EXPECT_GE(stats.bytes_peak, initial_stats.bytes_peak);
    EXPECT_GT(stats.bytes_total_allocated, initial_stats.bytes_total_allocated);
    EXPECT_GT(stats.allocate_calls, initial_stats.allocate_calls);
    EXPECT_GT(stats.deallocate_calls, initial_stats.deallocate_calls);
}

class TestRoot : public reflect::ReflectRoot<TestRoot>
{
protected:
    std::size_t* destructor_calls{nullptr};

public:
    virtual ~TestRoot() override
    {
        if(destructor_calls != nullptr)
        {
            ++(*destructor_calls);
        }
    }

    static void register_properties(reflect::ClassInfo& class_info);
    void init(std::size_t* destructor_calls)
    {
        this->destructor_calls = destructor_calls;
    }

    int root_value{42};
    std::string root_name{"root"};
};

DECLARE_REFLECTION(Test, TestRoot);
DEFINE_REFLECTION(TestRoot);

void TestRoot::register_properties(reflect::ClassInfo& class_info)
{
    class_info.register_property<&TestRoot::root_value>(
      "root_value",
      "Root Value",
      reflect::PropertyFlags::ReadOnly);
    class_info.register_property<&TestRoot::root_name>(
      "root_name",
      "Root Name");
}

struct TestChild
: public reflect::Reflected<TestChild, TestRoot>
{
    virtual ~TestChild() override
    {
        if(destructor_calls != nullptr)
        {
            ++(*destructor_calls);
        }
    }

    static void register_properties(reflect::ClassInfo& class_info);

    bool enabled{true};
};

DECLARE_REFLECTION(Test, TestChild);
DEFINE_REFLECTION(TestChild);

void TestChild::register_properties(reflect::ClassInfo& class_info)
{
    class_info.register_property<&TestChild::enabled>(
      "enabled",
      "Enabled");
}

void ensure_reflection_ready()
{
    static bool initialized = false;
    if(initialized)
    {
        return;
    }

    reflect::ReflectionSystem::allow_auto_registration(false);
    reflect::ReflectionSystem::process_pending_registrations();
    initialized = true;
}

TEST_F(MemoryManagerTests, ObjectConstruction)
{
    ensure_reflection_ready();

    // TODO - One failure mode: PropertyDescriptors contains a unique_ptr that is
    //        initialized when calling ensure_reflection_ready and not freed.
    //      - Initial memory is larger than later memory (why?)

    auto initial_stats = memory::stats();

    {
        auto obj = reflect::construct<TestRoot>(
          TestChild::static_class());

        auto stats = memory::stats();

        EXPECT_GT(stats.bytes_live, initial_stats.bytes_live);
        EXPECT_GE(stats.bytes_peak, initial_stats.bytes_peak);
        EXPECT_GT(stats.bytes_total_allocated, initial_stats.bytes_total_allocated);
        EXPECT_GT(stats.allocate_calls, initial_stats.allocate_calls);
        EXPECT_GT(stats.deallocate_calls, initial_stats.deallocate_calls);
    }

    auto stats = memory::stats();

    EXPECT_EQ(stats.bytes_live, initial_stats.bytes_live);
    EXPECT_GE(stats.bytes_peak, initial_stats.bytes_peak);
    EXPECT_GT(stats.bytes_total_allocated, initial_stats.bytes_total_allocated);
    EXPECT_GT(stats.allocate_calls, initial_stats.allocate_calls);
    EXPECT_GT(stats.deallocate_calls, initial_stats.deallocate_calls);
}