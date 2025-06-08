#pragma once

namespace ImGui::Renderer
{
    inline std::chrono::milliseconds maxIntervalBetweenPresses(220);
	void UpdateMaxIntervalBetweenPresses();

	void Install();

	struct WndProc
	{
		static LRESULT thunk(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
        static inline WNDPROC func;
	};

	struct CreateD3DAndSwapChain
	{
        static inline IDXGISwapChain* swapChain = nullptr;
        static inline ID3D11Device* device = nullptr;
        static inline ID3D11DeviceContext* context = nullptr;
		static void thunk();
        static inline REL::Relocation<decltype(thunk)> func;
	};

    struct DrawHook {
		static void thunk(std::uint32_t a_timer);
        static inline REL::Relocation<decltype(thunk)> func;

	};

	struct InputHook {
		static void thunk(RE::BSTEventSource<RE::InputEvent*>* a_dispatcher, RE::InputEvent* const* a_event);
		static inline REL::Relocation<decltype(thunk)> func;
		static bool ProcessInput(RE::InputEvent* event);
	};

	template <typename MenuType>
    class MenuHook : public MenuType {
        using ProcessMessage_t = decltype(&MenuType::ProcessMessage);
        static inline REL::Relocation<ProcessMessage_t> ProcessMessage_;
        RE::UI_MESSAGE_RESULTS ProcessMessage_Hook(RE::UIMessage& a_message);
    public:
        static void InstallHook(const REL::VariantID& varID);
    };

}