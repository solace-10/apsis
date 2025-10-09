#pragma once


namespace WingsOfSteel
{

class IDebugUI
{
public:
    IDebugUI() = default;
    ~IDebugUI() = default;

    virtual void DrawDebugUI() = 0;
    bool IsDebugUIVisible() const { return m_ShowDebugUI; }
    void ShowDebugUI(bool state) { m_ShowDebugUI = state; }

protected:
    bool m_ShowDebugUI{false};
};

} // namespace WingsOfSteel