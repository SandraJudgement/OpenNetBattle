#pragma once

#include "bnAudioType.h"

#include <SFML/Audio/SoundBuffer.hpp>
#include <SFML/Audio/Sound.hpp>
#include <SFML/Audio/Music.hpp>
#include <atomic>
#include <mutex>

// For more retro experience, decrease available channels.
#define NUM_OF_CHANNELS 10

// Prevent duplicate sounds from stacking on same frame
// Allows duplicate Audio() samples to play in X ms apart from eachother
#define Audio_DUPLICATES_ALLOWED_IN_X_MILLISECONDS 58 // 58ms = ~3.5 frames

/**
  * @class AudioPriority
  * @brief Each priority describes how or if a playing sample should be interrupted
  * 
  * Priorities are LOWEST  (one at a time, if channel available),
  *                LOW     (any free channels),
  *                HIGH    (force a channel to play sound, but one at a time, and don't interrupt other high priorities),
  *                HIGHEST (force a channel to play sound always)
  */
enum class AudioPriority : int {
  lowest,
  low,
  high,
  highest
};

/**
 * @class AudioResourceManager
 * @author mav
 * @date 06/05/19
 * @brief Manager loads Audio() samples
 */
class AudioResourceManager {
public:
  /**
   * @brief If true, plays Audio(). If false, does not play Audio()
   * @param status
   */
  void EnableAudio(bool status);
  
  /**
   * @brief Loads all queued resources. Increases status value.
   * @param status thread-safe counter will reach total count of all samples to load when finished.
   */
  void LoadAllSources(std::atomic<int> &status);
  
  /**
   * @brief Loads an Audio() source at path and map it to enum type
   * @param type Audio() enum to map to
   * @param path path to Audio() sample
   */
  void LoadSource(AudioType type, const std::string& path);
  
  /**
   * @brief Play a sound with an Audio() priority
   * @param type Audio() to play
   * @param priority describes if and how to interrupt other playing samples
   * @return -1 if could not play, otherwise 0
   */
  int Play(AudioType type, AudioPriority priority = AudioPriority::low);
  int Stream(std::string path, bool loop = false, sf::Music::TimeSpan span = sf::Music::TimeSpan());
  void StopStream();
  void SetStreamVolume(float volume);
  void SetChannelVolume(float volume);

  AudioResourceManager();
  ~AudioResourceManager();

private:
  struct Channel {
    sf::Sound buffer;
    AudioPriority priority;
  };

  std::mutex mutex;
  Channel* channels;
  sf::SoundBuffer* sources;
  sf::Music stream;
  float channelVolume;
  float streamVolume;
  bool isEnabled;
};