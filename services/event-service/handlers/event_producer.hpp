#pragma once

#include <atomic>
#include <cstdint>
#include <string>

#include <userver/kafka/producer_component.hpp>
#include <userver/server/handlers/http_handler_base.hpp>

namespace taxi::events {

class EventProducer final : public userver::server::handlers::HttpHandlerBase {
public:
    static constexpr std::string_view kName = "handler-event-produce";

    EventProducer(const userver::components::ComponentConfig& config,
                  const userver::components::ComponentContext& context);

    std::string HandleRequestThrow(
        const userver::server::http::HttpRequest& request,
        userver::server::request::RequestContext& context) const override;

private:
    const userver::kafka::Producer& producer_;
    std::string topic_;
    mutable std::atomic<std::uint64_t> sequence_{0};
};

}  // namespace taxi::events
