#pragma once

extern "C" double glfwGetTime(void);

class LevelTimer {
  double m_Time; // If positive: starttime, if negative: duration

public:
  void Start() { m_Time = glfwGetTime(); }
  void Stop() { m_Time = m_Time - glfwGetTime(); }

  // Returns: time. If negative, it isn't stopped yet.
  double GetTime() { return -m_Time; }
};
