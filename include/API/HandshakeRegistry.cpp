#include "HandshakeRegistry.h"

bool Handshake::Registry::RequestHandshake(const ClientID a_clientID, const HandshakeKey a_key,
                                            const HandshakeKey a_otherKey) {
    if (a_clientID == 0) {
        return false;
    }

    std::lock_guard lock(mutex_);
    const Request request{.key = a_key, .other_key = a_otherKey};
    auto& client_requests = requests_[a_clientID];
    if (std::ranges::find(client_requests, request) != client_requests.end()) {
        return true;
    }
    client_requests.push_back(request);

    for (const auto& [other_client_id, other_requests] : requests_) {
        if (other_client_id == a_clientID) {
            continue;
        }
        if (std::ranges::any_of(other_requests, [&](const Request& a_otherRequest) {
                return IsReciprocal(request, a_otherRequest);
            })) {
            compatible_[a_clientID].insert(other_client_id);
            compatible_[other_client_id].insert(a_clientID);
        }
    }

    return true;
}

bool Handshake::Registry::AreCompatible(const ClientID a_firstClientID,
                                        const ClientID a_secondClientID) const {
    if (a_firstClientID == a_secondClientID) {
        return true;
    }

    std::lock_guard lock(mutex_);
    const auto matches = compatible_.find(a_firstClientID);
    return matches != compatible_.end() && matches->second.contains(a_secondClientID);
}

bool Handshake::Registry::IsReciprocal(const Request& a_first, const Request& a_second) {
    return a_first.key == a_second.other_key && a_first.other_key == a_second.key;
}
