#pragma once

/* -------------------------------------------------------------------------- */

float clamp(float x, float edge0, float edge1) {
  return min( edge1, max(edge0, x));
}

float smoothstep(float edge0, float edge1, float x) {
  x = clamp((x - edge0) / (edge1 - edge0), 0.0f, 1.0f); 
  return x * x * (3.0f - 2.0f * x);
}

float smoothcurve(float x) {
  return smoothstep(0.0f, 0.5f, x) -
         smoothstep(0.5f, 1.0f, x); 
}

float lerp(float a, float b, float x) {
  return a + x*(b-a);
}

float mmap(float x, float a, float b, float c, float d) {
  x = (x - a) / (b - a);
  return lerp(c, d, x);
}

/* -------------------------------------------------------------------------- */

float tick(float delay = 1000.0f) {
  return fmodf(millis(), delay) / delay;
}

/* -------------------------------------------------------------------------- */

void animateLED(int pin, float delay=1000.0f) {
  analogWrite(pin, int(255 * smoothcurve(tick(delay))));
}

void animateLED2(int pin, float delay) {
  float x = fmodf(millis(), delay) / delay;
  float sx = 16.f*x*x*(x-1.f)*(x-1.f);
  analogWrite(pin, int(255 * sx));
}

/* -------------------------------------------------------------------------- */