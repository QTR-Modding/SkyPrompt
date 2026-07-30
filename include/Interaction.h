#pragma once

namespace ACTIONS {
    using Action = uint32_t;
};

namespace SCENES {
    using Event = uint32_t;
};

namespace InteractionID {
    constexpr uint32_t Pack(const uint16_t a_clientID, const uint16_t a_localID) {
        return (static_cast<uint32_t>(a_clientID) << 16) | a_localID;
    }

    constexpr uint16_t Local(const uint32_t a_id) {
        return static_cast<uint16_t>(a_id);
    }

    constexpr uint16_t Client(const uint32_t a_id) { return static_cast<uint16_t>(a_id >> 16); }
}

struct Interaction {
    ACTIONS::Action action = 0;
    SCENES::Event event = 0;

    Interaction() = default;

    Interaction(const Interaction& a_rhs) {
        action = a_rhs.action;
        event = a_rhs.event;
    }

    Interaction(const SCENES::Event& a_event, const ACTIONS::Action& a_action) : action(a_action), event(a_event) {
    }

    bool operator<(const Interaction& a_rhs) const {
        return event == a_rhs.event ? action < a_rhs.action : event < a_rhs.event;
    };
    bool operator==(const Interaction& a_rhs) const { return action == a_rhs.action && event == a_rhs.event; }
};