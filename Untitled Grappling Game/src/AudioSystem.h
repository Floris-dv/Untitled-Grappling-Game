#include <miniaudio/miniaudio.h>

class AudioSystem {
  ma_engine m_Engine;
  static constexpr uint32_t m_ListenerIndex = 0;
  std::vector<ma_sound> m_Sounds;

public:
  explicit AudioSystem(ma_engine_config *config); // just set this to nullptr to
                                                  // actually construct this
  void SetListenerOptions(const glm::vec3 &position,
                          const glm::vec3 &direction);

  void AddSound(std::string_view fileName, const glm::vec3 &position);

  ~AudioSystem();
};
