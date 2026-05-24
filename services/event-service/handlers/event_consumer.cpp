#include "handlers/event_consumer.hpp"

#include <userver/components/component_context.hpp>
#include <userver/formats/json/serialize.hpp>
#include <userver/logging/log.hpp>

namespace taxi::events {

EventConsumer::EventConsumer(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& context)
    : ComponentBase(config, context),
      consumer_(context.FindComponent<userver::kafka::ConsumerComponent>(
                    "kafka-consumer")
                    .GetConsumer()) {
    consumer_.Start([this](userver::kafka::MessageBatchView messages) {
        Consume(messages);
        consumer_.AsyncCommit();
    });
}

EventConsumer::~EventConsumer() { consumer_.Stop(); }

void EventConsumer::Consume(userver::kafka::MessageBatchView messages) {
    for (const auto& message : messages) {
        LOG_INFO() << "taxi event consumed"
                   << " topic=" << message.GetTopic()
                   << " partition=" << message.GetPartition()
                   << " offset=" << message.GetOffset()
                   << " key=" << message.GetKey()
                   << " payload=" << message.GetPayload();
    }
}

}  // namespace taxi::events
