#pragma once
#include "imgui.h"

namespace ImGui
{
    bool TabButton(const char* icon, const char* label, bool active, ImVec2 size = { 140, 45 });
    bool Switch(const char* label, bool* v, float rounding_bg = 15.f, float rounding_knob = 9.5f);
    bool TextSwitch(const char* label, bool* v);
    bool AButton(const char* label, const ImVec2& size = { 200 , 40 });;
    void DrawRectShadow(ImDrawList* draw, ImVec2 rect_min, ImVec2 rect_max, float radius, ImU32 color, float rounding = 5.f, int steps = 12, float alpha_multiplier = 1.0f);
    void ABeginChild(const char* label, ImVec2 Size);
    void ABeginChildEnd();
    bool ASliderFloat(const char* label, float* value, float min, float max, float width);
    bool ASliderInt(const char* label, int* value, int min, int max, float width);
    void ASeparator(float thickness = 1.f);
    void TipText(const char* fmt);

    ImGuiID ASettingButton(const char* id_str, const char* icon, bool* opened);
    bool ASettingBegin(ImGuiID id, ImVec2 size);
    void ASettingEnd();
}