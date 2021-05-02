#pragma once

#include <functional>
#include <vector>
#include <memory>

namespace Utilities {

template <auto event, typename... Args>
class EventHandler {
    public:
    typedef std::function<void(decltype(event), Args...)> ListenerFunction;

    struct Connector {
        friend class EventHandler;
        ListenerFunction handler;
    };

    std::shared_ptr<Connector> connect(ListenerFunction listener) {
        auto connector = std::shared_ptr<Connector>(new Connector{listener});
        connectors.push_back(connector);

        return connector;
    }

    void send(Args... args) {
        auto it = connectors.begin();
        while (it != connectors.end()) {
            auto connector = it->lock();
            if (connector) {
                connector->handler(event, args...);
                ++it;
            } else {
                it = connectors.erase(it);
            }
        }
    }

    private:
    std::vector<std::weak_ptr<Connector>> connectors;
};

}