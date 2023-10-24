
#include <glog/logging.h>
#include <gflags/gflags.h>

#include "http_server.h"

DEFINE_string(host, "0.0.0.0", "");
DEFINE_int32(port, 10080, "");
DEFINE_int32(threads, 1, "");
DEFINE_string(website, "./examples/website", "");

int main(int argc, char **argv) {
  FLAGS_logtostderr = 1;
  FLAGS_colorlogtostderr = 1;
  gflags::ParseCommandLineFlags(&argc, &argv, true);
  google::InitGoogleLogging(argv[0]);

  LOG(WARNING) << "Hello guys!";
  
  porter::HttpServer http_server(FLAGS_host, FLAGS_port, FLAGS_threads, FLAGS_website);
  http_server.Run();
  
  return 0;
}
