#ifndef SIGNAL_UTILS_H_
#define SIGNAL_UTILS_H_

/*  
  Sets of simple signals processing routines used for a smoother user experience. 
*/

/* -------------------------------------------------------------------------- */

namespace {

/* Return the value x clamped between edge0 and edge1. */
float clamp(float x, float edge0, float edge1) {
  return min( edge1, max(edge0, x));
}

/* Return 0.0 when x is below a, 1.0 otherwhise. */
float step(float a, float x) {
  return (x < a) ? 0.0f : 1.0f;
}

/* Approximate smooth interpolation of value x from [edge0, edge1] to [0, 1]. */
float smoothstep(float edge0, float edge1, float x) {
  x = clamp((x - edge0) / (edge1 - edge0), 0.0f, 1.0f); 
  return x * x * (3.0f - 2.0f * x);
}

/* Map a value from [0, 1] to follow a centerd smoothed curve. */
float smoothcurve(float x) {
  return smoothstep(0.0f, 0.5f, x) -
         smoothstep(0.5f, 1.0f, x); 
}

/* Return the interpolated value between a and b, where x is in [0, 1]. */
float lerp(float a, float b, float x) {
  return a + x * (b - a);
}

/* Map the value of x in range [a, b] to range [c, d]. */
float mmap(float x, float a, float b, float c, float d) {
  x = (x - a) / (b - a);
  return lerp(c, d, x);
}

/* Return an absolute value before next ticks of time t_delay, in milliseconds. */
float tick(int t_delay = 1000) {
  t_delay = max(1, t_delay);
  const int elapsedTime = static_cast<int>(millis());
  return (elapsedTime % t_delay) / static_cast<float>(t_delay);
}

/* Animate a signal output to pin following a smooth curve of period t_delay, in milliseconds. */
void animateLED(int pin, int t_delay = 1000) {
  const float dt = smoothcurve(tick(t_delay));
  const int led_intensity = static_cast<int>(255 * dt);
  analogWrite(pin, led_intensity);
}

/* Returns a random floating point value in a range. */
float randf(float edge0 = 0.0f, float edge1 = 1.0f) {
  const float n = random(RAND_MAX) / static_cast<float>(RAND_MAX - 1);
  return lerp(edge0, edge1, n);
}

} // namespace

/* -------------------------------------------------------------------------- */

#endif // SIGNAL_UTILS_H_