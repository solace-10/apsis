#include <array>
#include <numbers>
#include <string>
#include <sstream>
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

namespace WingsOfSteel::UI
{

static const float sThemeWindowAccentHeight = 6.0f;

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
    GetResourceSystem()->RequestResource(prefabPath, [this, prefabPath](ResourceSharedPtr pResource) {
        m_pDataStore = std::dynamic_pointer_cast<ResourceDataStore>(pResource);
        Deserialize(m_pDataStore->Data());
        WindowSharedPtr pWindow = std::static_pointer_cast<Window>(shared_from_this());
        Game::Get()->GetPrefabEditor()->AddPrefabData(prefabPath, pWindow);
        OnInitializationCompleted();
    });
}

nlohmann::json Window::Serialize() const
{
    nlohmann::json data = Element::Serialize();
    if (m_pStack)
    {
        data["stack"] = m_pStack->Serialize();
    }
    return data;
}

void Window::Deserialize(const nlohmann::json& data)
{
    Element::Deserialize(data);
    if (data.contains("stack"))
    {
        const nlohmann::json& stackData = data["stack"];
        if (stackData.is_object())
        {
            m_pStack = std::make_shared<Stack>();
            m_pStack->SetParent(shared_from_this());
            m_pStack->Deserialize(stackData);
        }
    }
}

void Window::Render()
{
    if (!m_pDataStore || HasFlag(Flags::Hidden))
    {
        return;
    }

    const ImVec2 windowSize = GetSize();
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

    if (m_pStack && !m_pStack->HasFlag(Flags::Hidden))
    {
        m_pStack->SetPosition(glm::ivec2(0, sThemeWindowAccentHeight));
        m_pStack->SetSize(glm::ivec2(windowSize.x, windowSize.y - sThemeWindowAccentHeight));
        m_pStack->Render();
    }

    if (HasFlag(Flags::SelectedInEditor))
    {
        ImDrawList* pDrawList = ImGui::GetWindowDrawList();
        pDrawList->AddRect(windowPos, windowPos + windowSize, IM_COL32(255, 0, 0, 255));
    }

    ImGui::End();
    ImGui::PopStyleVar();
}

void Window::RenderProperties()
{
    Element::RenderProperties();
}

void Window::RenderBackground()
{
    const ImVec2 cp0 = ImGui::GetCursorScreenPos(); // ImDrawList API uses screen coordinates!
    const ImVec2 cp1 = cp0 + GetSize();

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

    pDrawList->AddRectFilled(ImVec2(cp0.x, cp0.y), ImVec2(cp1.x, cp0.y + sThemeWindowAccentHeight), accentColor);

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

ElementSharedPtr Window::FindElementInternal(const std::string& path) const
{
    std::vector<std::string> tokens;
    std::stringstream ss(path);
    std::string token;
    while(std::getline(ss, token, '/'))
    {
        tokens.push_back(token);
    }

    if (tokens.size() < 3 || !m_pStack)
    {
        return nullptr;
    }

    ElementSharedPtr pFoundElement = FindElementHierarchyDescent(tokens, 2, m_pStack);
    if (pFoundElement)
    {
        pFoundElement->AddFlag(Flags::Bound);
        return pFoundElement;
    }
    else
    {
        return nullptr;
    }
}

ElementSharedPtr Window::FindElementHierarchyDescent(const std::vector<std::string>& tokens, size_t currentElementIdx, StackSharedPtr pStackElement) const
{
    if (currentElementIdx + 1 == tokens.size())
    {
        return nullptr;
    }

    const size_t nextElementIdx = currentElementIdx + 1;
    const std::string& nextElementName = tokens[nextElementIdx];
    for (auto& pChildElement : pStackElement->GetElements())
    {
        if (pChildElement->GetName() == nextElementName)
        {
            const bool isLeaf = (nextElementIdx == tokens.size() - 1);
            if (isLeaf)
            {
                return pChildElement;
            }
            else
            {
                StackSharedPtr pStackChildElement = std::dynamic_pointer_cast<Stack>(pChildElement);
                if (pStackChildElement)
                {
                    return FindElementHierarchyDescent(tokens, nextElementIdx, pStackChildElement);
                }
                else
                {
                    return nullptr;
                }
            }
        }
    }

    return nullptr;
}

} // namespace WingsOfSteel::UI