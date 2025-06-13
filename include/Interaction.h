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
	std::string text;
	uint32_t text_color;

	Interaction() = default;
	Interaction(const Interaction& a_rhs) {
		action = a_rhs.action;
		event = a_rhs.event;
		text = a_rhs.text;
		text_color = a_rhs.text_color;
	}
	Interaction(const SCENES::Event& a_event, const ACTIONS::Action& a_action) : action(a_action), event(a_event),text_color(0xFFFFFFFF){}

	[[nodiscard]] std::string name() const { return text; }

    bool operator<(const Interaction& a_rhs) const {return event == a_rhs.event ? action == a_rhs.action ? text < a_rhs.text : action < a_rhs.action : event < a_rhs.event;};
	bool operator==(const Interaction & a_rhs) const {return action == a_rhs.action && event == a_rhs.event && text == a_rhs.text;}
};