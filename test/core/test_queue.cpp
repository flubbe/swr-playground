#include <atomic>
#include <thread>
#include <vector>

#include <gtest/gtest.h>

#include "queue.h"

TEST(ThreadSafeQueue, DefaultConstructedIsEmpty)
{
    ThreadSafeQueue<int> queue;

    EXPECT_TRUE(queue.empty());
    EXPECT_EQ(queue.size(), 0);
}

TEST(ThreadSafeQueue, PushAndPop)
{
    ThreadSafeQueue<int> queue;

    queue.push_back(42);
    queue.push_back(43);

    EXPECT_FALSE(queue.empty());
    EXPECT_EQ(queue.size(), 2);

    int value = 0;

    ASSERT_TRUE(queue.try_pop(value));
    EXPECT_EQ(value, 42);

    ASSERT_TRUE(queue.try_pop(value));
    EXPECT_EQ(value, 43);

    EXPECT_FALSE(queue.try_pop(value));
    EXPECT_TRUE(queue.empty());
}

TEST(ThreadSafeQueue, SupportsMoveAndEmplace)
{
    ThreadSafeQueue<std::string> queue;

    std::string value{"hello"};
    queue.push_back(std::move(value));
    queue.emplace_back("world");

    auto drained = queue.drain();

    ASSERT_EQ(drained.size(), 2);
    EXPECT_EQ(drained[0], "hello");
    EXPECT_EQ(drained[1], "world");
    EXPECT_TRUE(queue.empty());
}

TEST(ThreadSafeQueue, DrainAtomicallyRemovesContents)
{
    ThreadSafeQueue<int> queue;

    queue.push_back(1);
    queue.push_back(2);
    queue.push_back(3);

    auto drained = queue.drain();

    ASSERT_EQ(drained.size(), 3);
    EXPECT_EQ(drained[0], 1);
    EXPECT_EQ(drained[1], 2);
    EXPECT_EQ(drained[2], 3);
    EXPECT_TRUE(queue.empty());

    queue.push_back(4);

    EXPECT_EQ(queue.size(), 1);
    EXPECT_EQ(queue.drain()[0], 4);
}

TEST(ThreadSafeQueue, Clear)
{
    ThreadSafeQueue<int> queue;

    queue.push_back(1);
    queue.push_back(2);
    queue.clear();

    EXPECT_TRUE(queue.empty());
    EXPECT_EQ(queue.size(), 0);
}

TEST(ThreadSafeQueue, ConcurrentProducersAndConsumer)
{
    ThreadSafeQueue<int> queue;

    constexpr int producer_count = 4;
    constexpr int items_per_producer = 10'000;
    constexpr int total_items =
      producer_count * items_per_producer;

    std::atomic<bool> producers_done{false};
    std::atomic<int> consumed{0};

    std::vector<std::thread> producers;

    for(int p = 0; p < producer_count; ++p)
    {
        producers.emplace_back(
          [&queue, p]
          {
              for(int i = 0; i < items_per_producer; ++i)
              {
                  queue.push_back(
                    p * items_per_producer + i);
              }
          });
    }

    std::thread consumer(
      [&]
      {
          int value;

          while(consumed.load() < total_items)
          {
              if(queue.try_pop(value))
              {
                  ++consumed;
              }
              else
              {
                  std::this_thread::yield();
              }
          }
      });

    for(auto& producer: producers)
    {
        producer.join();
    }

    consumer.join();

    EXPECT_EQ(consumed, total_items);
    EXPECT_TRUE(queue.empty());
}

TEST(ThreadSafeQueue, ConcurrentProducersAndDrain)
{
    ThreadSafeQueue<int> queue;

    constexpr int producer_count = 4;
    constexpr int items_per_producer = 25'000;
    constexpr int total_items =
      producer_count * items_per_producer;

    std::atomic<bool> producers_done{false};
    std::atomic<int> drained{0};

    std::vector<std::thread> producers;

    for(int p = 0; p < producer_count; ++p)
    {
        producers.emplace_back(
          [&queue, p]
          {
              for(int i = 0; i < items_per_producer; ++i)
              {
                  queue.emplace_back(
                    p * items_per_producer + i);
              }
          });
    }

    std::thread consumer(
      [&]
      {
          while(!producers_done.load() || !queue.empty())
          {
              auto batch = queue.drain();
              drained += static_cast<int>(batch.size());

              if(batch.empty())
              {
                  std::this_thread::yield();
              }
          }

          // Catch anything enqueued between the final empty()
          // check and the producers terminating.
          drained += static_cast<int>(queue.drain().size());
      });

    for(auto& producer: producers)
    {
        producer.join();
    }

    producers_done = true;
    consumer.join();

    EXPECT_EQ(drained, total_items);
    EXPECT_TRUE(queue.empty());
}
