#include <userver/components/minimal_server_component_list.hpp>
#include <userver/kafka/consumer_component.hpp>
#include <userver/kafka/producer_component.hpp>
#include <userver/storages/secdist/component.hpp>
#include <userver/storages/secdist/provider_component.hpp>
#include <userver/testsuite/testsuite_support.hpp>
#include <userver/utils/daemon_run.hpp>

#include "handlers/event_consumer.hpp"
#include "handlers/event_producer.hpp"

int main(int argc, char* argv[]) {
    auto component_list = userver::components::MinimalServerComponentList()
        .Append<userver::components::TestsuiteSupport>()
        .Append<userver::components::DefaultSecdistProvider>()
        .Append<userver::components::Secdist>()
        .Append<userver::kafka::ProducerComponent>("kafka-producer")
        .Append<userver::kafka::ConsumerComponent>("kafka-consumer")
        .Append<taxi::events::EventConsumer>()
        .Append<taxi::events::EventProducer>();

    return userver::utils::DaemonMain(argc, argv, component_list);
}
