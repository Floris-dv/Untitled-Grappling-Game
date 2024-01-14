#include "pch.h"

#include "Endscreen.h"
#include "spdlog/fmt/fmt.h"
#include <imgui/imgui.h>

void Endscreen::Render() {

  if (ImGui::BeginPopupModal(m_EndscreenText.c_str())) {
    if (m_State != State::Open) {
      ImGui::CloseCurrentPopup();
      ImGui::EndPopup();
      return;
    }
    ImGui::Text("%s", m_EndscreenText.c_str());

    if (m_LevelCompletionTime.hours().count())
      ImGui::Text("Time: %d:%02d:%02lld:%03lld",
                  m_LevelCompletionTime.hours().count(),
                  m_LevelCompletionTime.minutes().count(),
                  m_LevelCompletionTime.seconds().count(),
                  m_LevelCompletionTime.subseconds().count());
    else
      ImGui::Text("Time: %02d:%02lld:%03lld",
                  m_LevelCompletionTime.minutes().count(),
                  m_LevelCompletionTime.seconds().count(),
                  m_LevelCompletionTime.subseconds().count());

    if (ImGui::Button("Restart")) {
      m_State = State::Restart;
      ImGui::CloseCurrentPopup();
    } else if (ImGui::Button("Next Level")) {
      m_State = State::Next_Level;
      ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
  }
}

Endscreen::Endscreen(
    std::string_view levelName,
    std::chrono::duration<int32_t, std::milli> levelCompletiontime)
    : m_LevelCompletionTime(levelCompletiontime) {
  m_EndscreenText = fmt::format("Completed {}!", levelName);
  ImGui::OpenPopup(m_EndscreenText.c_str());
}

Endscreen::~Endscreen() { Close(); }
