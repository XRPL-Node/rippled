#include <xrpld/app/main/BasicApp.h>

#include <xrpl/beast/core/CurrentThreadName.h>

#include <boost/asio/executor_work_guard.hpp>

BasicApp::BasicApp(std::size_t numberOfThreads) : numberOfThreads_(numberOfThreads)
{
    work_.emplace(boost::asio::make_work_guard(io_context_));
}

void
BasicApp::startIOThreads()
{
    threads_.reserve(numberOfThreads_);

    while (numberOfThreads_--)
    {
        threads_.emplace_back([this, n = numberOfThreads_]() {
            beast::setCurrentThreadName("io svc #" + std::to_string(n));
            this->io_context_.run();
        });
    }
}

BasicApp::~BasicApp()
{
    work_.reset();

    for (auto& t : threads_)
        t.join();
}
