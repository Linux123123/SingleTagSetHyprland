#include <hyprland/src/Compositor.hpp>
#include <hyprland/src/config/ConfigManager.hpp>
#include <hyprland/src/helpers/Color.hpp>
#include <hyprland/src/helpers/Workspace.hpp>
#include <hyprland/src/managers/KeybindManager.hpp>

#include "globals.hpp"

#include <algorithm>
#include <any>
#include <map>
#include <thread>
#include <unistd.h>
#include <vector>

const CColor s_pluginColor = {0xC3 / 255.0f, 0xB0 / 255.0f, 0x91 / 255.0f, 1.0f};

static HOOK_CALLBACK_FN* e_monitorAddedHandle = nullptr;
static HOOK_CALLBACK_FN* e_monitorRemovedHandle = nullptr;

void monitorWorkspace(std::string workspace)
{
    CMonitor* monitor = g_pCompositor->getMonitorFromCursor();
    CWorkspace* workspaceTo = g_pCompositor->getWorkspaceByID(monitor->activeWorkspace);
    CWorkspace* workspaceFrom = g_pCompositor->getWorkspaceByName(workspace);

    if (workspaceFrom == nullptr || workspaceFrom->m_iMonitorID == workspaceTo->m_iMonitorID) {
        HyprlandAPI::invokeHyprctlCommand("dispatch", "workspace " + workspace);
        return;
    }

    if (workspaceFrom->m_iID == workspaceTo->m_iID) {
        return;
    }

    CMonitor* monitorWorkFrom = g_pCompositor->getMonitorFromID(workspaceFrom->m_iMonitorID);

    if (monitorWorkFrom->activeWorkspace != workspaceFrom->m_iID) {
        g_pCompositor->moveWorkspaceToMonitor(workspaceFrom, monitor);
        if (monitor->activeWorkspace != workspaceFrom->m_iID) {
            monitor->changeWorkspace(workspaceFrom, false);
        }
        return;
    }

    g_pCompositor->moveWorkspaceToMonitor(workspaceFrom, monitor);
    if (monitor->activeWorkspace != workspaceFrom->m_iID) {
        monitor->changeWorkspace(workspaceFrom, false);
    }
    g_pCompositor->moveWorkspaceToMonitor(workspaceTo, monitorWorkFrom);
    if (monitorWorkFrom->activeWorkspace != workspaceTo->m_iID) {
        monitorWorkFrom->changeWorkspace(workspaceTo, false);
    }
}

void mapWorkspacesToMonitors()
{
    int monitorIndex = 0;
    for (auto& monitor : g_pCompositor->m_vMonitors) {
        std::string workspaceName = std::to_string(monitorIndex);
        HyprlandAPI::invokeHyprctlCommand("keyword", "workspace " + workspaceName + "," + monitor->szName);
        CWorkspace* workspace = g_pCompositor->getWorkspaceByName(workspaceName);

        if (workspace != nullptr) {
            g_pCompositor->moveWorkspaceToMonitor(workspace, monitor.get());
        }
        HyprlandAPI::invokeHyprctlCommand("dispatch", "workspace " + std::to_string(monitorIndex));
        monitorIndex++;
    }
}

void refreshMapping(void*, std::any value)
{
    mapWorkspacesToMonitors();
}

// Do NOT change this function.
APICALL EXPORT std::string PLUGIN_API_VERSION()
{
    return HYPRLAND_API_VERSION;
}

APICALL EXPORT PLUGIN_DESCRIPTION_INFO PLUGIN_INIT(HANDLE handle)
{
    PHANDLE = handle;

    HyprlandAPI::addDispatcher(PHANDLE, "single-tag-workspace", monitorWorkspace);

    HyprlandAPI::reloadConfig();

    mapWorkspacesToMonitors();

    HyprlandAPI::addNotification(PHANDLE, "[single-tag-set] Initialized successfully!", s_pluginColor, 5000);

    e_monitorAddedHandle = HyprlandAPI::registerCallbackDynamic(PHANDLE, "monitorAdded", refreshMapping);
    e_monitorRemovedHandle = HyprlandAPI::registerCallbackDynamic(PHANDLE, "monitorRemoved", refreshMapping);

    return {"single-tag-set", "Single workspace", "Linux123123", "1.0"};
}

APICALL EXPORT void PLUGIN_EXIT()
{
    HyprlandAPI::addNotification(PHANDLE, "[single-tag-set] Unloaded successfully!", s_pluginColor, 5000);
}
