#include <thread>

#include "external/envoy/source/common/common/random_generator.h"
#include "external/envoy/source/common/runtime/runtime_impl.h"
#include "external/envoy/source/common/stats/isolated_store_impl.h"
#include "external/envoy/test/mocks/local_info/mocks.h"
#include "external/envoy/test/mocks/protobuf/mocks.h"
#include "external/envoy/test/mocks/thread_local/mocks.h"

#include "source/common/worker_impl.h"

#include "gtest/gtest.h"

using namespace testing;

namespace Nighthawk {

class TestWorker : public WorkerImpl {
public:
  TestWorker(Envoy::Api::Api& api, Envoy::ThreadLocal::Instance& tls)
      : WorkerImpl(api, tls, test_store_), thread_id_(std::this_thread::get_id()) {}
  void work() override {
    EXPECT_NE(thread_id_, std::this_thread::get_id());
    ran_ = true;
  }
  void shutdownThread() override {}

  bool ran_{};
  Envoy::Stats::IsolatedStoreImpl test_store_;
  std::thread::id thread_id_;
};

class WorkerTest : public Test {
public:
  WorkerTest() : api_(Envoy::Api::createApiForTest()) {}

  Envoy::Api::ApiPtr api_;
  Envoy::ThreadLocal::MockInstance tls_;
  Envoy::Stats::IsolatedStoreImpl test_store_;
  Envoy::Random::RandomGeneratorImpl rand_;
  NiceMock<Envoy::LocalInfo::MockLocalInfo> local_info_;
  NiceMock<Envoy::ProtobufMessage::MockValidationVisitor> validation_visitor_;
};

TEST_F(WorkerTest, WorkerExecutesOnThread) {
  InSequence in_sequence;

  EXPECT_CALL(tls_, registerThread(_, false));
  EXPECT_CALL(tls_, allocateSlot());

  TestWorker worker(*api_, tls_);
  NiceMock<Envoy::Event::MockDispatcher> dispatcher;
  Envoy::Runtime::LoaderPtr loader = Envoy::Runtime::LoaderPtr{new Envoy::Runtime::LoaderImpl(
      dispatcher, tls_, {}, local_info_, test_store_, rand_, validation_visitor_, *api_)};
  worker.start();
  worker.waitForCompletion();

  EXPECT_CALL(tls_, shutdownThread());
  ASSERT_TRUE(worker.ran_);
  worker.shutdown();
}

} // namespace Nighthawk
