//
//  vector3d.c
//  trajectory
//
//  Created by Thomas Malthouse on 6/28/16.
//  Copyright © 2016 Thomas Malthouse. All rights reserved.
//

#include "vector3d.h"
#include <tgmath.h>

#define SQ(x) (x * x)

static inline bool fp_eq(double a, double b) {
  return fabs(a - b) < 0.1 ? true : false;
}

bool v3d_equal(Vector3d a, Vector3d b) {
  if (a.x == b.x && a.y == b.y && a.z == b.z) {
    return true;
  }
  return false;
}

bool v3d_fp_eq(Vector3d a, Vector3d b) {
  if (fp_eq(a.x, b.x) && fp_eq(a.y, b.y) && fp_eq(a.z, b.z)) {
    return true;
  }
  return false;
}

double v3d_abs(Vector3d v) { return v3d_absdist(v, V3D_0_VECTOR); }

double v3d_absdist(Vector3d a, Vector3d b) {
  Vector3d diff = v3d_vdiff(a, b);
  return sqrt(SQ(diff.x) + SQ(diff.y) + SQ(diff.z));
}

Vector3d v3d_vdiff(Vector3d a, Vector3d b) {
  return (Vector3d){a.x - b.x, a.y - b.y, a.z - b.z};
}

Vector3d v3d_vsum(Vector3d a, Vector3d b) {
#ifdef OPENCL_VECTORS
  return a + b;
#else
  return (Vector3d){a.x + b.x, a.y + b.y, a.z + b.z};
#endif
}

Vector3d v3d_fmult(Vector3d a, double f) {
#ifdef OPENCL_VECTORS
  return a * f;
#else
  return (Vector3d){a.x * f, a.y * f, a.z * f};
#endif
}

double v3d_dotprod(Vector3d a, Vector3d b) {
  return a.x * b.x + a.y * b.y + a.z * b.z;
}

Vector3d v3d_xprod(Vector3d a, Vector3d b) {
  return (Vector3d){a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z,
                    a.x * b.y - a.y * b.x};
}

double v3d_vector_angle(Vector3d a, Vector3d b) {
  return acos(v3d_dotprod(a, b) / (v3d_abs(a) * v3d_abs(b)));
}

Vector3d v3d_unit_vector(Vector3d a) {
  double abs = v3d_abs(a);

  // Divide each quantity by the magnitude
  return v3d_fmult(a, 1 / abs);
}

Vector3d v3d_asum(Vector3d *vectors, uint64_t count) {
  if (count == 0) {
    return V3D_0_VECTOR;
  }
  return v3d_vsum(vectors[0], v3d_asum(vectors + 1, count - 1));
}
