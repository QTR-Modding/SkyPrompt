#pragma once

#include "SkyPrompt/API.hpp"
#include <unordered_set>

namespace Handshake {
    using ClientID = SkyPromptAPI::ClientID;
    using HandshakeKey = SkyPromptAPI::HandshakeKey;

    class Registry {
    public:
        bool RequestHandshake(ClientID a_clientID, HandshakeKey a_key, HandshakeKey a_otherKey);
        [[nodiscard]] bool AreCompatible(ClientID a_firstClientID, ClientID a_secondClientID) const;

    private:
        struct Request {
            HandshakeKey key;
            HandshakeKey other_key;

            bool operator==(const Request&) const = default;
        };

        [[nodiscard]] static bool IsReciprocal(const Request& a_first, const Request& a_second);

        mutable std::mutex mutex_;
        std::unordered_map<ClientID, std::vector<Request>> requests_;
        std::unordered_map<ClientID, std::unordered_set<ClientID>> compatible_;
    };
}
