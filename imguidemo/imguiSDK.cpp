#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui.h"
#include "imguiSDK.h"
#include "imgui_internal.h"
#include <string>
#include <unordered_map>

struct SmoothScrollState
{
    float Current = 0.0f;
    float Target = 0.0f;
};

static std::unordered_map<ImGuiID, SmoothScrollState> g_ScrollStates;

static float ImSmootherStep(float current, float target, float speed)
{
    return current + (target - current) * speed;
}

bool ImGui::TabButton(const char* icon, const char* label, bool active, ImVec2 size)
{
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(label);

    const ImVec2 pos = window->DC.CursorPos;
    const ImRect bb(pos, ImVec2(pos.x + size.x, pos.y + size.y));

    ImGui::ItemSize(bb, style.FramePadding.y);
    if (!ImGui::ItemAdd(bb, id))
        return false;

    bool hovered, held;
    bool pressed = ImGui::ButtonBehavior(bb, id, &hovered, &held);

    ImGuiStorage* storage = ImGui::GetStateStorage();
    float anim = storage->GetFloat(id, active ? 1.0f : 0.0f);
    float target = active ? 1.0f : 0.0f;

    float speed = g.IO.DeltaTime * 12.0f;
    if (speed >= 1.0f)
        anim = target;
    else
        anim += (target - anim) * speed;

    storage->SetFloat(id, anim);

    ImVec4 off = { 0.3f, 0.3f, 0.3f, 0.0f };
    ImVec4 on = { 0.3f, 0.3f, 0.3f, 1.0f };

    ImVec4 bg = ImVec4(
        off.x + (on.x - off.x) * anim,
        off.y + (on.y - off.y) * anim,
        off.z + (on.z - off.z) * anim,
        off.w + (on.w - off.w) * anim);

    ImDrawList* draw = window->DrawList;
    float rounding = style.FrameRounding;
    draw->AddRectFilled(bb.Min, bb.Max, ImGui::GetColorU32(bg), rounding);

    ImVec2 iconSize = ImGui::CalcTextSize(icon);

    ImVec2 iconPos(bb.Min.x + style.FramePadding.x + 5, bb.Min.y + (size.y - iconSize.y) * 0.5f + 1.0f);

    draw->AddText(iconPos, ImGui::GetColorU32(ImGuiCol_TextDisabled), icon);

    ImVec2 textSize = ImGui::CalcTextSize(label);

    ImVec2 textPos(iconPos.x + 28.0f, bb.Min.y + (size.y - textSize.y) * 0.5f);

    draw->AddText(textPos, ImGui::GetColorU32(ImGuiCol_TextDisabled), label);

    return pressed;
}

bool ImGui::Switch(const char* label, bool* v, float rounding_bg, float rounding_knob)
{
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (!window || window->SkipItems)
        return false;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(label);

    float height = ImGui::GetFrameHeight();
    float width = height * 2.2f * 0.85f * 1.03f;

    ImVec2 pos = window->DC.CursorPos;
    ImRect bb(pos, ImVec2(pos.x + width, pos.y + height));
    ImGui::ItemSize(bb, style.FramePadding.y);

    if (!ImGui::ItemAdd(bb, id))
        return false;

    bool hovered = false, held = false;
    bool clicked = ImGui::ButtonBehavior(bb, id, &hovered, &held);

    if (clicked)
        *v = !*v;

    ImGuiStorage* storage = ImGui::GetStateStorage();
    float anim = storage->GetFloat(id, *v ? 1.0f : 0.0f);
    float target = *v ? 1.0f : 0.0f;
    anim = ImSmootherStep(anim, target, g.IO.DeltaTime * 12.0f);
    storage->SetFloat(id, anim);

    ImVec4 off = style.Colors[ImGuiCol_FrameBg];
    ImVec4 on = style.Colors[ImGuiCol_CheckMark];
    ImVec4 bg = off;

    if (*v || anim > 0.001f) {
        bg.x = off.x + (on.x - off.x) * anim;
        bg.y = off.y + (on.y - off.y) * anim;
        bg.z = off.z + (on.z - off.z) * anim;
        bg.w = off.w + (on.w - off.w) * anim;
    }

    ImDrawList* draw = window->DrawList;
    draw->AddRectFilled(ImVec2(bb.Min.x, bb.Min.y), ImVec2(bb.Max.x, bb.Max.y), ImGui::GetColorU32(bg), rounding_bg);

    float knob_r = rounding_knob;
    float padding = knob_r * 0.25f;
    float usable_width = bb.GetWidth() - (padding * 2.0f + knob_r * 2.0f);
    float x = bb.Min.x + padding + knob_r + usable_width * anim;
    ImVec2 c(x, bb.Min.y + height * 0.5f);
    draw->AddCircleFilled(c, knob_r, ImGui::GetColorU32(ImGuiCol_TextDisabled));

    if (held)
        draw->AddCircleFilled(c, knob_r * 0.85f, IM_COL32(255, 255, 255, 40));

    ImVec2 label_pos(bb.Max.x + 8.0f, bb.Min.y + (height - ImGui::GetTextLineHeight()) * 0.5f);

    const char* label_display_end = ImGui::FindRenderedTextEnd(label);
    draw->AddText(label_pos, ImGui::GetColorU32(ImGuiCol_TextDisabled), label, label_display_end);

    return clicked;
}

bool ImGui::TextSwitch(const char* label, bool* v)
{
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (!window || window->SkipItems)
        return false;

    ImGuiContext& g = *GImGui;
    ImGuiStyle& style = g.Style;

    float height = ImGui::GetFrameHeight();
    float switch_h = height;
    float switch_w = height * 2.2f * 0.85f * 1.03f;

    ImVec2 pos = window->DC.CursorPos;
    float full_width = ImGui::GetContentRegionAvail().x;

    ImRect bb(pos, ImVec2(pos.x + full_width, pos.y + height));

    ImGuiID id = window->GetID(label);

    ImGui::ItemSize(bb, style.FramePadding.y);
    if (!ImGui::ItemAdd(bb, id))
        return false;

    ImRect switch_bb(ImVec2(bb.Max.x - switch_w - style.WindowPadding.x, bb.Min.y + 1), ImVec2(bb.Max.x - style.WindowPadding.x, bb.Max.y + 1));

    bool hovered, held;
    bool clicked = ImGui::ButtonBehavior(switch_bb, id, &hovered, &held);

    if (clicked)
        *v = !*v;

    ImGuiStorage* storage = ImGui::GetStateStorage();

    float anim = storage->GetFloat(id, *v ? 1.0f : 0.0f);
    float target = *v ? 1.0f : 0.0f;

    anim = ImSmootherStep(anim, target, g.IO.DeltaTime * 12.0f);
    storage->SetFloat(id, anim);

    ImVec4 off = style.Colors[ImGuiCol_FrameBg];
    ImVec4 on = style.Colors[ImGuiCol_CheckMark];

    ImVec4 bg(off.x + (on.x - off.x) * anim, off.y + (on.y - off.y) * anim, off.z + (on.z - off.z) * anim, off.w + (on.w - off.w) * anim);
    
    ImDrawList* draw = window->DrawList;

    draw->AddRectFilled(switch_bb.Min, switch_bb.Max, ImGui::GetColorU32(bg), switch_h * 0.5f);

    float knob_r = switch_h * 0.35f;
    float padding = switch_h * 0.15f;

    float knob_x = switch_bb.Min.x + padding + knob_r + (switch_w - padding * 2 - knob_r * 2) * anim;

    draw->AddCircleFilled(ImVec2(knob_x, switch_bb.GetCenter().y), knob_r, ImGui::GetColorU32(style.Colors[ImGuiCol_TextDisabled]), 32);

    const char* end = ImGui::FindRenderedTextEnd(label);

    draw->AddText(ImVec2(bb.Min.x, bb.Min.y + (height - 1 - ImGui::GetTextLineHeight()) * 0.5f), ImGui::GetColorU32(style.Colors[ImGuiCol_Text]), label, end);

    return clicked;
}

bool ImGui::AButton(const char* label, const ImVec2& size)
{
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImGuiContext& g = *GImGui;
    ImGuiStyle& style = g.Style;

    ImGuiID id = window->GetID(label);

    ImVec2 pos = window->DC.CursorPos;
    ImVec2 button_size = ImGui::CalcItemSize(size, ImGui::CalcTextSize(label).x + style.FramePadding.x * 2.0f, ImGui::GetFrameHeight());

    ImRect bb(pos, ImVec2(pos.x + button_size.x, pos.y + button_size.y));

    ImGui::ItemSize(bb, style.FramePadding.y);
    if (!ImGui::ItemAdd(bb, id))
        return false;

    bool hovered, held;
    bool pressed = ImGui::ButtonBehavior(bb, id, &hovered, &held);

    ImGuiStorage* storage = ImGui::GetStateStorage();

    float anim = storage->GetFloat(id, 0.0f);

    float target = held ? 1.0f : hovered ? 0.6f : 0.0f;

    anim += (target - anim) * g.IO.DeltaTime * 27.5f;

    storage->SetFloat(id, anim);

    ImVec4 base = style.Colors[ImGuiCol_Button];
    ImVec4 hover = style.Colors[ImGuiCol_ButtonHovered];
    ImVec4 active = style.Colors[ImGuiCol_ButtonActive];

    ImVec4 color = base;

    if (anim <= 0.6f)
    {
        float t = anim / 0.6f;

        color.x = base.x + (hover.x - base.x) * t;
        color.y = base.y + (hover.y - base.y) * t;
        color.z = base.z + (hover.z - base.z) * t;
        color.w = base.w + (hover.w - base.w) * t;
    }
    else
    {
        float t = (anim - 0.6f) / 0.4f;

        color.x = hover.x + (active.x - hover.x) * t;
        color.y = hover.y + (active.y - hover.y) * t;
        color.z = hover.z + (active.z - hover.z) * t;
        color.w = hover.w + (active.w - hover.w) * t;
    }

    ImDrawList* draw = window->DrawList;

    draw->AddRectFilled(bb.Min, bb.Max, ImGui::GetColorU32(color), style.FrameRounding);

    ImVec2 text_size = ImGui::CalcTextSize(label);

    ImVec2 text_pos(
        bb.Min.x + (button_size.x - text_size.x) * 0.5f,
        bb.Min.y + (button_size.y - text_size.y) * 0.5f
    );

    draw->AddText(text_pos, ImGui::GetColorU32(style.Colors[ImGuiCol_Text]), label);

    return pressed;
}

struct AChildAutoSizeData
{
    float Height = 60.0f;
};

static std::unordered_map<ImGuiID, AChildAutoSizeData> g_ABeginChildSize;
static ImGuiID g_ABeginChildID = 0;

void ImGui::ABeginChild(const char* label, ImVec2 Size)
{
    ImGuiStyle& style = ImGui::GetStyle();
    ImDrawList* parent_draw = ImGui::GetWindowDrawList();

    ImGuiID id = ImGui::GetID(label);
    g_ABeginChildID = id;

    if (Size.x <= 0.0f)
        Size.x = ImGui::GetContentRegionAvail().x;

    if (Size.y <= 0.0f)
        Size.y = g_ABeginChildSize[id].Height;

    if (Size.y <= 0.0f)
        Size.y = 60.0f;


    ImVec2 child_pos = ImGui::GetCursorScreenPos();

    ImGui::BeginChild(label, Size, true);


    ImVec2 pos = ImGui::GetWindowPos();
    ImVec2 window_size = ImGui::GetWindowSize();

    ImDrawList* draw = ImGui::GetWindowDrawList();


    const char* end = ImGui::FindRenderedTextEnd(label);

    draw->AddText(ImVec2(pos.x + 18, pos.y + 13), ImGui::GetColorU32(style.Colors[ImGuiCol_TextDisabled]), label, end);


    ImVec4 c = style.Colors[ImGuiCol_ButtonActive];

    draw->AddLine(ImVec2(pos.x, pos.y + 45),ImVec2(pos.x + window_size.x, pos.y + 45), ImGui::GetColorU32(ImVec4(0.6941f, 0.6392f, 0.9490f, 1.0f)),1.0f);


    ImGui::SetCursorPosY(style.WindowPadding.y + 45);
    ImGui::SetCursorPosX(style.WindowPadding.x);
    ImGui::BeginGroup();
}

void ImGui::ABeginChildEnd()
{
    ImGuiStyle& style = ImGui::GetStyle();

    ImGui::Dummy(ImVec2(0, style.WindowPadding.y));

    ImGui::EndGroup();

    ImVec2 min = ImGui::GetItemRectMin();
    ImVec2 max = ImGui::GetItemRectMax();

    float height = (max.y - min.y) + style.WindowPadding.y + 45.f;

    if (height < 60.0f)
        height = 60.0f;

    g_ABeginChildSize[g_ABeginChildID].Height = height;

    ImGui::EndChild();

    g_ABeginChildID = 0;
}
struct SliderSmoothData
{
    float DisplayValue = 0.0f;
    bool Init = false;
};

static std::unordered_map<ImGuiID, SliderSmoothData> g_SliderSmooth;
bool ImGui::ASliderFloat(const char* label, float* value, float min, float max, float width)
{
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems) return false;

    ImGuiContext& g = *GImGui;
    ImGuiStyle& style = g.Style;
    ImGuiID id = window->GetID(label);

    ImVec2 pos = ImGui::GetCursorScreenPos();
    float text_h = ImGui::GetTextLineHeight();
    float height = 32.0f;

    float track_y = pos.y + text_h + 9.0f;
    float track_h = 6.0f;

    ImRect bb(pos, ImVec2(pos.x + width, pos.y + height));
    ImRect slider_bb(ImVec2(pos.x - 5, track_y - track_h * 0.5f - 5), ImVec2(pos.x + width + 5, track_y + track_h * 0.5f + 5));

    ImGui::ItemSize(bb);
    if (!ImGui::ItemAdd(slider_bb, id)) return false;

    bool hovered, held;
    bool pressed = ImGui::ButtonBehavior(slider_bb, id, &hovered, &held);

    static ImGuiID edit_id = 0;
    static char edit_buf[64];

    char value_text[64];
    snprintf(value_text, sizeof(value_text), "%.2f", *value);

    ImVec2 value_size = ImGui::CalcTextSize(value_text);
    ImRect value_bb(ImVec2(pos.x + width - value_size.x - 5, pos.y - 2), ImVec2(pos.x + width + 5, pos.y + text_h + 2));

    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && ImGui::IsKeyDown(ImGuiKey_ModCtrl) && value_bb.Contains(g.IO.MousePos))
    {
        edit_id = id;
        snprintf(edit_buf, sizeof(edit_buf), "%.3f", *value);
    }

    if (held && edit_id != id)
    {
        float t = ImClamp((g.IO.MousePos.x - pos.x) / width, 0.0f, 1.0f);
        *value = min + (max - min) * t;
    }

    if (edit_id == id)
    {
        ImVec2 backup_cursor = ImGui::GetCursorPos();
        ImVec2 backup_max = window->DC.CursorMaxPos;

        ImGui::SetCursorScreenPos(ImVec2(pos.x + width - value_size.x - 8, pos.y - 2));
        ImGui::SetNextItemWidth(value_size.x + 20);
        ImGui::SetKeyboardFocusHere();

        bool finish = ImGui::InputText("##slider_edit", edit_buf, sizeof(edit_buf), ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll);

        bool click_outside = ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !ImGui::IsItemHovered();

        if (finish || click_outside || ImGui::IsKeyPressed(ImGuiKey_Escape))
        {
            if (!ImGui::IsKeyPressed(ImGuiKey_Escape))
            {
                char* end;
                float v = strtof(edit_buf, &end);

                if (end != edit_buf)
                    *value = ImClamp(v, min, max);
            }

            edit_id = 0;
        }

        ImGui::SetCursorPos(backup_cursor);
        window->DC.CursorMaxPos = backup_max;
    }


    SliderSmoothData& smooth = g_SliderSmooth[id];

    if (!smooth.Init)
    {
        smooth.DisplayValue = *value;
        smooth.Init = true;
    }

    float speed = 25.0f;

    if (smooth.DisplayValue < *value)
    {
        smooth.DisplayValue += (*value - smooth.DisplayValue) * g.IO.DeltaTime * speed;

        if (smooth.DisplayValue > *value)
            smooth.DisplayValue = *value;
    }
    else if (smooth.DisplayValue > *value)
    {
        smooth.DisplayValue += (*value - smooth.DisplayValue) * g.IO.DeltaTime * speed;

        if (smooth.DisplayValue < *value)
            smooth.DisplayValue = *value;
    }

    float t = ImClamp((smooth.DisplayValue - min) / (max - min), 0.0f, 1.0f);

    ImDrawList* draw = window->DrawList;

    draw->AddText(pos, ImGui::GetColorU32(style.Colors[ImGuiCol_Text]), label);

    if (edit_id != id)
        draw->AddText(ImVec2(pos.x + width - value_size.x, pos.y), ImGui::GetColorU32(style.Colors[ImGuiCol_Text]), value_text);

    float knob_x = pos.x + width * t;

    draw->AddRectFilled(ImVec2(pos.x, track_y - track_h * 0.5f), ImVec2(pos.x + width, track_y + track_h * 0.5f), ImGui::GetColorU32(ImVec4(0.18f, 0.18f, 0.20f, 1.0f)), 3.0f);

    draw->AddRectFilled(ImVec2(pos.x, track_y - track_h * 0.5f), ImVec2(knob_x, track_y + track_h * 0.5f), ImGui::GetColorU32(ImVec4(0.6941f, 0.6392f, 0.9490f, 1.0f)), 3.0f);

    draw->AddCircleFilled(ImVec2(knob_x, track_y), 8.0f, ImGui::GetColorU32(ImVec4(0.95f, 0.95f, 0.97f, 1.0f)), 32);

    return pressed;
}
struct SliderIntSmoothData
{
    float DisplayValue = 0.0f;
    bool Init = false;
};

static std::unordered_map<ImGuiID, SliderIntSmoothData> g_SliderIntSmooth;

bool ImGui::ASliderInt(const char* label, int* value, int min, int max, float width)
{
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems) return false;

    ImGuiContext& g = *GImGui;
    ImGuiStyle& style = g.Style;
    ImGuiID id = window->GetID(label);

    ImVec2 pos = ImGui::GetCursorScreenPos();
    float text_h = ImGui::GetTextLineHeight();
    float height = 32.0f;

    float track_y = pos.y + text_h + 9.0f;
    float track_h = 6.0f;

    ImRect bb(pos, ImVec2(pos.x + width, pos.y + height));
    ImRect slider_bb(ImVec2(pos.x - 5, track_y - track_h * 0.5f - 5), ImVec2(pos.x + width + 5, track_y + track_h * 0.5f + 5));

    ImGui::ItemSize(bb);
    if (!ImGui::ItemAdd(slider_bb, id)) return false;

    bool hovered, held;
    bool pressed = ImGui::ButtonBehavior(slider_bb, id, &hovered, &held);

    static ImGuiID edit_id = 0;
    static char edit_buf[64];

    char value_text[64];
    snprintf(value_text, sizeof(value_text), "%d", *value);

    ImVec2 value_size = ImGui::CalcTextSize(value_text);
    ImRect value_bb(ImVec2(pos.x + width - value_size.x - 5, pos.y - 2), ImVec2(pos.x + width + 5, pos.y + text_h + 2));

    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && ImGui::GetIO().KeyCtrl && value_bb.Contains(g.IO.MousePos))
    {
        edit_id = id;
        snprintf(edit_buf, sizeof(edit_buf), "%d", *value);
    }

    if (held && edit_id != id)
    {
        float t = ImClamp((g.IO.MousePos.x - pos.x) / width, 0.0f, 1.0f);
        *value = (int)(min + (max - min) * t + 0.5f);
    }

    if (edit_id == id)
    {
        ImVec2 backup_cursor = ImGui::GetCursorPos();
        ImVec2 backup_max = window->DC.CursorMaxPos;

        ImGui::SetCursorScreenPos(ImVec2(pos.x + width - value_size.x - 8, pos.y - 2));
        ImGui::SetNextItemWidth(value_size.x + 20);
        ImGui::SetKeyboardFocusHere();

        bool finish = ImGui::InputText("##slider_int_edit", edit_buf, sizeof(edit_buf), ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll);

        bool click_outside = ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !ImGui::IsItemHovered();

        if (finish || click_outside || ImGui::IsKeyPressed(ImGuiKey_Escape))
        {
            if (!ImGui::IsKeyPressed(ImGuiKey_Escape))
            {
                char* end;
                int v = strtol(edit_buf, &end, 10);

                if (end != edit_buf)
                    *value = ImClamp(v, min, max);
            }

            edit_id = 0;
        }

        ImGui::SetCursorPos(backup_cursor);
        window->DC.CursorMaxPos = backup_max;
    }

    SliderIntSmoothData& smooth = g_SliderIntSmooth[id];

    if (!smooth.Init)
    {
        smooth.DisplayValue = (float)*value;
        smooth.Init = true;
    }

    float speed = 25.0f;

    smooth.DisplayValue += ((float)*value - smooth.DisplayValue) * g.IO.DeltaTime * speed;

    if (fabsf(smooth.DisplayValue - (float)*value) < 0.01f)
        smooth.DisplayValue = (float)*value;

    float t = ImClamp((smooth.DisplayValue - min) / (float)(max - min), 0.0f, 1.0f);

    ImDrawList* draw = window->DrawList;

    draw->AddText(pos, ImGui::GetColorU32(style.Colors[ImGuiCol_Text]), label);

    if (edit_id != id)
        draw->AddText(ImVec2(pos.x + width - value_size.x, pos.y), ImGui::GetColorU32(style.Colors[ImGuiCol_Text]), value_text);

    float knob_x = pos.x + width * t;

    draw->AddRectFilled(ImVec2(pos.x, track_y - track_h * 0.5f), ImVec2(pos.x + width, track_y + track_h * 0.5f), ImGui::GetColorU32(ImVec4(0.18f, 0.18f, 0.20f, 1.0f)), 3.0f);

    draw->AddRectFilled(ImVec2(pos.x, track_y - track_h * 0.5f), ImVec2(knob_x, track_y + track_h * 0.5f), ImGui::GetColorU32(ImVec4(0.6941f, 0.6392f, 0.9490f, 1.0f)), 3.0f);

    draw->AddCircleFilled(ImVec2(knob_x, track_y), 8.0f, ImGui::GetColorU32(ImVec4(0.95f, 0.95f, 0.97f, 1.0f)), 32);

    return pressed;
}
void ImGui::ASeparator(float thickness)
{
    ImGuiStyle& style = ImGui::GetStyle();
    ImGuiWindow* window = ImGui::GetCurrentWindow();

    if (window->SkipItems)
        return;

    float width = ImGui::GetWindowSize().x - (style.WindowPadding.x * 2);

    ImVec2 pos = ImGui::GetCursorScreenPos();

    ImGui::Dummy(ImVec2(width, thickness));

    window->DrawList->AddLine(
        ImVec2(pos.x, pos.y),
        ImVec2(pos.x + width, pos.y),
        ImGui::GetColorU32(style.Colors[ImGuiCol_Separator]),
        thickness
    );
}
extern ImFont* g_FontLight = nullptr;
void ImGui::TipText(const char* fmt)
{
    ImGuiStyle& style = ImGui::GetStyle();
    ImGuiWindow* window = ImGui::GetCurrentWindow();

    if (window->SkipItems)
        return;

    float width = ImGui::GetWindowSize().x - (style.WindowPadding.x * 2);

    ImVec2 pos = ImGui::GetCursorScreenPos();

    ImGui::Dummy(ImVec2(0, 8));
    window->DrawList->AddText(g_FontLight,15.f, ImVec2(pos.x, pos.y - 10), ImGui::GetColorU32(style.Colors[ImGuiCol_Text]), fmt);

}
struct ASettingData
{
    bool Open = false;
    float Alpha = 0.0f;
    ImVec2 Pos;
    ImRect Rect;
    ImRect ButtonRect;
    bool* OpenPtr = nullptr;
};

static std::unordered_map<ImGuiID, ASettingData> g_ASettingStates;


ImGuiID ImGui::ASettingButton(const char* id_str, const char* icon, bool* opened)
{
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
        return 0;

    ImGuiStyle& style = ImGui::GetStyle();

    ImGuiID id = window->GetID(id_str);


    ImVec2 pos = window->DC.CursorPos;
    ImVec2 size = ImGui::CalcTextSize(icon);

    ImRect bb(pos, ImVec2(pos.x + size.x, pos.y + size.y));;

    ImGui::ItemSize(bb);

    if (!ImGui::ItemAdd(bb, id))
        return id;

    bool hovered, held;
    bool pressed = ImGui::ButtonBehavior(bb, id, &hovered, &held);
    ImVec4 color = hovered ? style.Colors[ImGuiCol_Text] : style.Colors[ImGuiCol_TextDisabled];
    window->DrawList->AddText(bb.Min, ImGui::GetColorU32(color), icon);
    ASettingData& data = g_ASettingStates[id];

    data.ButtonRect = bb;
    data.OpenPtr = opened;

    if (pressed)
    {
        data.Open = !data.Open;
        *opened = data.Open;

        if (data.Open)
            data.Pos = ImVec2(bb.Min.x, bb.Max.y + 5.0f);
    }

    return id;
}

bool ImGui::ASettingBegin(ImGuiID id, ImVec2 size)
{
    auto it = g_ASettingStates.find(id);
    if (it == g_ASettingStates.end())
        return false;

    ASettingData& data = it->second;

    if (!data.Open)
        return false;

    ImGuiContext& g = *GImGui;

    if (g.IO.MouseClicked[ImGuiMouseButton_Left])
    {
        bool inside_window = data.Rect.Contains(g.IO.MousePos);
        bool inside_button = data.ButtonRect.Contains(g.IO.MousePos);

        if (!inside_window && !inside_button)
        {
            data.Open = false;
            if (data.OpenPtr)*data.OpenPtr = false;
            return false;
        }
    }

    ImGui::SetNextWindowPos(data.Pos, ImGuiCond_Always);
    ImGui::SetNextWindowSize(size, ImGuiCond_Always);

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings;

    bool result = ImGui::Begin("##ASettingPopup", nullptr, flags);

    if (result)
    {
        ImVec2 min = ImGui::GetWindowPos();
        data.Rect = ImRect(min, ImVec2(min.x + size.x, min.y + size.y));

        ImGui::GetWindowDrawList()->AddRect(min, ImVec2(min.x + size.x, min.y + size.y), ImGui::GetColorU32(ImVec4(0.114118f, 0.114118f, 0.121961f, 1.0f)), ImGui::GetStyle().WindowRounding, 0, 1.0f);
    }

    return result;
}

void ImGui::ASettingEnd()
{
    ImGui::End();
}

void ImGui::DrawRectShadow(ImDrawList* draw, ImVec2 rect_min, ImVec2 rect_max, float radius, ImU32 color, float rounding, int steps, float alpha_multiplier)
{
    if (radius <= 0.0f || steps <= 0) return;

    int r = (color >> IM_COL32_R_SHIFT) & 0xFF;
    int g = (color >> IM_COL32_G_SHIFT) & 0xFF;
    int b = (color >> IM_COL32_B_SHIFT) & 0xFF;

    for (int i = 0; i < steps; ++i)
    {
        float t = (float)(i + 1) / steps;

        float offset = radius * t;
        float alpha = (1.0f - t) * (1.0f - t);
        alpha *= alpha_multiplier;
        if (alpha <= 0.0f) continue;

        ImU32 layer_color = IM_COL32(r, g, b, (int)(alpha * 255.0f));

        draw->AddRectFilled(ImVec2(rect_min.x - offset, rect_min.y - offset), ImVec2(rect_max.x + offset, rect_max.y + offset), layer_color, rounding);
    }
}