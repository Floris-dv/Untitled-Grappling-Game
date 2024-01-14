#include "pch.h"

#include "Application.h"
#include "Log.h"
#include "RigidBody.h"
#include "Test.h"
#include "Window.h"

constexpr bool ShouldLog(TestLoggingOptions options, unsigned int depth,
                         bool success) {
  if (success)
    return options.successes >= depth;
  return options.failures >= depth;
}
constexpr bool ShouldLogVerbose(TestLoggingOptions options, unsigned int depth,
                                bool success) {
  if (success)
    return options.verbosesuccesses >= depth;
  return options.verbosefailures >= depth;
}

// Test a single function, very ugly templates, but that's c++
// testFunction: the function to test, gets passed in options,
// depth, and all the additional arguments, must return bool
// options: self-explanatory depth: the depth of the function: 0 is
// testall, 1 is for the main tests, 2 is for the subtests, 3 is for the
// sub-subtests, etc. name: name of the test args: additional arguments that
// get passed to testFunction
template <typename F, typename... AdditionalArgs>
std::enable_if_t<std::is_invocable_r_v<bool, F, TestLoggingOptions,
                                       unsigned int, AdditionalArgs...>,
                 bool>
Test(F testFunction, const char *name, TestLoggingOptions options,
     unsigned int depth, AdditionalArgs... args) {

  if (ShouldLogVerbose(options, depth, true)) {
    NG_INFO("Starting test {}", name);
  }
  bool result = [&]() {
    try {
      return testFunction(options, depth, args...);
    } catch (std::exception &e) {
      NG_ERROR("{}", e.what());
      return false;
    }
  }();

  if (result) {
    if (ShouldLog(options, depth, true))
      NG_INFO("Test {} succeeded", name);
  } else if (ShouldLog(options, depth, false))
    NG_WARN("Test {} failed", name);

  return result;
}

bool TestAll(TestLoggingOptions options, bool continueIfFailed) {
  bool result = true;

  // helper macro
#define TEST(testFunction)                                                     \
  do {                                                                         \
    bool tmp =                                                                 \
        Test(testFunction, #testFunction, options, 1, continueIfFailed);       \
    result &= tmp;                                                             \
    if (!tmp && !continueIfFailed)                                             \
      return false;                                                            \
  } while (0)

  TEST(TestPhysics);
  TEST(TestWindow);
  TEST(TestAudioSystem);

  return result;

#undef TEST
}

bool TestIntersection(TestLoggingOptions options, unsigned int depth,
                      const Collider *c1, const Collider *c2, bool expected,
                      const char *name);

bool TestPhysics(TestLoggingOptions options, unsigned int depth,
                 bool continueIfFailed) {
  bool result = true;

  /*
  HitInfo hi = Collide(&S1, &S2);                                            \
  if (0 && hi.hit) \
    NG_INFO("{} {} {}", hi.normal.x, hi.normal.y, hi.normal.z);              \
  */
#define TEST_INTERSECTION(name, object1, object2, expected)                    \
  do {                                                                         \
    bool tmp = Test(TestIntersection, name, options, depth + 1, &object1,      \
                    &object2, expected, name);                                 \
    result &= tmp;                                                             \
    if (!tmp && !continueIfFailed)                                             \
      return false;                                                            \
  } while (0)

  SphereCollider S1({0.0f, 0.0f, 0.0f}, 2.0f);
  SphereCollider S2({1.0f, 0.0f, 0.0f}, 2.0f);
  SphereCollider S3({5.0f, 0.0f, 0.0f}, 2.0f);
  BoxCollider B1({2.5f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f},
                 {1.0f, 0.0f, 0.0f, 0.0f});
  BoxCollider B2({4.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f},
                 {1.0f, 0.0f, 0.0f, 0.0f});
  BoxCollider B3({6.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f},
                 {1.0f, 0.0f, 0.0f, 0.0f});

  TEST_INTERSECTION("Two balls", S1, S2, true);
  TEST_INTERSECTION("Two apart balls", S1, S3, false);
  TEST_INTERSECTION("Two touching balls", S2, S3, true);
  TEST_INTERSECTION("Two boxes", B1, B2, true);
  TEST_INTERSECTION("Two apart boxes", B1, B3, false);
  TEST_INTERSECTION("Two touching boxes", B2, B3, true);
  TEST_INTERSECTION("Sphere and box", S1, B1, true);
  TEST_INTERSECTION("Offset sphere and box", S3, B3, true);
  TEST_INTERSECTION("Apart sphere and box", S1, B3, false);
  TEST_INTERSECTION("Touching sphere and box", S2, B2, true);

  return result;
}
#undef TEST_INTERSECTION

bool TestWindow(TestLoggingOptions options, unsigned int depth,
                [[maybe_unused]] bool continueIfFailed) {
  return Test(
      [](...) {
        Window w(WindowProps{});
        w.PollSwap();
        w.Maximize();
        w.PollSwap();
        return true;
      },
      "Window", options, depth + 1);
}

bool TestAudioSystem(TestLoggingOptions options, unsigned int depth,
                     [[maybe_unused]] bool continueIfFailed) {
  std::unique_ptr<AudioSystem> a;
  try {
    a = std::make_unique<AudioSystem>(nullptr);
  } catch ([[maybe_unused]] const std::exception &e) {
    if (ShouldLog(options, depth, false)) {
      NG_WARN("Could not create audiosystem");
      return false;
    }
  }
  if (!Test(
          []([[maybe_unused]] TestLoggingOptions options,
             [[maybe_unused]] unsigned int depth, AudioSystem *a) {
            a->AddSound("resources/my_sound.wav", glm::vec3{0.0f});
            return true;
          },
          "Adding sound", options, depth + 1, a.get())) {
    return false;
  }

  Test(
      []([[maybe_unused]] TestLoggingOptions options,
         [[maybe_unused]] unsigned int depth, AudioSystem *a) {
        for (float i = 0; i < 100.0f; i++) {
          float angle = i / 100.0f * glm::two_pi<float>();
          glm::vec3 pos{glm::cos(angle), 0.f, glm::sin(angle)};
          std::this_thread::sleep_for(std::chrono::milliseconds(10));
          a->SetListenerOptions(pos, glm::vec3{0.0f}, glm::vec3{0.0f});
        }
        return true;
      },
      "Position sound system", options, depth + 1, a.get());

  Test(
      []([[maybe_unused]] TestLoggingOptions options,
         [[maybe_unused]] unsigned int depth, AudioSystem *a) {
        for (float i = 0; i < 100.0f; i++) {
          float angle = i / 100.0f * glm::two_pi<float>();
          std::this_thread::sleep_for(std::chrono::milliseconds(10));
          a->SetListenerOptions(
              glm::vec3{1.0f, 0.0f, 0.0f}, glm::vec3{0.0f},
              glm::vec3{glm::cos(angle), 0.0f, glm::sin(angle)});
        }
        return true;
      },
      "Direction sound system", options, depth + 1, a.get());

  Test(
      []([[maybe_unused]] TestLoggingOptions options,
         [[maybe_unused]] unsigned int depth, AudioSystem *a) {
        for (float i = 0; i < 100.0f; i++) {
          std::this_thread::sleep_for(std::chrono::milliseconds(10));
          a->SetListenerOptions(glm::vec3{1.0f, 0.0f, 0.0f},
                                glm::vec3{i, 0.0f, 0.0f}, glm::vec3{0.0f});
        }
        for (float i = 100.0f; i > -100.0f; i--) {
          std::this_thread::sleep_for(std::chrono::milliseconds(10));
          a->SetListenerOptions(glm::vec3{1.0f, 0.0f, 0.0f},
                                glm::vec3{i, 0.0f, 0.0f}, glm::vec3{0.0f});
        }
        return true;
      },
      "Velocity sound system", options, depth + 1, a.get());

  a->SetListenerOptions(glm::vec3{0.0f}, glm::vec3{0.0f}, glm::vec3{0.0f});

  return true;
}

bool TestIntersection(TestLoggingOptions options, unsigned int depth,
                      const Collider *c1, const Collider *c2, bool expected,
                      const char *name) {
  HitInfo h1 = Collide(c1, c2);
  HitInfo h2 = Collide(c2, c1);
  bool t1 = h1.Hit;
  bool t2 = h2.Hit;
  const bool result = t1 == expected && t2 == expected;

  if (result) {
    if (ShouldLogVerbose(options, depth, true)) {
      NG_INFO("Intersection {} succeeded: {} == {} == {} (expected)", name, t1,
              t2, expected);
      if (t1)
        NG_INFO("Normals: ({} {} {}) ({} {} {})", h1.Normal.x, h1.Normal.y,
                h1.Normal.z, h2.Normal.x, h2.Normal.y, h2.Normal.z);
    }
  } else {
    if (ShouldLogVerbose(options, depth, false)) {
      if (t1 != expected) {
        NG_INFO("Intersection {} failed, expected {}, got {}", name, expected,
                t1);
      }
      if (t2 != expected) {
        NG_INFO("Intersection {} failed, expected {}, got {}", name, expected,
                t2);
      }
    }
  }

  return result;
}
