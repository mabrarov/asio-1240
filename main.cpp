#if defined(WIN32)

#include <tchar.h>

#endif

#include <cstdlib>
#include <sstream>
#include <iostream>
#include <thread>
#include <chrono>
#include <boost/asio.hpp>
#include <boost/system/error_code.hpp>

namespace {

namespace chrono = std::chrono;
namespace asio = boost::asio;

template <class Executor = asio::any_io_executor>
struct device
{
  enum state_t
  {
    posting, reading, complete
  };

  Executor executor;

  template <class CompletionToken>
  auto async_read(std::size_t op_id, CompletionToken&& token)
  {
    return asio::async_compose<CompletionToken,
        void(std::size_t, boost::system::error_code)>(
        [this, state = posting, op_id](auto& self,
            boost::system::error_code ec = {}) mutable
        {
          if (ec)
          {
            state = complete;
          }

          if (posting == state)
          {
            {
              std::ostringstream buffer;
              buffer << op_id << ": Posting thread ID: "
                  << std::this_thread::get_id() << '\n';
              std::cout << buffer.str() << std::flush;
            }
            state = reading;
            asio::post(executor, std::move(self));
            return;
          }

          if (reading == state)
          {
            state = complete;
            {
              std::ostringstream buffer;
              buffer << op_id << ": Reading thread ID: "
                  << std::this_thread::get_id() << '\n';
              std::cout << buffer.str() << std::flush;
            }
            // Do some crazy operation
            const auto start = chrono::high_resolution_clock::now();
            while (chrono::high_resolution_clock::now() - start
                < chrono::seconds(2))
            {
              std::this_thread::sleep_for(chrono::microseconds(100));
            }
            {
              std::ostringstream buffer;
              buffer << op_id << ": Reading completed\n";
              std::cout << buffer.str() << std::flush;
            }
          }

          if (complete == state)
          {
            self.complete(op_id, ec);
          }
        }, token, executor);
  }
};

template <class Executor = asio::any_io_executor>
struct processing_state
{
  Executor executor;
  device<Executor> dev;
  std::size_t op_id;

  void start()
  {
    next_read();
  }

  void next_read()
  {
    dev.async_read(op_id++,
        [this](std::size_t completed_op_id, boost::system::error_code ec)
        {
          asio::defer(executor, [this, completed_op_id, ec]()
          {
            if (ec)
            {
              std::ostringstream buffer;
              buffer << completed_op_id << ": Error: " << ec.message() << '\n';
              std::cout << buffer.str() << std::flush;
              return;
            }

            // Launch next async op and come back on this executor
            next_read();

            // Do some work on read stuff
            {
              std::ostringstream buffer;
              buffer << completed_op_id << ": Processing thread ID: "
                  << std::this_thread::get_id() << '\n';
              std::cout << buffer.str() << std::flush;
            }
            const auto start = chrono::high_resolution_clock::now();
            while (chrono::high_resolution_clock::now() - start
                < chrono::seconds(2))
            {
              std::this_thread::sleep_for(chrono::microseconds(100));
            }
            {
              std::ostringstream buffer;
              buffer << completed_op_id << ": Processing completed\n";
              std::cout << buffer.str() << std::flush;
            }
          });
        });
  }
};

}

#if defined(WIN32) && !defined(__MINGW32__)

int _tmain(int argc, _TCHAR* argv[])
#else
int main(int argc, char* argv[])
#endif
{
  using strand = asio::strand<asio::io_context::executor_type>;
  const std::size_t thread_count = 2;
  asio::io_context ctx(thread_count);
  std::vector<std::thread> pool;

  processing_state<strand>
      state{asio::make_strand(ctx), {asio::make_strand(ctx)}, 0};
  state.start();

  for (std::size_t i = 0; i < thread_count - 1; ++i)
  {
    pool.emplace_back([&ctx]
    {
      ctx.run();
    });
  }
  ctx.run();

  for (auto& t: pool)
  {
    t.join();
  }

  return EXIT_SUCCESS;
}
