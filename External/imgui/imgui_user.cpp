/**
 * \file imgui_user.inl
 * \brief Demiurge extension file for the imgui library.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "imgui.h"
#define IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_PLACEMENT_NEW
#include "imgui_internal.h"
#include "imgui_user.h"

namespace ImGui
{

static void PushMultiItemsWidthsAndLabels(const char* labels[], int components, float w_full)
{
    // Sanity.
    if (components < 1)
        return;

    // Context.
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;
    const ImGuiStyle& style = g.Style;

    // Compute full if requested.
    if (w_full <= 0.0f)
        w_full = ImGui::CalcItemWidth();

    // First/last, then text width of last.
    const float w_item_one = ImMax(1.0f, IM_FLOOR((w_full - (style.ItemInnerSpacing.x) * (components - 1)) / (float)components));
    const float w_item_last = ImMax(1.0f, IM_FLOOR(w_full - (w_item_one + style.ItemInnerSpacing.x) * (components - 1)));
    float textx = CalcTextSize(labels[components - 1]).x + style.ItemInnerSpacing.x;

    // Last item.
    window->DC.ItemWidthStack.push_back(window->DC.ItemWidth); // Backup current width
    window->DC.ItemWidthStack.push_back(w_item_last - textx);
    window->DC.ItemWidthStack.push_back(textx);

    // Middle items.
    for (int i = components - 2; i >= 1; --i) {
        textx = CalcTextSize(labels[i]).x + style.ItemInnerSpacing.x;
        window->DC.ItemWidthStack.push_back(w_item_one - textx);
        window->DC.ItemWidthStack.push_back(textx);
    }

    // First/initial item.
    if (components > 1) {
        textx = CalcTextSize(labels[0]).x + style.ItemInnerSpacing.x;
        window->DC.ItemWidthStack.push_back(w_item_one - textx);
        window->DC.ItemWidth = textx;
    }
    g.NextItemData.Flags &= ~ImGuiNextItemDataFlags_HasWidth;
}

bool CollapsingHeaderEx(const char* label, ImTextureID button_id, bool* p_button_activate, ImGuiTreeNodeFlags flags /* = 0 */)
{
    if (nullptr != p_button_activate)
    {
        *p_button_activate = false;
    }

    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImGuiID id = window->GetID(label);
    flags |= ImGuiTreeNodeFlags_CollapsingHeader;
    if (button_id)
    {
        flags |= ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_ClipLabelForTrailingButton;
    }
    const bool bOpen = TreeNodeBehavior(id, flags, label);
    if (button_id)
    {
        ImGuiContext& g = *GImGui;

        auto const image_button_id = GetIDWithSeed("#ACTIVATE", nullptr, id);
        const ImVec2 min(
            GetWindowContentRegionMax().x + window->Pos.x - g.FontSize,
            window->DC.CursorPosPrevLine.y + g.Style.FramePadding.y);
        const ImVec2 max(min + ImVec2(g.FontSize, g.FontSize));
        const ImRect bb(min, max);
        bool hovered, held;
        const bool pressed = ButtonBehavior(
            bb,
            image_button_id,
            &hovered,
            &held,
            ImGuiButtonFlags_PressedOnClick);

        window->DrawList->AddImage(
            button_id,
            bb.Min,
            bb.Max);

        if (nullptr != p_button_activate)
        {
            *p_button_activate = pressed;
        }
    }

    return bOpen;
}

bool DragFloatNEx(
    const char* labels[],
    float* v,
    int components,
    float v_speed,
    float v_min,
    float v_max,
    const char* display_format,
    ImGuiInputTextFlags const* aflags)
{
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImGuiContext& g = *GImGui;
    bool value_changed = false;
    BeginGroup();

    PushMultiItemsWidthsAndLabels(labels, components, 0.0f);
    for (int i = 0; i < components; i++)
    {
        PushID(labels[i]);
        PushID(i);
        if (i > 0)
            SameLine(0, g.Style.ItemInnerSpacing.x);
        TextUnformatted(labels[i], FindRenderedTextEnd(labels[i]));
        SameLine(0, g.Style.ItemInnerSpacing.x);
        PopItemWidth();
        ImGuiInputTextFlags flags = (nullptr != aflags ? aflags[i] : 0);
        value_changed |= DragFloat("", &v[i], v_speed, v_min, v_max, display_format, 0, flags);
        PopItemWidth();
        PopID();
        PopID();
    }

    EndGroup();

    return value_changed;
}

bool ImageButtonEx(
    ImTextureID user_texture_id,
    const ImVec2& size,
    bool selected /* = false */,
    bool enabled /* = true */,
    const ImVec2& uv0 /* = ImVec2(0, 0) */,
    const ImVec2& uv1 /* = ImVec2(1, 1) */,
    int frame_padding /* = -1 */)
{
    ImVec4 bg_color(0, 0, 0, 0);
    auto const& style = GetStyle();
    if (!enabled)
    {
        bg_color = style.Colors[ImGuiCol_TextDisabled];
    }
    else if (selected)
    {
        bg_color = style.Colors[ImGuiCol_Button];
    }

    return ImageButton(
        user_texture_id,
        size,
        uv0,
        uv1,
        frame_padding,
        bg_color,
        ImVec4(1, 1, 1, 1),
        enabled);
}

ImageButtonAction::Enum ImageButtonWithLabel(
    ImTextureID user_texture_id,
    const ImVec2& size,
    const char* label,
    bool selected /* = false */,
    bool enabled /* = true */,
    const ImVec2& uv0 /* = ImVec2(0, 0) */,
    const ImVec2& uv1 /* = ImVec2(1, 1) */,
    int frame_padding /* = -1 */)
{
    using namespace ImageButtonAction;

    ImageButtonAction::Enum eReturnValue = kNone;

    BeginGroup();
    PushID(label);
    if (ImageButtonEx(
        user_texture_id,
        size,
        selected,
        enabled,
        uv0,
        uv1,
        frame_padding))
    {
        eReturnValue = kSelected;
    }

    if (IsItemClicked(0) && IsMouseDoubleClicked(0))
    {
        eReturnValue = kDoubleClicked;
    }

    // Drag start handling.
    if (IsItemActive() && IsMouseDragging(0))
    {
        eReturnValue = kDragging;
    }

    PushItemWidth(size.x);
    AlignTextToFramePadding();
    LabelTextEx("", label);
    PopID();
    EndGroup();

    return eReturnValue;
}

bool ImageButtonCombo(
    ImTextureID user_texture_id,
    const ImVec2& size,
    int* current_item,
    bool(*items_getter)(void* data, int idx, const char** out_text),
    void* data,
    int items_count,
    int height_in_items /*= -1*/,
    bool enabled /*= true*/,
    bool indeterminate /*= false*/)
{
    BeginGroup();

    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;
    ImGuiID id = window->GetID("##image_button_combo_popup");
    bool was_popup_opened = IsPopupOpen(id, 0);
    if (ImageButton(user_texture_id, size, ImVec2(0, 0), ImVec2(1, 1), -1, ImVec4(0, 0, 0, 0), ImVec4(1, 1, 1, 1), enabled))
    {
        OpenPopup("##image_button_combo_popup");
    }

    if (!enabled)
    {
        EndGroup();
        return false;
    }

    bool value_changed = false;
    if (BeginPopup("##image_button_combo_popup"))
    {
        Spacing();

        for (int i = 0; i < items_count; ++i)
        {
            PushID((void*)(ptrdiff_t)i);

            const bool item_selected = (i == *current_item);
            const char* item_text;
            if (!items_getter(data, i, &item_text))
            {
                item_text = "*Unknown item*";
            }

            if (Selectable(item_text, item_selected))
            {
                ClearActiveID();
                value_changed = true;
                *current_item = i;
            }

            if (item_selected && !was_popup_opened)
            {
                SetScrollHereY();
            }

            PopID();
        }
        EndPopup();
    }

    EndGroup();

    return value_changed;
}

bool IsMouseHoveringCursorRelative(const ImVec2& pos, const ImVec2& size)
{
    ImGuiWindow* window = GetCurrentWindowRead();

    ImVec2 const min(
        window->Pos.x - window->Scroll.x + pos.x,
        window->Pos.y - window->Scroll.y + pos.y);
    return IsMouseHoveringRect(min, min + size);
}

bool IsTreeNodeOpen(const char* label, ImGuiTreeNodeFlags flags /*= 0*/)
{
    ImGuiWindow* window = GetCurrentWindow();
    ImGuiID id = window->GetID(label);

    return TreeNodeBehaviorIsOpen(id, flags);
}

void LabelTextEx(const char* label, const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    LabelTextExV(label, fmt, args);
    va_end(args);
}

void LabelTextExV(const char* label, const char* fmt, va_list args)
{
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
    {
        return;
    }

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const float w = CalcItemWidth();

    const ImVec2 label_size = CalcTextSize(label, NULL, true);
    const ImRect value_bb(window->DC.CursorPos, window->DC.CursorPos + ImVec2(w, label_size.y + style.FramePadding.y * 2));
    const ImRect total_bb(window->DC.CursorPos, window->DC.CursorPos + ImVec2(w + (label_size.x > 0.0f ? style.ItemInnerSpacing.x : 0.0f), style.FramePadding.y * 2) + label_size);
    ItemSize(total_bb, style.FramePadding.y);
    if (!ItemAdd(total_bb, 0))
    {
        return;
    }

    // Render
    const char* value_text_begin = &g.TempBuffer[0];
    const char* value_text_end = value_text_begin + ImFormatStringV(g.TempBuffer.Data, g.TempBuffer.Size, fmt, args);
    RenderTextClipped(value_bb.Min, value_bb.Max, value_text_begin, value_text_end, NULL, ImVec2(0.5f, 0.5f));
    if (label_size.x > 0.0f)
    {
        RenderText(ImVec2(value_bb.Max.x + style.ItemInnerSpacing.x, value_bb.Min.y + style.FramePadding.y), label);
    }
}

bool ToolbarButton(ImTextureID texture, bool selected, bool enabled)
{
    auto const v = GetItemRectSize().y - 2.0f * GetStyle().FramePadding.y;
    return ImageButtonEx(texture, ImVec2(v, v), selected, enabled);
}

static bool TreeNodeImageBehavior(
    ImTextureID closedTexture,
    ImTextureID openTexture,
    ImGuiID id, ImGuiTreeNodeFlags flags,
    const char* label,
    const char* label_end)
{
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const bool display_frame = (flags & ImGuiTreeNodeFlags_Framed) != 0;
    const ImVec2 padding = (display_frame || (flags & ImGuiTreeNodeFlags_FramePadding)) ? style.FramePadding : ImVec2(style.FramePadding.x, ImMin(window->DC.CurrLineTextBaseOffset, style.FramePadding.y));

    if (!label_end)
        label_end = FindRenderedTextEnd(label);
    const ImVec2 label_size = CalcTextSize(label, label_end, false);

    // We vertically grow up to current line height up the typical widget height.
    const float text_base_offset_y = ImMax(0.0f, window->DC.CurrLineTextBaseOffset - padding.y); // Latch before ItemSize changes it
    const float frame_height = ImMax(ImMin(window->DC.CurrLineSize.y, g.FontSize + style.FramePadding.y * 2), label_size.y + padding.y * 2);
    const ImRect bb = ImRect(window->DC.CursorPos, ImVec2(window->Pos.x + GetContentRegionMax().x, window->DC.CursorPos.y + frame_height));
    ImRect frame_bb;
    frame_bb.Min.x = (flags & ImGuiTreeNodeFlags_SpanFullWidth) ? window->WorkRect.Min.x : window->DC.CursorPos.x;
    frame_bb.Min.y = window->DC.CursorPos.y;
    frame_bb.Max.x = window->WorkRect.Max.x;
    frame_bb.Max.y = window->DC.CursorPos.y + frame_height;
    if (display_frame)
    {
        // Framed header expand a little outside the default padding, to the edge of InnerClipRect
        // (FIXME: May remove this at some point and make InnerClipRect align with WindowPadding.x instead of WindowPadding.x*0.5f)
        frame_bb.Min.x -= IM_FLOOR(window->WindowPadding.x * 0.5f - 1.0f);
        frame_bb.Max.x += IM_FLOOR(window->WindowPadding.x * 0.5f);
    }

    const float text_offset_x = g.FontSize + (display_frame ? padding.x * 3 : padding.x * 2);           // Collapser arrow width + Spacing
    const float text_offset_y = ImMax(padding.y, window->DC.CurrLineTextBaseOffset);                    // Latch before ItemSize changes it
    const float text_width = g.FontSize + (label_size.x > 0.0f ? label_size.x + padding.x * 2 : 0.0f);  // Include collapser
    ImVec2 text_pos(window->DC.CursorPos.x + text_offset_x, window->DC.CursorPos.y + text_offset_y);
    ItemSize(ImVec2(text_width, frame_height), padding.y);

    // For regular tree nodes, we arbitrary allow to click past 2 worth of ItemSpacing
    ImRect interact_bb = frame_bb;
    if (!display_frame && (flags & (ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_SpanFullWidth)) == 0)
        interact_bb.Max.x = frame_bb.Min.x + text_width + style.ItemSpacing.x * 2.0f;

    // Store a flag for the current depth to tell if we will allow closing this node when navigating one of its child.
    // For this purpose we essentially compare if g.NavIdIsAlive went from 0 to 1 between TreeNode() and TreePop().
    // This is currently only support 32 level deep and we are fine with (1 << Depth) overflowing into a zero.
    const bool is_leaf = (flags & ImGuiTreeNodeFlags_Leaf) != 0;
    bool bOpen = TreeNodeBehaviorIsOpen(id, flags);
    if (bOpen && !g.NavIdIsAlive && (flags & ImGuiTreeNodeFlags_NavLeftJumpsBackHere) && !(flags & ImGuiTreeNodeFlags_NoTreePushOnOpen))
        window->DC.TreeJumpToParentOnPopMask |= (1 << window->DC.TreeDepth);

    bool item_add = ItemAdd(interact_bb, id);
    g.LastItemData.StatusFlags |= ImGuiItemStatusFlags_HasDisplayRect;
    g.LastItemData.DisplayRect = frame_bb;

    if (!item_add)
    {
        if (bOpen && !(flags & ImGuiTreeNodeFlags_NoTreePushOnOpen))
            TreePushOverrideID(id);
        IMGUI_TEST_ENGINE_ITEM_INFO(g.LastItemData.ID, label, g.LastItemData.StatusFlags | (is_leaf ? 0 : ImGuiItemStatusFlags_Openable) | (bOpen ? ImGuiItemStatusFlags_Opened : 0));
        return bOpen;
    }

    ImGuiButtonFlags button_flags = ImGuiTreeNodeFlags_None;
    if (flags & ImGuiTreeNodeFlags_AllowItemOverlap)
        button_flags |= ImGuiButtonFlags_AllowItemOverlap;
    if (!is_leaf)
        button_flags |= ImGuiButtonFlags_PressedOnDragDropHold;

    // We allow clicking on the arrow section with keyboard modifiers held, in order to easily
    // allow browsing a tree while preserving selection with code implementing multi-selection patterns.
    // When clicking on the rest of the tree node we always disallow keyboard modifiers.
    const float arrow_hit_x1 = (text_pos.x - text_offset_x) - style.TouchExtraPadding.x;
    const float arrow_hit_x2 = (text_pos.x - text_offset_x) + (g.FontSize + padding.x * 2.0f) + style.TouchExtraPadding.x;
    const bool is_mouse_x_over_arrow = (g.IO.MousePos.x >= arrow_hit_x1 && g.IO.MousePos.x < arrow_hit_x2);
    if (window != g.HoveredWindow || !is_mouse_x_over_arrow)
        button_flags |= ImGuiButtonFlags_NoKeyModifiers;

    // Open behaviors can be altered with the _OpenOnArrow and _OnOnDoubleClick flags.
    // Some alteration have subtle effects (e.g. toggle on MouseUp vs MouseDown events) due to requirements for multi-selection and drag and drop support.
    // - Single-click on label = Toggle on MouseUp (default, when _OpenOnArrow=0)
    // - Single-click on arrow = Toggle on MouseDown (when _OpenOnArrow=0)
    // - Single-click on arrow = Toggle on MouseDown (when _OpenOnArrow=1)
    // - Double-click on label = Toggle on MouseDoubleClick (when _OpenOnDoubleClick=1)
    // - Double-click on arrow = Toggle on MouseDoubleClick (when _OpenOnDoubleClick=1 and _OpenOnArrow=0)
    // It is rather standard that arrow click react on Down rather than Up.
    // We set ImGuiButtonFlags_PressedOnClickRelease on OpenOnDoubleClick because we want the item to be active on the initial MouseDown in order for drag and drop to work.
    if (is_mouse_x_over_arrow)
        button_flags |= ImGuiButtonFlags_PressedOnClick;
    else if (flags & ImGuiTreeNodeFlags_OpenOnDoubleClick)
        button_flags |= ImGuiButtonFlags_PressedOnClickRelease | ImGuiButtonFlags_PressedOnDoubleClick;
    else
        button_flags |= ImGuiButtonFlags_PressedOnClickRelease;

    bool selected = (flags & ImGuiTreeNodeFlags_Selected) != 0;
    const bool was_selected = selected;

    bool hovered, held;
    bool pressed = ButtonBehavior(interact_bb, id, &hovered, &held, button_flags);
    bool toggled = false;
    if (!is_leaf)
    {
        if (pressed && g.DragDropHoldJustPressedId != id)
        {
            if ((flags & (ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick)) == 0 || (g.NavActivateId == id))
                toggled = true;
            if (flags & ImGuiTreeNodeFlags_OpenOnArrow)
                toggled |= is_mouse_x_over_arrow && !g.NavDisableMouseHover; // Lightweight equivalent of IsMouseHoveringRect() since ButtonBehavior() already did the job
            if ((flags & ImGuiTreeNodeFlags_OpenOnDoubleClick) && g.IO.MouseClickedCount[0] == 2)
                toggled = true;
        }
        else if (pressed && g.DragDropHoldJustPressedId == id)
        {
            IM_ASSERT(button_flags & ImGuiButtonFlags_PressedOnDragDropHold);
            if (!bOpen) // When using Drag and Drop "hold to open" we keep the node highlighted after opening, but never close it again.
                toggled = true;
        }

        if (g.NavId == id && g.NavMoveDir == ImGuiDir_Left && bOpen)
        {
            toggled = true;
            NavMoveRequestCancel();
        }
        if (g.NavId == id && g.NavMoveDir == ImGuiDir_Right && !bOpen) // If there's something upcoming on the line we may want to give it the priority?
        {
            toggled = true;
            NavMoveRequestCancel();
        }

        if (toggled)
        {
            bOpen = !bOpen;
            window->DC.StateStorage->SetInt(id, bOpen);
            g.LastItemData.StatusFlags |= ImGuiItemStatusFlags_ToggledOpen;
        }
    }
    if (flags & ImGuiTreeNodeFlags_AllowItemOverlap)
        SetItemAllowOverlap();

    // In this branch, TreeNodeBehavior() cannot toggle the selection so this will never trigger.
    if (selected != was_selected) //-V547
        g.LastItemData.StatusFlags |= ImGuiItemStatusFlags_ToggledSelection;

    // Render
    const ImU32 text_col = GetColorU32(ImGuiCol_Text);
    ImGuiNavHighlightFlags nav_highlight_flags = ImGuiNavHighlightFlags_TypeThin;
    ImTextureID texture = (bOpen ? openTexture : closedTexture);
    const ImVec2 img_pos = bb.Min + padding + ImVec2(0.0f, text_base_offset_y);
    if (display_frame)
    {
        // Framed type
        const ImU32 bg_col = GetColorU32((held && hovered) ? ImGuiCol_HeaderActive : hovered ? ImGuiCol_HeaderHovered : ImGuiCol_Header);
        RenderFrame(frame_bb.Min, frame_bb.Max, bg_col, true, /* EXT_DEMIURGE: style.FrameRounding */style.TreeNodeRounding/* EXT_DEMIURGE: */);
        RenderNavHighlight(frame_bb, id, nav_highlight_flags);
        if (flags & ImGuiTreeNodeFlags_Bullet)
            RenderBullet(window->DrawList, ImVec2(text_pos.x - text_offset_x * 0.60f, text_pos.y + g.FontSize * 0.5f), text_col);
        else if (texture)
            window->DrawList->AddImage(texture, img_pos, img_pos + ImVec2(g.FontSize, g.FontSize));
        else // Leaf without bullet, left-adjusted text
            text_pos.x -= text_offset_x;
        if (flags & ImGuiTreeNodeFlags_ClipLabelForTrailingButton)
            frame_bb.Max.x -= g.FontSize + style.FramePadding.x;

        if (g.LogEnabled)
            LogSetNextTextDecoration("###", "###");
        RenderTextClipped(text_pos, frame_bb.Max, label, label_end, &label_size);
    }
    else
    {
        // Unframed typed for tree nodes
        if (hovered || selected)
        {
            const ImU32 bg_col = GetColorU32((held && hovered) ? ImGuiCol_HeaderActive : hovered ? ImGuiCol_HeaderHovered : ImGuiCol_Header);
            RenderFrame(frame_bb.Min, frame_bb.Max, bg_col, false);
        }
        RenderNavHighlight(frame_bb, id, nav_highlight_flags);
        if (flags & ImGuiTreeNodeFlags_Bullet)
            RenderBullet(window->DrawList, ImVec2(text_pos.x - text_offset_x * 0.5f, text_pos.y + g.FontSize * 0.5f), text_col);
        else if (texture)
            window->DrawList->AddImage(texture, img_pos, img_pos + ImVec2(g.FontSize, g.FontSize));
        if (g.LogEnabled)
            LogSetNextTextDecoration(">", NULL);
        RenderText(text_pos, label, label_end, false);
    }

    if (bOpen && !(flags & ImGuiTreeNodeFlags_NoTreePushOnOpen))
        TreePushOverrideID(id);
    IMGUI_TEST_ENGINE_ITEM_INFO(id, label, g.LastItemData.StatusFlags | (is_leaf ? 0 : ImGuiItemStatusFlags_Openable) | (bOpen ? ImGuiItemStatusFlags_Opened : 0));
    return bOpen;
}

bool TreeNodeImage(
    ImTextureID closedTexture,
    ImTextureID openTexture,
    const char* label,
    ImGuiTreeNodeFlags flags /*= 0*/)
{
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    return TreeNodeImageBehavior(
        closedTexture,
        openTexture,
        window->GetID(label),
        flags,
        label,
        nullptr);
}

bool TreeNodeImageEx(
    ImTextureID closedTexture,
    ImTextureID openTexture,
    const char* label,
    ImTextureID button_id,
    bool* p_button_activate,
    ImGuiTreeNodeFlags flags /* = 0 */)
{
    if (nullptr != p_button_activate)
    {
        *p_button_activate = false;
    }

    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImGuiID id = window->GetID(label);
    if (button_id)
    {
        flags |= ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_ClipLabelForTrailingButton;
    }
    const bool bOpen = TreeNodeImageBehavior(
        closedTexture,
        openTexture,
        window->GetID(label),
        flags,
        label,
        nullptr);

    if (button_id)
    {
        ImGuiContext& g = *GImGui;

        auto const image_button_id = GetIDWithSeed("#ACTIVATE", nullptr, id);
        const ImVec2 min(
            GetWindowContentRegionMax().x + window->Pos.x - g.FontSize,
            window->DC.CursorPosPrevLine.y);
        const ImVec2 max(min + ImVec2(g.FontSize, g.FontSize));
        const ImRect bb(min, max);
        bool hovered, held;
        const bool pressed = ButtonBehavior(
            bb,
            image_button_id,
            &hovered,
            &held,
            ImGuiButtonFlags_PressedOnClick);

        window->DrawList->AddImage(
            button_id,
            bb.Min,
            bb.Max);

        if (nullptr != p_button_activate)
        {
            *p_button_activate = pressed;
        }
    }

    return bOpen;
}

void MenuBarImage(ImTextureID texture, const ImVec2& vSize)
{
    ImGuiWindow* pWindow = GetCurrentWindow();
    if (pWindow->SkipItems)
        return;

    const ImVec2 min(pWindow->DC.CursorPos.x, pWindow->DC.CursorPos.y + pWindow->DC.CurrLineTextBaseOffset);
    const ImVec2 max(min + vSize);
    const ImRect bb(min, max);
    ItemSize(bb);
    if (!ItemAdd(bb, 0))
        return;

    pWindow->DrawList->AddImage(
        texture,
        bb.Min,
        bb.Max);
}

} // namespace ImGui
