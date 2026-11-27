#pragma once

#include "imgui.h"
#include <string>
#include <functional>

void EnsureImGuiWindow();
void ExitImguiWindow();

void DrawImGuiWindowUntil(const char * windowName, std::function<bool(void)> callback);

class ExitImguiLater {
public:
	~ExitImguiLater() {
		ExitImguiWindow();
	}
};