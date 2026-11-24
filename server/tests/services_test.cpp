// The gRPC layer, exercised over an in-process channel.

#include "services.h"

#include <memory>

#include <grpcpp/grpcpp.h>
#include <gtest/gtest.h>

#include "test_spots.h"

namespace gto::server {
namespace {

using testing::quick_spot;

class ServicesTest : public ::testing::Test {
 protected:
  explicit ServicesTest(std::string token = "")
      : auth_(std::move(token)),
        jobs_(make_config()),
        server_service_(auth_, "test"),
        tree_service_(auth_, jobs_),
        job_service_(auth_, jobs_),
        solution_service_(auth_, jobs_) {
    grpc::ServerBuilder builder;
    builder.RegisterService(&server_service_);
    builder.RegisterService(&tree_service_);
    builder.RegisterService(&job_service_);
    builder.RegisterService(&solution_service_);
    server_ = builder.BuildAndStart();
    channel_ = server_->InProcessChannel(grpc::ChannelArguments());
  }

  ~ServicesTest() override {
    jobs_.shutdown();
    server_->Shutdown(std::chrono::system_clock::now() + std::chrono::seconds(1));
  }

  static JobManagerConfig make_config() {
    JobManagerConfig config;
    config.threads = 2;
    return config;
  }

  /// A call context carrying `token`, or none if it is empty.
  static std::unique_ptr<grpc::ClientContext> context(const std::string& token = "") {
    auto result = std::make_unique<grpc::ClientContext>();
    result->set_deadline(std::chrono::system_clock::now() + std::chrono::seconds(30));
    if (!token.empty()) result->AddMetadata("authorization", "Bearer " + token);
    return result;
  }

  Auth auth_;
  JobManager jobs_;
  ServerServiceImpl server_service_;
  TreeServiceImpl tree_service_;
  JobServiceImpl job_service_;
  SolutionServiceImpl solution_service_;
  std::unique_ptr<grpc::Server> server_;
  std::shared_ptr<grpc::Channel> channel_;
};

TEST_F(ServicesTest, SolvesAndBrowsesOverRpc) {
  auto jobs = v1::JobService::NewStub(channel_);
  auto trees = v1::TreeService::NewStub(channel_);
  auto solutions = v1::SolutionService::NewStub(channel_);

  v1::EstimateTreeRequest estimate_request;
  *estimate_request.mutable_spot() = quick_spot();
  v1::EstimateTreeResponse estimate;
  ASSERT_TRUE(trees->EstimateTree(context().get(), estimate_request, &estimate).ok());
  EXPECT_EQ(estimate.nodes(), 6u);
  EXPECT_TRUE(estimate.can_solve());

  v1::SubmitJobRequest submit;
  *submit.mutable_spot() = quick_spot();
  submit.set_name("river toy");
  submit.mutable_settings()->set_max_iterations(1000);
  submit.mutable_settings()->set_target_exploitability_pct(0);
  v1::Job job;
  ASSERT_TRUE(jobs->Submit(context().get(), submit, &job).ok());
  EXPECT_EQ(job.name(), "river toy");

  // Watch delivers the job until it is done, then ends by itself.
  v1::WatchJobRequest watch;
  watch.set_id(job.id());
  auto watch_context = context();
  auto stream = jobs->Watch(watch_context.get(), watch);
  v1::Job update, last;
  int updates = 0;
  while (stream->Read(&update)) {
    EXPECT_GE(update.iteration(), last.iteration());
    last = update;
    ++updates;
  }
  EXPECT_TRUE(stream->Finish().ok());
  EXPECT_GT(updates, 0);
  EXPECT_EQ(last.state(), v1::JOB_STATE_DONE);
  EXPECT_EQ(last.iteration(), 1000u);
  EXPECT_NEAR(last.oop_ev(), 70.0, 0.2);

  v1::ListJobsResponse list;
  ASSERT_TRUE(jobs->List(context().get(), v1::ListJobsRequest(), &list).ok());
  ASSERT_EQ(list.jobs_size(), 1);
  EXPECT_EQ(list.jobs(0).id(), job.id());

  v1::GetNodeRequest node_request;
  node_request.set_job_id(job.id());
  node_request.set_path("check");
  v1::GetNodeResponse node;
  ASSERT_TRUE(solutions->GetNode(context().get(), node_request, &node).ok());
  EXPECT_EQ(node.path(), "check");
  ASSERT_EQ(node.actions_size(), 2);
  EXPECT_NEAR(node.actions(1).frequency(), 0.3, 0.01);

  v1::DeleteJobRequest remove;
  remove.set_id(job.id());
  v1::DeleteJobResponse removed;
  ASSERT_TRUE(jobs->Delete(context().get(), remove, &removed).ok());
  ASSERT_TRUE(jobs->List(context().get(), v1::ListJobsRequest(), &list).ok());
  EXPECT_EQ(list.jobs_size(), 0);
}

TEST_F(ServicesTest, MapsErrorsToStatusCodes) {
  auto jobs = v1::JobService::NewStub(channel_);
  auto solutions = v1::SolutionService::NewStub(channel_);

  v1::GetJobRequest get;
  get.set_id("nope");
  v1::Job job;
  EXPECT_EQ(jobs->Get(context().get(), get, &job).error_code(), grpc::StatusCode::NOT_FOUND);

  v1::SubmitJobRequest bad;
  *bad.mutable_spot() = quick_spot();
  bad.mutable_spot()->set_oop_range("banana");
  const grpc::Status status = jobs->Submit(context().get(), bad, &job);
  EXPECT_EQ(status.error_code(), grpc::StatusCode::INVALID_ARGUMENT);
  EXPECT_NE(status.error_message().find("oop_range"), std::string::npos);

  v1::WatchJobRequest watch;
  watch.set_id("nope");
  auto watch_context = context();
  auto stream = jobs->Watch(watch_context.get(), watch);
  v1::Job update;
  EXPECT_FALSE(stream->Read(&update));
  EXPECT_EQ(stream->Finish().error_code(), grpc::StatusCode::NOT_FOUND);

  v1::GetNodeRequest node_request;
  node_request.set_job_id("nope");
  v1::GetNodeResponse node;
  EXPECT_EQ(solutions->GetNode(context().get(), node_request, &node).error_code(),
            grpc::StatusCode::NOT_FOUND);
}

TEST_F(ServicesTest, NoTokenNeededWhenNoneIsSet) {
  auto server = v1::ServerService::NewStub(channel_);
  v1::GetInfoResponse info;
  ASSERT_TRUE(server->GetInfo(context().get(), v1::GetInfoRequest(), &info).ok());
  EXPECT_EQ(info.api_major(), kApiMajor);
  EXPECT_EQ(info.server_version(), "test");
  EXPECT_GT(info.logical_cores(), 0u);
}

class AuthenticatedServicesTest : public ServicesTest {
 protected:
  AuthenticatedServicesTest() : ServicesTest("s3cret-token") {}
};

TEST_F(AuthenticatedServicesTest, RejectsCallsWithoutTheToken) {
  auto server = v1::ServerService::NewStub(channel_);
  auto jobs = v1::JobService::NewStub(channel_);
  auto trees = v1::TreeService::NewStub(channel_);
  auto solutions = v1::SolutionService::NewStub(channel_);
  const auto unauthenticated = grpc::StatusCode::UNAUTHENTICATED;

  // Every service, with no token, a wrong one, a prefix of the right one,
  // and the right one in the wrong scheme.
  for (const std::string token : {"", "wrong", "s3cret", "s3cret-token-and-more"}) {
    SCOPED_TRACE("token '" + token + "'");
    v1::GetInfoResponse info;
    EXPECT_EQ(server->GetInfo(context(token).get(), v1::GetInfoRequest(), &info).error_code(),
              unauthenticated);
    v1::PingResponse pong;
    EXPECT_EQ(server->Ping(context(token).get(), v1::PingRequest(), &pong).error_code(),
              unauthenticated);

    v1::ListJobsResponse list;
    EXPECT_EQ(jobs->List(context(token).get(), v1::ListJobsRequest(), &list).error_code(),
              unauthenticated);
    v1::SubmitJobRequest submit;
    *submit.mutable_spot() = quick_spot();
    v1::Job job;
    EXPECT_EQ(jobs->Submit(context(token).get(), submit, &job).error_code(), unauthenticated);
    EXPECT_EQ(jobs->Get(context(token).get(), v1::GetJobRequest(), &job).error_code(),
              unauthenticated);
    EXPECT_EQ(jobs->Pause(context(token).get(), v1::PauseJobRequest(), &job).error_code(),
              unauthenticated);
    EXPECT_EQ(jobs->Resume(context(token).get(), v1::ResumeJobRequest(), &job).error_code(),
              unauthenticated);
    EXPECT_EQ(jobs->Cancel(context(token).get(), v1::CancelJobRequest(), &job).error_code(),
              unauthenticated);
    v1::DeleteJobResponse removed;
    EXPECT_EQ(jobs->Delete(context(token).get(), v1::DeleteJobRequest(), &removed).error_code(),
              unauthenticated);

    auto watch_context = context(token);
    auto stream = jobs->Watch(watch_context.get(), v1::WatchJobRequest());
    v1::Job update;
    EXPECT_FALSE(stream->Read(&update));
    EXPECT_EQ(stream->Finish().error_code(), unauthenticated);

    v1::EstimateTreeResponse estimate;
    EXPECT_EQ(trees->EstimateTree(context(token).get(), v1::EstimateTreeRequest(), &estimate)
                  .error_code(),
              unauthenticated);
    v1::GetNodeResponse node;
    EXPECT_EQ(
        solutions->GetNode(context(token).get(), v1::GetNodeRequest(), &node).error_code(),
        unauthenticated);
  }

  // Right token, wrong scheme.
  grpc::ClientContext basic;
  basic.AddMetadata("authorization", "Basic s3cret-token");
  v1::GetInfoResponse info;
  EXPECT_EQ(server->GetInfo(&basic, v1::GetInfoRequest(), &info).error_code(), unauthenticated);

  // Nothing got through.
  EXPECT_TRUE(jobs_.list().empty());
}

TEST_F(AuthenticatedServicesTest, AcceptsTheRightToken) {
  auto server = v1::ServerService::NewStub(channel_);
  v1::PingRequest ping;
  ping.set_payload("hello");
  v1::PingResponse pong;
  ASSERT_TRUE(server->Ping(context("s3cret-token").get(), ping, &pong).ok());
  EXPECT_EQ(pong.payload(), "hello");

  auto jobs = v1::JobService::NewStub(channel_);
  v1::ListJobsResponse list;
  EXPECT_TRUE(jobs->List(context("s3cret-token").get(), v1::ListJobsRequest(), &list).ok());
}

}  // namespace
}  // namespace gto::server
