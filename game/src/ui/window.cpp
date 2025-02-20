#include <array>
#include <numbers>
#include <string>
#include <imgui.h>

#include <core/log.hpp>
#include <resources/resource.fwd.hpp>
#include <resources/resource_data_store.hpp>
#include <resources/resource_system.hpp>
#include <pandora.hpp>

#include "ui/window.hpp"
#include "ui/prefab_editor.hpp"
#include "ui/stack.hpp"
#include "game.hpp"

namespace WingsOfSteel::TheBrightestStar::UI
{

Window::Window()
{

}

Window::~Window()
{

}

const std::string& Window::GetIcon() const
{
    static const std::string icon(ICON_FA_WINDOW_MAXIMIZE);
    return icon;
}

void Window::Initialize(const std::string& prefabPath)
{
    using namespace Pandora;
    GetResourceSystem()->RequestResource(prefabPath, [this, prefabPath](ResourceSharedPtr pResource) {
        m_pDataStore = std::dynamic_pointer_cast<ResourceDataStore>(pResource);
        Deserialize(m_pDataStore->Data());
        WindowSharedPtr pWindow = std::static_pointer_cast<Window>(shared_from_this());
        Game::Get()->GetPrefabEditor()->AddPrefabData(prefabPath, pWindow);
    });
}

nlohmann::json Window::Serialize() const
{
    nlohmann::json data = Element::Serialize();
    data["w"] = m_Width;
    data["h"] = m_Height;
    if (m_pStack)
    {
        data["stack"] = m_pStack->Serialize();
    }
    return data;
}

void Window::Deserialize(const nlohmann::json& data)
{
    Element::Deserialize(data);
    TryDeserialize(data, "w", m_Width, 256);
    TryDeserialize(data, "h", m_Height, 256);

    if (data.contains("stack"))
    {
        const nlohmann::json& stackData = data["stack"];
        if (stackData.is_object())
        {
            m_pStack = std::make_shared<Stack>();
            m_pStack->Deserialize(stackData);
        }
    }
}

void Window::Render()
{
    if (!m_pDataStore)
    {
        return;
    }

    const ImVec2 windowSize(GetWidth(), GetHeight());
    ImVec2 viewportSize = ImGui::GetMainViewport()->Size;
    ImVec2 windowPos(
        (viewportSize.x - windowSize.x) * 0.5f,
        (viewportSize.y - windowSize.y) * 0.5f
    );

    ImGui::SetNextWindowSize(windowSize, ImGuiCond_Always);
    ImGui::SetNextWindowPos(windowPos, ImGuiCond_Always);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBackground;
    ImGui::Begin("test", nullptr, windowFlags);

    RenderBackground();

    if (m_pStack)
    {
        ImGui::SetCursorPos(ImVec2(0, 0));
        m_pStack->Render();
    }

    ImGui::End();
    ImGui::PopStyleVar();
}

void Window::RenderProperties()
{
    Element::RenderProperties();
    ImGui::InputInt("Width", &m_Width);
    ImGui::InputInt("Height", &m_Height);
}

void Window::AddElement(ElementSharedPtr pElement)
{
    //m_Elements.push_back(pElement);
}

void Window::RenderBackground()
{
    const ImVec2 cp0 = ImGui::GetCursorScreenPos(); // ImDrawList API uses screen coordinates!
    const ImVec2 cp1 = cp0 + ImVec2(GetWidth(), GetHeight());

    //const Theme* pActiveTheme = g_pGame->GetThemeManager()->GetActiveTheme();
    static const ImU32 accentColor = IM_COL32(5, 250, 191, 255);
    static const ImU32 backgroundStartColor = IM_COL32(46, 46, 46, 240);
    static const ImU32 backgroundEndColor = IM_COL32(20, 20, 20, 240);
    static const float notchSize = 16.0f;
    ImDrawList* pDrawList = ImGui::GetWindowDrawList();

    std::array<ImVec2, 5> background;
    background[ 0 ] = ImVec2(cp0.x, cp0.y);
    background[ 1 ] = ImVec2(cp1.x, cp0.y);
    background[ 2 ] = ImVec2(cp1.x, cp1.y - notchSize);
    background[ 3 ] = ImVec2(cp1.x - notchSize, cp1.y);
    background[ 4 ] = ImVec2(cp0.x, cp1.y);

    std::array<ImU32, 5> backgroundColors;
    backgroundColors[ 0 ] = backgroundStartColor;
    backgroundColors[ 1 ] = backgroundStartColor;
    backgroundColors[ 2 ] = backgroundEndColor;
    backgroundColors[ 3 ] = backgroundEndColor;
    backgroundColors[ 4 ] = backgroundEndColor;
    pDrawList->AddConvexPolyFilledMultiColor(background.data(), backgroundColors.data(), static_cast<int>(background.size()));

    const float topAccentHeight = 6.0f;
    pDrawList->AddRectFilled(ImVec2(cp0.x, cp0.y), ImVec2(cp1.x, cp0.y + topAccentHeight), accentColor);

    static const int gridAlpha = 6;
    static const ImU32 gridColor = IM_COL32(255, 255, 255, gridAlpha);
    static const float gridStep = 48.0f;
    static const float gridOffset = -16.0f;
    static const float gridThickness = 1.5f;
    for (float x = cp0.x + gridOffset; x < cp1.x; x += gridStep)
    {
        pDrawList->AddLine(ImVec2(x, cp0.y), ImVec2(x, cp1.y), gridColor, gridThickness);
    }
    for (float y = cp0.y + gridOffset; y < cp1.y; y += gridStep)
    {
        pDrawList->AddLine(ImVec2(cp0.x, y), ImVec2(cp1.x, y), gridColor, gridThickness);
    }
}

} // namespace WingsOfSteel::TheBrightestStar::UI