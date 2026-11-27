#include "imgui_windows.h"

#include "imgui_driver.h"

void ImGuiDisplayLoadingWindow() {
    //This is a demo window, not used
    DrawImGuiWindowUntil("Hello, world", []() {
        ImGui::Text(u8"一部分补丁没有成功，但可以继续");
        if (ImGui::Button(u8"点击将继续加载中文补丁")) {
            return true;
        }
        return false;
    });
}