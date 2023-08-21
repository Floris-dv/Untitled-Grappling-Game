#pragma once
class Endscreen {
public:
  enum class State {
    Closed,
    Open,
    Restart,
    Next_Level,
  };

private:
  std::string m_EndscreenText;
  std::chrono::hh_mm_ss<std::chrono::duration<int32_t, std::milli>>
      m_LevelCompletionTime;
  State m_State = State::Open;

public:
  void Render();
  Endscreen() : m_State{State::Closed} {}
  Endscreen(std::string_view levelName,
            std::chrono::duration<int32_t, std::milli> levelCompletiontime);
  ~Endscreen();

  State GetState() { return m_State; }
  void Close() { m_State = State::Closed; }
};
