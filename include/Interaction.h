#pragma once

namespace ACTIONS {
	using Action = uint32_t;
};

namespace SCENES {
	using Event = uint32_t;
};


struct Interaction {

    ACTIONS::Action action = 0;
    SCENES::Event event = 0;

    Interaction() = default;
    Interaction(const Interaction& a_rhs) {
	    action = a_rhs.action;
	    event = a_rhs.event;
    }
    Interaction(const SCENES::Event& a_event, const ACTIONS::Action& a_action) : action(a_action), event(a_event){}

    bool operator<(const Interaction& a_rhs) const {return event == a_rhs.event ? action < a_rhs.action : event < a_rhs.event;};
    bool operator==(const Interaction& a_rhs) const {return action == a_rhs.action && event == a_rhs.event;}
};

