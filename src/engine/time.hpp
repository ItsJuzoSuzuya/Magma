namespace Magma {

class Time {
public:
  static void update(double now) {
      deltaTime = lastTime == 0.0 ? 0.0f : static_cast<float>(now - lastTime);
      lastTime = now;
  }

  static double getDeltaTime() {
      return deltaTime;
  }

private:
  inline static double lastTime = 0.0;
  inline static float deltaTime = 0.0f;

};

}
