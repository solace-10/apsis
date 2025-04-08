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

    void Initialize(const std::string& prefabPath);
    nlohmann::json Serialize() const override;
    void Deserialize(const nlohmann::json& data) override;

    virtual void OnInitializationCompleted() {}

    ElementType GetType() const override;
    const std::string& GetIcon() const override;

    void Render() override;
    void RenderProperties() override;

    template <typename T>
    std::shared_ptr<T> FindElement(const std::string& path) const
    {
        return std::dynamic_pointer_cast<T>(FindElementInternal(path));
    }

    StackSharedPtr GetStack() const;
    Pandora::ResourceDataStore* GetDataStore() const;

private:
    void RenderBackground();
    ElementSharedPtr FindElementInternal(const std::string& path) const;
    ElementSharedPtr FindElementHierarchyDescent(const std::vector<std::string>& tokens, size_t currentElementIdx, StackSharedPtr pStackElement) const;

    Pandora::ResourceDataStoreSharedPtr m_pDataStore;
    StackSharedPtr m_pStack;
};

inline ElementType Window::GetType() const
{
    return ElementType::Window;
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