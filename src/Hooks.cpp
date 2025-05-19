#include "Hooks.h"
#include "IconsFonts.h"
#include "Input.h"
#include "Renderer.h"
#include "Styles.h"
#include "imgui_impl_dx11.h"
#include "imgui_impl_win32.h"


using namespace ImGui::Renderer;

LRESULT WndProc::thunk(const HWND hWnd, const UINT uMsg, const WPARAM wParam, const LPARAM lParam) {
    auto& io = ImGui::GetIO();
    if (uMsg == WM_KILLFOCUS) {
        io.ClearInputKeys();
    }
    return func(hWnd, uMsg, wParam, lParam);
}


void CreateD3DAndSwapChain::thunk() {
    func();

    if (const auto renderer = RE::BSGraphics::Renderer::GetSingleton()) {
        swapChain = reinterpret_cast<IDXGISwapChain*>(renderer->data.renderWindows[0].swapChain);
        if (!swapChain) {
            logger::error("couldn't find swapChain");
            return;
        }

        DXGI_SWAP_CHAIN_DESC desc{};
        if (FAILED(swapChain->GetDesc(std::addressof(desc)))) {
            logger::error("IDXGISwapChain::GetDesc failed.");
            return;
        }

        device = reinterpret_cast<ID3D11Device*>(renderer->data.forwarder);
        context = reinterpret_cast<ID3D11DeviceContext*>(renderer->data.context);

        logger::info("Initializing ImGui...");

        ImGui::CreateContext();

        auto& io = ImGui::GetIO();
        io.ConfigFlags = ImGuiConfigFlags_NavEnableKeyboard | ImGuiConfigFlags_NavEnableGamepad;
        io.IniFilename = nullptr;

        if (!ImGui_ImplWin32_Init(desc.OutputWindow)) {
            logger::error("ImGui initialization failed (Win32)");
            return;
        }
        if (!ImGui_ImplDX11_Init(device, context)) {
            logger::error("ImGui initialization failed (DX11)");
            return;
        }

        MANAGER(IconFont)->LoadIcons();
        logger::info("ImGui initialized.");

        MCP::Settings::initialized.store(true);

        WndProc::func = reinterpret_cast<WNDPROC>(
            SetWindowLongPtrA(
                desc.OutputWindow,
                GWLP_WNDPROC,
                reinterpret_cast<LONG_PTR>(WndProc::thunk)));
        if (!WndProc::func) {
            logger::error("SetWindowLongPtrA failed!");
        }
    }
}

void DrawHook::thunk(std::uint32_t a_timer) {
    func(a_timer);

    if (!MCP::Settings::initialized.load()) {
        return;
    }

    ImGui::Styles::GetSingleton()->OnStyleRefresh();

    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    {
        // trick imgui into rendering at game's real resolution (ie. if upscaled with Display Tweaks)
        static const auto screenSize = RE::BSGraphics::Renderer::GetScreenSize();

        auto& io = ImGui::GetIO();
        io.DisplaySize.x = static_cast<float>(screenSize.width);
        io.DisplaySize.y = static_cast<float>(screenSize.height);
    }
    ImGui::NewFrame();
    {
        RenderPrompts();
    }
    ImGui::EndFrame();
    ImGui::Render();
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}


void ImGui::Renderer::UpdateMaxIntervalBetweenPresses()
{
    const auto responsiveness = MCP::Settings::SpecialCommands::responsiveness;
	// at responsiveness == 1, the max interval between presses is 220ms
	// at responsiveness == 0, the max interval between presses is 1000ms
	maxIntervalBetweenPresses = std::chrono::milliseconds(static_cast<int>(1000 - (780)*responsiveness));
}

void ImGui::Renderer::Install()
{
    auto& trampoline = SKSE::GetTrampoline();
    constexpr size_t size_per_hook = 14;
	trampoline.create(size_per_hook*3);
    const REL::Relocation<std::uintptr_t> target{REL::RelocationID(75595, 77226)};  // BSGraphics::InitD3D
    CreateD3DAndSwapChain::func = trampoline.write_call<5>(target.address() + REL::Relocate(0x9, 0x275), CreateD3DAndSwapChain::thunk);

	const REL::Relocation<std::uintptr_t> target2{REL::RelocationID(75461, 77246)}; // BSGraphics::Renderer::End
    DrawHook::func = trampoline.write_call<5>(target2.address() + 0x9, DrawHook::thunk);

    const REL::Relocation<std::uintptr_t> target3{REL::RelocationID(67315, 68617)};
    InputHook::func = trampoline.write_call<5>(target3.address() + 0x7B, InputHook::thunk);

    MenuHook<RE::LoadingMenu>::InstallHook(RE::VTABLE_LoadingMenu[0]);
}

void ImGui::Renderer::InputHook::thunk(RE::BSTEventSource<RE::InputEvent*>* a_dispatcher, RE::InputEvent* const* a_event)
{
	if (!a_dispatcher || !a_event) {
		return func(a_dispatcher, a_event);
	}

    auto first = *a_event;
    auto last = *a_event;
    size_t length = 0;

    for (auto current = *a_event; current; current = current->next) {
        if (ProcessInput(current)) {
            if (current != last) {
                last->next = current->next;
            } else {
                last = current->next;
                first = current->next;
            }
        } else {
            last = current;
            ++length;
        }
    }

    if (length == 0) {
        constexpr RE::InputEvent* const dummy[] = {nullptr};
        func(a_dispatcher, dummy);
    } else {
        RE::InputEvent* const e[] = {first};
        func(a_dispatcher, e);
    }
}

bool ImGui::Renderer::InputHook::ProcessInput(RE::InputEvent* event)
{
    bool block = false;

    const auto render_manager = MANAGER(ImGui::Renderer);
	if (render_manager->IsPaused()) return block;
	if (render_manager->IsHidden()) return block;

    const auto input_manager = MANAGER(Input);
	input_manager->UpdateInputDevice(event);

	if (const auto button_event = event->AsButtonEvent()) {
		const auto key = input_manager->Convert(button_event->GetIDCode(),button_event->GetDevice());
        for (const auto prompt_keys = render_manager->GetPromptKeys(); const auto& prompt_key : prompt_keys) {
            if (prompt_key!=0 && prompt_key == key) {
			    block = true;
			    const auto now = std::chrono::steady_clock::now();
                if (const auto submanager = render_manager->GetSubManagerByKey(prompt_key)) {
					submanager->buttonState.isPressing = button_event->IsPressed();
                    if (button_event->IsDown()) {
                        submanager->buttonState.pressCount++;
                        submanager->buttonState.lastPressTime = now;
                        submanager->SendEvent(submanager->GetCurrentInteraction(),SkyPromptAPI::PromptEventType::kDown);
					}
					else if (button_event->IsUp()) {
						submanager->SendEvent(submanager->GetCurrentInteraction(), SkyPromptAPI::PromptEventType::kUp);
					}
                    if (submanager->buttonState.isPressing) {
                        if (const auto held_dur = button_event->HeldDuration() * 1000.f; 
                            now - submanager->buttonState.lastPressTime < std::chrono::milliseconds(100+static_cast<int>(held_dur))) {
					        submanager->UpdateProgressCircle(true);
						}
                    }
                    else {
					    submanager->UpdateProgressCircle(false);
                    }
                }
		    }
		}

	    if (!block && button_event->IsDown()) {
			const auto device = input_manager->GetInputDevice();
            const bool is_L = key == MCP::Settings::cycle_L[device];
			const bool is_R = key == MCP::Settings::cycle_R[device];
		    if (is_L || is_R) {
				block = Manager::GetSingleton()->CycleClient(is_L);
		    }
        }
	}
	else if (const auto mouse_event = event->AsMouseMoveEvent()) {
        constexpr auto key = SkyPromptAPI::kMouseMove;
		for (const auto prompt_keys = render_manager->GetPromptKeys(); const auto & prompt_key : prompt_keys) {
			if (prompt_key != 0 && prompt_key == key) {
				block = true;
				if (const auto submanager = render_manager->GetSubManagerByKey(prompt_key)) {
                    submanager->SendEvent(submanager->GetCurrentInteraction(), SkyPromptAPI::PromptEventType::kMove, {static_cast<float>(mouse_event->mouseInputX),static_cast<float>(mouse_event->mouseInputY)});
					submanager->UpdateProgressCircle(mouse_event->mouseInputX != 0 || mouse_event->mouseInputY != 0);
				}
			}
		}
	}
	else if (const auto thumbstick_event = event->AsThumbstickEvent()) {
		constexpr auto key = SkyPromptAPI::kThumbstickMove;
		for (const auto prompt_keys = render_manager->GetPromptKeys(); const auto & prompt_key : prompt_keys) {
			if (prompt_key != 0 && prompt_key == key) {
				block = true;
				if (const auto submanager = render_manager->GetSubManagerByKey(prompt_key)) {
					submanager->SendEvent(submanager->GetCurrentInteraction(), SkyPromptAPI::PromptEventType::kMove, { thumbstick_event->xValue,thumbstick_event->yValue });
					submanager->UpdateProgressCircle(thumbstick_event->xValue != 0.f || thumbstick_event->yValue != 0.f);
				}
			}
		}
	}

	return block;
}

bool ImGui::Renderer::InputHook::IsOtherButtonPressed(RE::InputEvent* const* a_event)
{
	auto curr_prompt_keys = MANAGER(ImGui::Renderer)->GetPromptKeys();
	const std::unordered_set<uint32_t> prompt_keys(curr_prompt_keys.begin(), curr_prompt_keys.end());
    const auto input_manager = MANAGER(Input);
	const auto user_events = RE::UserEvents::GetSingleton();
	for (auto current = *a_event; current; current = current->next) {
		if (const auto button_event = current->AsButtonEvent()) {
            if (!button_event->IsPressed()) continue;
            const auto key = input_manager->Convert(button_event->GetIDCode(),button_event->GetDevice());
			if (prompt_keys.contains(key)) {
				continue;
			}
			if (button_event->userEvent == user_events->move) {
				continue;
            }
			return true;
        }
    }

    return false;
}

template <typename MenuType>
RE::UI_MESSAGE_RESULTS MenuHook<MenuType>::ProcessMessage_Hook(RE::UIMessage& a_message) {
    if (const std::string_view menuname = MenuType::MENU_NAME; a_message.menu==menuname) {
        if (const auto msg_type = static_cast<int>(a_message.type.get()); msg_type == 1) {
            if (menuname == RE::LoadingMenu::MENU_NAME) {
                MCP::Settings::initialized.store(false);
                MANAGER(ImGui::Renderer)->Stop();
            }
        }
        else if (msg_type == 3) {
            MCP::Settings::initialized.store(true);
        }
    }
    return ProcessMessage_(this, a_message);
}

template <typename MenuType>
void MenuHook<MenuType>::InstallHook(const REL::VariantID& varID) {
    REL::Relocation<std::uintptr_t> vTable(varID);
    ProcessMessage_ = vTable.write_vfunc(0x4, &MenuHook<MenuType>::ProcessMessage_Hook);
}