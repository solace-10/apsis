#pragma once

#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

#include <core/smart_ptr.hpp>
#include <resources/resource.fwd.hpp>

#include "ui/element.hpp"
#include "ui/prefab_data.hpp"

namespace WingsOfSteel::TheBrightestStar::UI
{

DECLARE_SMART_PTR(Stack);

DECLARE_SMART_PTR(Window);
class Window : public Element
{
public:
    Window();
    ~Window() override;

    virtual void Initialize(const std::string& prefabPath);
    nlohmann::json Serialize() const override;
    void Deserialize(const nlohmann::json& data) override;

    ElementType GetType() const override;
    const std::string& GetIcon() const override;

    void Render() override;
    void RenderProperties() override;

    void AddElement(ElementSharedPtr pElement);

    void SetWidth(int width);
    void SetHeight(int height);
    int GetWidth() const;
    int GetHeight() const;

    StackSharedPtr GetStack() const;
    Pandora::ResourceDataStore* GetDataStore() const;

private:
    void RenderBackground();

    Pandora::ResourceDataStoreSharedPtr m_pDataStore;

    int m_Width{0};
    int m_Height{0};
    StackSharedPtr m_pStack;
};

inline ElementType Window::GetType() const
{
    return ElementType::Window;
}

inline void Window::SetWidth(int width)
{
    m_Width = width;
}

inline void Window::SetHeight(int height)
{
    m_Height = height;
}

inline int Window::GetWidth() const
{
    return m_Width;
}

inline int Window::GetHeight() const
{
    return m_Height;
}

inline StackSharedPtr Window::GetStack() const
{
    return m_pStack;
}

inline Pandora::ResourceDataStore* Window::GetDataStore() const
{
    return m_pDataStore.get();
}

} // namespace WingsOfSteel::TheBrightestStar::UI