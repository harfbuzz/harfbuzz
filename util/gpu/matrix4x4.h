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

#ifndef MATRIX4x4_H
#define MATRIX4x4_H

/* Copies other matrix into mat */
float *
m4Copy (float *mat, const float *other);

float *
m4Multiply (float *mat, const float *right);

float *
m4MultMatrix (float *mat, const float *left);

float
m4Get (float *mat, unsigned int row, unsigned int col);

float *
m4Scale (float *mat, float sx, float sy, float sz);

float *
m4Translate (float *mat, float tx, float ty, float tz);

float *
m4Rotate (float *mat, float angle, float x, float y, float z);

float *
m4Frustum (float *mat, float left, float right, float bottom, float top, float nearZ, float farZ);

float *
m4Perspective (float *mat, float fovy, float aspect, float nearZ, float farZ);

float *
m4Ortho (float *mat, float left, float right, float bottom, float top, float nearZ, float farZ);

/* In-place inversion */
float *
m4Invert (float *mat);

/* Puts the inverse of other matrix into mat */
float *
m4Inverse (float *mat, const float *other);

/* In-place transpose */
float *
m4Transpose (float *mat);

float *
m4LoadIdentity (float *mat);

#endif
