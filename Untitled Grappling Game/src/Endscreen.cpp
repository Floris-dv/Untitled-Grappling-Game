#include "pch.h"
#include "Endscreen.h"
#include <imgui/imgui.h>

void Endscreen::Render()
{
	if (m_State != State::Open)
		return;

	if (ImGui::BeginPopupModal(m_EndscreenText.c_str())) {
		ImGui::Text("%s", m_EndscreenText.c_str());

		if (m_LevelCompletionTime.hours().count())
			ImGui::Text("Time: %d:%02d:%02lld:%03lld", m_LevelCompletionTime.hours().count(), m_LevelCompletionTime.minutes().count(), m_LevelCompletionTime.seconds().count(), m_LevelCompletionTime.subseconds().count());
		else
			ImGui::Text("Time: %02d:%02lld:%03lld", m_LevelCompletionTime.minutes().count(), m_LevelCompletionTime.seconds().count(), m_LevelCompletionTime.subseconds().count());

		if (ImGui::Button("Restart")) {
			m_State = State::Restart;
			ImGui::CloseCurrentPopup();
		}
		else if (ImGui::Button("Next Level")) {
			m_State = State::Next_Level;
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}
}

Endscreen::Endscreen(std::string_view levelName, std::chrono::duration<int64_t, std::milli> levelCompletiontime) : m_LevelCompletionTime(levelCompletiontime)
{
	m_EndscreenText = std::format("Completed {}!", levelName);
	ImGui::OpenPopup(m_EndscreenText.c_str());
}

Endscreen::~Endscreen()
{
	if (m_State == State::Open) {
		if (ImGui::BeginPopupModal("Finished "))
			ImGui::CloseCurrentPopup();
		m_State = State::Closed;
	}
}
