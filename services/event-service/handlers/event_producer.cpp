#include "handlers/event_producer.hpp"

#include <chrono>
#include <string>

#include <userver/components/component_config.hpp>
#include <userver/components/component_context.hpp>
#include <userver/formats/json/serialize.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/server/handlers/exceptions.hpp>
#include <userver/server/http/http_status.hpp>
#include <userver/utils/datetime.hpp>

namespace taxi::events {

namespace {

userver::formats::json::Value ParseJsonOrThrow(const std::string& body) {
    if (body.empty()) {
        return userver::formats::json::ValueBuilder().ExtractValue();
    }

    try {
        return userver::formats::json::FromString(body);
    } catch (const std::exception& e) {
        throw userver::server::handlers::ClientError(
            userver::server::handlers::ExternalBody{
                std::string{"invalid JSON: "} + e.what()});
    }
}

std::string NowIso() {
    return userver::utils::datetime::Timestring(
        userver::utils::datetime::Now());
}

}  // namespace

EventProducer::EventProducer(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& context)
    : HttpHandlerBase(config, context),
      producer_(context.FindComponent<userver::kafka::ProducerComponent>(
                    "kafka-producer")
                    .GetProducer()),
      topic_("taxi.events") {}

std::string EventProducer::HandleRequestThrow(
    const userver::server::http::HttpRequest& request,
    userver::server::request::RequestContext&) const {
    auto& response = request.GetHttpResponse();
    response.SetContentType("application/json");

    const auto body = ParseJsonOrThrow(request.RequestBody());
    const auto seq = sequence_.fetch_add(1, std::memory_order_relaxed) + 1;

    const auto event_type =
        body["event_type"].As<std::string>("RideCreated");
    const auto aggregate_type =
        body["aggregate_type"].As<std::string>("ride");
    const auto aggregate_id =
        body["aggregate_id"].As<std::string>("ride-demo");

    userver::formats::json::ValueBuilder payload;
    payload["source"] = body["source"].As<std::string>("lab6-demo");
    payload["note"] = body["note"].As<std::string>(
        "demo event published by userver Kafka producer");

    userver::formats::json::ValueBuilder event;
    event["event_id"] = "evt-" + std::to_string(seq);
    event["event_type"] = event_type;
    event["event_version"] = 1;
    event["occurred_at"] = NowIso();
    event["producer"] = "event-service";
    event["aggregate_type"] = aggregate_type;
    event["aggregate_id"] = aggregate_id;
    event["payload"] = payload.ExtractValue();

    const auto message = userver::formats::json::ToString(event.ExtractValue());
    producer_.Send(topic_, aggregate_id, message);

    response.SetStatus(userver::server::http::HttpStatus::kAccepted);

    userver::formats::json::ValueBuilder out;
    out["status"] = "published";
    out["topic"] = topic_;
    out["key"] = aggregate_id;
    out["event_type"] = event_type;
    out["event_id"] = "evt-" + std::to_string(seq);
    return userver::formats::json::ToString(out.ExtractValue());
}

}  // namespace taxi::events
