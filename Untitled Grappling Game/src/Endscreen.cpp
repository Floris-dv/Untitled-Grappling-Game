#include "pch.h"
#include "Endscreen.h"
#include <imgui/imgui.h>

void Endscreen::Render()
{
	if (m_State != State::Open)
		return;

	if (ImGui::BeginPopupModal(m_EndscreenText.c_str())) {
		ImGui::Text(m_EndscreenText.c_str());

		if (m_LevelCompletionTime.hours().count())
			ImGui::Text("Time: %d:%02d:%02d:%03d", m_LevelCompletionTime.hours(), m_LevelCompletionTime.minutes(), m_LevelCompletionTime.seconds(), m_LevelCompletionTime.subseconds());
		else
			ImGui::Text("Time: %02d:%02d:%03d", m_LevelCompletionTime.minutes(), m_LevelCompletionTime.seconds(), m_LevelCompletionTime.subseconds());

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
