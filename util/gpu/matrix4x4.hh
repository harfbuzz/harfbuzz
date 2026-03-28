#ifndef MATRIX4X4_HH
#define MATRIX4X4_HH

#include <cmath>
#include <cstring>

/*
 * Copyright (c) 2009, Mozilla Corp
 * Copyright (c) 2012, Google, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the <organization> nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY <copyright holder> ''AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL <copyright holder> BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Based on sample code from the OpenGL(R) ES 2.0 Programming Guide, which carriers
 * the following header:
 *
 * Book:      OpenGL(R) ES 2.0 Programming Guide
 * Authors:   Aaftab Munshi, Dan Ginsburg, Dave Shreiner
 * ISBN-10:   0321502795
 * ISBN-13:   9780321502797
 * Publisher: Addison-Wesley Professional
 * URLs:      http://safari.informit.com/9780321563835
 *            http://www.opengles-book.com
 */

/*
 * Ported from JavaScript to C by Behdad Esfahbod, 2012.
 * Added MultMatrix.  Converting from fixed-function OpenGL matrix
 * operations to these functions should be as simple as renaming the
 * 'gl' prefix to 'm4' and adding the matrix argument to the call.
 *
 * The C version lives at http://code.google.com/p/matrix4x4-c/
 */



/*
 * A simple 4x4 matrix utility implementation
 */


static inline float *
m4LoadIdentity (float *mat) {
  unsigned int i;
  for (i = 0; i < 16; i++)
    mat[i] = 0;
  mat[0*4+0] = 1.0;
  mat[1*4+1] = 1.0;
  mat[2*4+2] = 1.0;
  mat[3*4+3] = 1.0;
  return mat;
}

/* Copies other matrix into mat */
static inline float *
m4Copy (float *mat, const float *other) {
  unsigned int i;
  for (i = 0; i < 16; i++) {
    mat[i] = other[i];
  }
  return mat;
}

static inline float *
m4Multiply (float *mat, const float *right) {
  float tmp[16];
  unsigned int i;

  for (i = 0; i < 4; i++) {
    tmp[i*4+0] =
      (mat[i*4+0] * right[0*4+0]) +
      (mat[i*4+1] * right[1*4+0]) +
      (mat[i*4+2] * right[2*4+0]) +
      (mat[i*4+3] * right[3*4+0]) ;

    tmp[i*4+1] =
      (mat[i*4+0] * right[0*4+1]) +
      (mat[i*4+1] * right[1*4+1]) +
      (mat[i*4+2] * right[2*4+1]) +
      (mat[i*4+3] * right[3*4+1]) ;

    tmp[i*4+2] =
      (mat[i*4+0] * right[0*4+2]) +
      (mat[i*4+1] * right[1*4+2]) +
      (mat[i*4+2] * right[2*4+2]) +
      (mat[i*4+3] * right[3*4+2]) ;

    tmp[i*4+3] =
      (mat[i*4+0] * right[0*4+3]) +
      (mat[i*4+1] * right[1*4+3]) +
      (mat[i*4+2] * right[2*4+3]) +
      (mat[i*4+3] * right[3*4+3]) ;
  }

  return m4Copy (mat, tmp);
}

static inline float
m4Get (float *mat, unsigned int row, unsigned int col) {
  return mat[4*row+col];
}

static inline float *
m4MultMatrix (float *mat, const float *left) {
  float tmp[16];
  return m4Copy (mat, m4Multiply (m4Copy (tmp, left), mat));
}

static inline float *
m4Scale (float *mat, float sx, float sy, float sz) {
  mat[0*4+0] *= sx;
  mat[0*4+1] *= sx;
  mat[0*4+2] *= sx;
  mat[0*4+3] *= sx;

  mat[1*4+0] *= sy;
  mat[1*4+1] *= sy;
  mat[1*4+2] *= sy;
  mat[1*4+3] *= sy;

  mat[2*4+0] *= sz;
  mat[2*4+1] *= sz;
  mat[2*4+2] *= sz;
  mat[2*4+3] *= sz;

  return mat;
}

static inline float *
m4Translate (float *mat, float tx, float ty, float tz) {
  mat[3*4+0] += mat[0*4+0] * tx + mat[1*4+0] * ty + mat[2*4+0] * tz;
  mat[3*4+1] += mat[0*4+1] * tx + mat[1*4+1] * ty + mat[2*4+1] * tz;
  mat[3*4+2] += mat[0*4+2] * tx + mat[1*4+2] * ty + mat[2*4+2] * tz;
  mat[3*4+3] += mat[0*4+3] * tx + mat[1*4+3] * ty + mat[2*4+3] * tz;

  return mat;
}

static inline float *
m4Rotate (float *mat, float angle, float x, float y, float z) {
  float mag = sqrt(x*x + y*y + z*z);
  float sinAngle = sin(angle * M_PI / 180.0);
  float cosAngle = cos(angle * M_PI / 180.0);

  float xx, yy, zz, xy, yz, zx, xs, ys, zs;
  float oneMinusCos;

  float rotMat[16];

  if (mag <= 0)
    return mat;

  m4LoadIdentity (rotMat);

  x /= mag;
  y /= mag;
  z /= mag;

  xx = x * x;
  yy = y * y;
  zz = z * z;
  xy = x * y;
  yz = y * z;
  zx = z * x;
  xs = x * sinAngle;
  ys = y * sinAngle;
  zs = z * sinAngle;
  oneMinusCos = 1.0 - cosAngle;

  rotMat[0*4+0] = (oneMinusCos * xx) + cosAngle;
  rotMat[0*4+1] = (oneMinusCos * xy) - zs;
  rotMat[0*4+2] = (oneMinusCos * zx) + ys;
  rotMat[0*4+3] = 0.0;

  rotMat[1*4+0] = (oneMinusCos * xy) + zs;
  rotMat[1*4+1] = (oneMinusCos * yy) + cosAngle;
  rotMat[1*4+2] = (oneMinusCos * yz) - xs;
  rotMat[1*4+3] = 0.0;

  rotMat[2*4+0] = (oneMinusCos * zx) - ys;
  rotMat[2*4+1] = (oneMinusCos * yz) + xs;
  rotMat[2*4+2] = (oneMinusCos * zz) + cosAngle;
  rotMat[2*4+3] = 0.0;

  rotMat[3*4+0] = 0.0;
  rotMat[3*4+1] = 0.0;
  rotMat[3*4+2] = 0.0;
  rotMat[3*4+3] = 1.0;

  return m4Copy (mat, m4Multiply (rotMat, mat));
}

static inline float *
m4Frustum (float *mat, float left, float right, float bottom, float top, float nearZ, float farZ) {
  float deltaX = right - left;
  float deltaY = top - bottom;
  float deltaZ = farZ - nearZ;

  float frust[16];

  if ( (nearZ <= 0.0) || (farZ <= 0.0) ||
       (deltaX <= 0.0) || (deltaY <= 0.0) || (deltaZ <= 0.0) )
       return mat;

  m4LoadIdentity (frust);

  frust[0*4+0] = 2.0 * nearZ / deltaX;
  frust[0*4+1] = frust[0*4+2] = frust[0*4+3] = 0.0;

  frust[1*4+1] = 2.0 * nearZ / deltaY;
  frust[1*4+0] = frust[1*4+2] = frust[1*4+3] = 0.0;

  frust[2*4+0] = (right + left) / deltaX;
  frust[2*4+1] = (top + bottom) / deltaY;
  frust[2*4+2] = -(nearZ + farZ) / deltaZ;
  frust[2*4+3] = -1.0;

  frust[3*4+2] = -2.0 * nearZ * farZ / deltaZ;
  frust[3*4+0] = frust[3*4+1] = frust[3*4+3] = 0.0;

  return m4Copy (mat, m4Multiply (frust, mat));
}

static inline float *
m4Perspective (float *mat, float fovy, float aspect, float nearZ, float farZ) {
  float frustumH = tan(fovy / 360.0 * M_PI) * nearZ;
  float frustumW = frustumH * aspect;

  return m4Frustum(mat, -frustumW, frustumW, -frustumH, frustumH, nearZ, farZ);
}

static inline float *
m4Ortho (float *mat, float left, float right, float bottom, float top, float nearZ, float farZ) {
  float deltaX = right - left;
  float deltaY = top - bottom;
  float deltaZ = farZ - nearZ;

  float ortho[16];

  if ( (deltaX == 0.0) || (deltaY == 0.0) || (deltaZ == 0.0) )
    return mat;

  m4LoadIdentity (ortho);

  ortho[0*4+0] = 2.0 / deltaX;
  ortho[3*4+0] = -(right + left) / deltaX;
  ortho[1*4+1] = 2.0 / deltaY;
  ortho[3*4+1] = -(top + bottom) / deltaY;
  ortho[2*4+2] = -2.0 / deltaZ;
  ortho[3*4+2] = -(nearZ + farZ) / deltaZ;

  return m4Copy (mat, m4Multiply (ortho, mat));
}

/* In-place inversion */
static inline float *
m4Invert (float *mat) {
  float tmp_0 = m4Get(mat,2,2) * m4Get(mat,3,3);
  float tmp_1 = m4Get(mat,3,2) * m4Get(mat,2,3);
  float tmp_2 = m4Get(mat,1,2) * m4Get(mat,3,3);
  float tmp_3 = m4Get(mat,3,2) * m4Get(mat,1,3);
  float tmp_4 = m4Get(mat,1,2) * m4Get(mat,2,3);
  float tmp_5 = m4Get(mat,2,2) * m4Get(mat,1,3);
  float tmp_6 = m4Get(mat,0,2) * m4Get(mat,3,3);
  float tmp_7 = m4Get(mat,3,2) * m4Get(mat,0,3);
  float tmp_8 = m4Get(mat,0,2) * m4Get(mat,2,3);
  float tmp_9 = m4Get(mat,2,2) * m4Get(mat,0,3);
  float tmp_10 = m4Get(mat,0,2) * m4Get(mat,1,3);
  float tmp_11 = m4Get(mat,1,2) * m4Get(mat,0,3);
  float tmp_12 = m4Get(mat,2,0) * m4Get(mat,3,1);
  float tmp_13 = m4Get(mat,3,0) * m4Get(mat,2,1);
  float tmp_14 = m4Get(mat,1,0) * m4Get(mat,3,1);
  float tmp_15 = m4Get(mat,3,0) * m4Get(mat,1,1);
  float tmp_16 = m4Get(mat,1,0) * m4Get(mat,2,1);
  float tmp_17 = m4Get(mat,2,0) * m4Get(mat,1,1);
  float tmp_18 = m4Get(mat,0,0) * m4Get(mat,3,1);
  float tmp_19 = m4Get(mat,3,0) * m4Get(mat,0,1);
  float tmp_20 = m4Get(mat,0,0) * m4Get(mat,2,1);
  float tmp_21 = m4Get(mat,2,0) * m4Get(mat,0,1);
  float tmp_22 = m4Get(mat,0,0) * m4Get(mat,1,1);
  float tmp_23 = m4Get(mat,1,0) * m4Get(mat,0,1);

  float t0 = ((tmp_0 * m4Get(mat,1,1) + tmp_3 * m4Get(mat,2,1) + tmp_4 * m4Get(mat,3,1)) -
            (tmp_1 * m4Get(mat,1,1) + tmp_2 * m4Get(mat,2,1) + tmp_5 * m4Get(mat,3,1)));
  float t1 = ((tmp_1 * m4Get(mat,0,1) + tmp_6 * m4Get(mat,2,1) + tmp_9 * m4Get(mat,3,1)) -
            (tmp_0 * m4Get(mat,0,1) + tmp_7 * m4Get(mat,2,1) + tmp_8 * m4Get(mat,3,1)));
  float t2 = ((tmp_2 * m4Get(mat,0,1) + tmp_7 * m4Get(mat,1,1) + tmp_10 * m4Get(mat,3,1)) -
            (tmp_3 * m4Get(mat,0,1) + tmp_6 * m4Get(mat,1,1) + tmp_11 * m4Get(mat,3,1)));
  float t3 = ((tmp_5 * m4Get(mat,0,1) + tmp_8 * m4Get(mat,1,1) + tmp_11 * m4Get(mat,2,1)) -
            (tmp_4 * m4Get(mat,0,1) + tmp_9 * m4Get(mat,1,1) + tmp_10 * m4Get(mat,2,1)));

  float d = 1.0 / (m4Get(mat,0,0) * t0 + m4Get(mat,1,0) * t1 + m4Get(mat,2,0) * t2 + m4Get(mat,3,0) * t3);

  float out_00 = d * t0;
  float out_01 = d * t1;
  float out_02 = d * t2;
  float out_03 = d * t3;

  float out_10 = d * ((tmp_1 * m4Get(mat,1,0) + tmp_2 * m4Get(mat,2,0) + tmp_5 * m4Get(mat,3,0)) -
                    (tmp_0 * m4Get(mat,1,0) + tmp_3 * m4Get(mat,2,0) + tmp_4 * m4Get(mat,3,0)));
  float out_11 = d * ((tmp_0 * m4Get(mat,0,0) + tmp_7 * m4Get(mat,2,0) + tmp_8 * m4Get(mat,3,0)) -
                    (tmp_1 * m4Get(mat,0,0) + tmp_6 * m4Get(mat,2,0) + tmp_9 * m4Get(mat,3,0)));
  float out_12 = d * ((tmp_3 * m4Get(mat,0,0) + tmp_6 * m4Get(mat,1,0) + tmp_11 * m4Get(mat,3,0)) -
                    (tmp_2 * m4Get(mat,0,0) + tmp_7 * m4Get(mat,1,0) + tmp_10 * m4Get(mat,3,0)));
  float out_13 = d * ((tmp_4 * m4Get(mat,0,0) + tmp_9 * m4Get(mat,1,0) + tmp_10 * m4Get(mat,2,0)) -
                    (tmp_5 * m4Get(mat,0,0) + tmp_8 * m4Get(mat,1,0) + tmp_11 * m4Get(mat,2,0)));

  float out_20 = d * ((tmp_12 * m4Get(mat,1,3) + tmp_15 * m4Get(mat,2,3) + tmp_16 * m4Get(mat,3,3)) -
                    (tmp_13 * m4Get(mat,1,3) + tmp_14 * m4Get(mat,2,3) + tmp_17 * m4Get(mat,3,3)));
  float out_21 = d * ((tmp_13 * m4Get(mat,0,3) + tmp_18 * m4Get(mat,2,3) + tmp_21 * m4Get(mat,3,3)) -
                    (tmp_12 * m4Get(mat,0,3) + tmp_19 * m4Get(mat,2,3) + tmp_20 * m4Get(mat,3,3)));
  float out_22 = d * ((tmp_14 * m4Get(mat,0,3) + tmp_19 * m4Get(mat,1,3) + tmp_22 * m4Get(mat,3,3)) -
                    (tmp_15 * m4Get(mat,0,3) + tmp_18 * m4Get(mat,1,3) + tmp_23 * m4Get(mat,3,3)));
  float out_23 = d * ((tmp_17 * m4Get(mat,0,3) + tmp_20 * m4Get(mat,1,3) + tmp_23 * m4Get(mat,2,3)) -
                    (tmp_16 * m4Get(mat,0,3) + tmp_21 * m4Get(mat,1,3) + tmp_22 * m4Get(mat,2,3)));

  float out_30 = d * ((tmp_14 * m4Get(mat,2,2) + tmp_17 * m4Get(mat,3,2) + tmp_13 * m4Get(mat,1,2)) -
                    (tmp_16 * m4Get(mat,3,2) + tmp_12 * m4Get(mat,1,2) + tmp_15 * m4Get(mat,2,2)));
  float out_31 = d * ((tmp_20 * m4Get(mat,3,2) + tmp_12 * m4Get(mat,0,2) + tmp_19 * m4Get(mat,2,2)) -
                    (tmp_18 * m4Get(mat,2,2) + tmp_21 * m4Get(mat,3,2) + tmp_13 * m4Get(mat,0,2)));
  float out_32 = d * ((tmp_18 * m4Get(mat,1,2) + tmp_23 * m4Get(mat,3,2) + tmp_15 * m4Get(mat,0,2)) -
                    (tmp_22 * m4Get(mat,3,2) + tmp_14 * m4Get(mat,0,2) + tmp_19 * m4Get(mat,1,2)));
  float out_33 = d * ((tmp_22 * m4Get(mat,2,2) + tmp_16 * m4Get(mat,0,2) + tmp_21 * m4Get(mat,1,2)) -
                    (tmp_20 * m4Get(mat,1,2) + tmp_23 * m4Get(mat,2,2) + tmp_17 * m4Get(mat,0,2)));

  mat[0*4+0] = out_00;
  mat[0*4+1] = out_01;
  mat[0*4+2] = out_02;
  mat[0*4+3] = out_03;
  mat[1*4+0] = out_10;
  mat[1*4+1] = out_11;
  mat[1*4+2] = out_12;
  mat[1*4+3] = out_13;
  mat[2*4+0] = out_20;
  mat[2*4+1] = out_21;
  mat[2*4+2] = out_22;
  mat[2*4+3] = out_23;
  mat[3*4+0] = out_30;
  mat[3*4+1] = out_31;
  mat[3*4+2] = out_32;
  mat[3*4+3] = out_33;
  return mat;
}

/* Puts the inverse of other matrix into mat */
static inline float *
m4Inverse (float *mat, const float *other) {
  m4Copy (mat, other);
  m4Invert (mat);
  return mat;
}

/* In-place transpose */
static inline float *
m4Transpose (float *mat) {
  float tmp = mat[0*4+1];
  mat[0*4+1] = mat[1*4+0];
  mat[1*4+0] = tmp;

  tmp = mat[0*4+2];
  mat[0*4+2] = mat[2*4+0];
  mat[2*4+0] = tmp;

  tmp = mat[0*4+3];
  mat[0*4+3] = mat[3*4+0];
  mat[3*4+0] = tmp;

  tmp = mat[1*4+2];
  mat[1*4+2] = mat[2*4+1];
  mat[2*4+1] = tmp;

  tmp = mat[1*4+3];
  mat[1*4+3] = mat[3*4+1];
  mat[3*4+1] = tmp;

  tmp = mat[2*4+3];
  mat[2*4+3] = mat[3*4+2];
  mat[3*4+2] = tmp;

  return mat;
}

#endif /* MATRIX4X4_HH */
