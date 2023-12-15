#include "pch.h"

#include "AudioSystem.h"
#include "Log.h"

AudioSystem::AudioSystem(ma_engine_config *config) {
  NG_TRACE("Initializing audio system");
  ma_result result = ma_engine_init(config, &m_Engine);
  if (result != MA_SUCCESS) {
    NG_CRITICAL("Failed to initialize audio system");
    throw "Failed to initialize audio system!";
  }
}

void AudioSystem::SetListenerOptions(const glm::vec3 &position,
                                     const glm::vec3 &direction) {
  if (!s_UsePosition)
    return;
  ma_engine_listener_set_position(&m_Engine, m_ListenerIndex, position.x,
                                  position.y, position.z);
  ma_engine_listener_set_direction(&m_Engine, m_ListenerIndex, direction.x,
                                   direction.y, direction.z);
}

void AudioSystem::AddSound(std::string_view fileName,
                           const glm::vec3 &position) {
  NG_TRACE("Adding Sound {}", fileName);
  m_Sounds.push_back({});
  ma_result result = ma_sound_init_from_file(
      &m_Engine, fileName.data(), 0, nullptr, nullptr, &m_Sounds.back());

  if (result != MA_SUCCESS) {
    NG_ERROR("Failed to load sound {}", fileName.data());
    throw "Failed to initialize sound";
  }

  if (s_UsePosition)
    ma_sound_set_position(&m_Sounds.back(), position.x, position.y, position.z);

  ma_sound_start(&m_Sounds.back());
}

AudioSystem::~AudioSystem() {
  for (ma_sound &sound : m_Sounds)
    ma_sound_stop(&sound);
  ma_engine_uninit(&m_Engine);
}
