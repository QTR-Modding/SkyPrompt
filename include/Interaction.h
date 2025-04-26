#pragma once

namespace ACTIONS {

    enum class Actions {
        kNone,
		kPrompt1,
		kPrompt2,
		kPrompt3,
		kPrompt4,
		kHint1,
		kHint2,
		kHint3,
		kHint4,
		kHintCenter,
        kTotal
    };

    inline std::map<Actions, std::string> action_strings = {
        {Actions::kNone, "kNone"},
		{Actions::kPrompt1, "Prompt1"},
		{Actions::kPrompt2, "Prompt2"},
		{Actions::kPrompt3, "Prompt3"},
		{Actions::kPrompt4, "Prompt4"},
		{Actions::kHint1, "Hint1"},
		{Actions::kHint2, "Hint2"},
		{Actions::kHint3, "Hint3"},
		{Actions::kHint4, "Hint4"},
		{Actions::kHintCenter, "HintCenter"},
        {Actions::kTotal, "kTotal"}

    };

    inline std::string_view GetActionString(const Actions e) {
	    static constexpr std::string_view unknown = "Unknown";
        const auto it = action_strings.find(e);
        return (it != action_strings.end()) ? it->second : unknown;
	}


    struct Action
    {
	    ACTIONS::Actions action;
		Action():action(ACTIONS::Actions::kNone){};
        Action(const ACTIONS::Actions& a_action) : action(a_action){};
	    [[nodiscard]] std::string_view text() const {return ACTIONS::GetActionString(action);}
        bool operator<(const Action& a_rhs) const {return action < a_rhs.action;};
        bool operator==(const Action& a_rhs) const {return action == a_rhs.action;};
		Action& operator=(const Action& a_rhs) = default;
    };
};

namespace SCENES {
    enum Events {
	    kNone = 0,
		kPrompt1,
		kPrompt2,
		kPrompt3,
		kPrompt4,
		kHint1,
		kHint2,
		kHint3,
		kHint4,
		kHintCenter,
	    kTotal
    };
};


struct Interaction {

	ACTIONS::Action action = ACTIONS::Actions::kNone;
	SCENES::Events event = SCENES::Events::kNone;
	std::string extra_text;

	Interaction() = default;
	Interaction(const Interaction& a_rhs) {
		action = a_rhs.action;
		event = a_rhs.event;
		extra_text = a_rhs.extra_text;
	}
	Interaction(const SCENES::Events& a_event, const ACTIONS::Actions& a_action) : action(a_action), event(a_event) {}

	[[nodiscard]] std::string_view action_name() const { return action.text(); }
	[[nodiscard]] std::string name() const {
        std::string result;
        if (!extra_text.empty()) {
            result.reserve(action_name().size() + 1 + extra_text.size());
            result = std::string(action_name()) + " " + extra_text;
        } else {
            result = std::string(action_name());
        }
        return result;
	};

    bool operator<(const Interaction& a_rhs) const {return action == a_rhs.action ? event < a_rhs.event : action < a_rhs.action;};
	bool operator==(const Interaction & a_rhs) const {return action == a_rhs.action && event == a_rhs.event;}
};