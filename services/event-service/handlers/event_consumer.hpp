#pragma once

#include <userver/components/component_base.hpp>
#include <userver/kafka/consumer_component.hpp>
#include <userver/kafka/consumer_scope.hpp>

namespace taxi::events {

class EventConsumer final : public userver::components::ComponentBase {
public:
    static constexpr std::string_view kName = "event-consumer";

    EventConsumer(const userver::components::ComponentConfig& config,
                  const userver::components::ComponentContext& context);
    ~EventConsumer() override;

private:
    void Consume(userver::kafka::MessageBatchView messages);

    userver::kafka::ConsumerScope consumer_;
};

}  // namespace taxi::events
