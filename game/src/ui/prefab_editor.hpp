#pragma once

#include <vector>

#include <ui/prefab_data.hpp>

namespace WingsOfSteel::TheBrightestStar::UI
{

class PrefabEditor
{
public:
    PrefabEditor();
    ~PrefabEditor();

    void Initialize();
    void ShowPrefabEditor(bool state);
    void DrawPrefabEditor();

    void AddPrefabData(PrefabDataSharedPtr pPrefabData);

private:
    bool m_ShowPrefabEditor{false};

    std::vector<PrefabDataWeakPtr> m_PrefabData;
    PrefabDataSharedPtr m_pSelectedPrefabData;
};

} // namespace WingsOfSteel::TheBrightestStar::UI