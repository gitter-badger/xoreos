/* xoreos - A reimplementation of BioWare's Aurora engine
 *
 * xoreos is the legal property of its developers, whose names
 * can be found in the AUTHORS file distributed with this source
 * distribution.
 *
 * xoreos is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * xoreos is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with xoreos. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file common/transmatrix.cpp
 *  A transformation matrix.
 */

#include <cstring>

#include "common/transmatrix.h"
#include "common/maths.h"

#define GLM_FORCE_RADIANS
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/transform.hpp>

namespace Common {

TransformationMatrix::TransformationMatrix(bool identity) : _matrix(glm::mat4(0.0f)) {
	if(identity)
		loadIdentity();
}

TransformationMatrix::TransformationMatrix(const TransformationMatrix &m) {
	_matrix = m._matrix;
}

TransformationMatrix::TransformationMatrix(const float *m) {
	_matrix = glm::make_mat4(m);
}

TransformationMatrix::~TransformationMatrix() {
}

const float *TransformationMatrix::get() const {
	return glm::value_ptr(_matrix);
}

float TransformationMatrix::getX() const {
	return glm::value_ptr(_matrix)[12];
}

float TransformationMatrix::getY() const {
	return glm::value_ptr(_matrix)[13];
}

float TransformationMatrix::getZ() const {
	return glm::value_ptr(_matrix)[14];
}

void TransformationMatrix::getPosition(float &x, float &y, float &z) const {
	x = glm::value_ptr(_matrix)[12];
	y = glm::value_ptr(_matrix)[13];
	z = glm::value_ptr(_matrix)[14];
}

const float *TransformationMatrix::getPosition() const {
	return get() + 12;
}

const float *TransformationMatrix::getXAxis() const {
	return get() + 0;
}

const float *TransformationMatrix::getYAxis() const {
	return get() + 4;
}

const float *TransformationMatrix::getZAxis() const {
	return get() + 8;
}

void TransformationMatrix::loadIdentity() {
	_matrix = glm::mat4();
}

void TransformationMatrix::translate(float x, float y, float z) {
	_matrix = glm::translate(_matrix, glm::vec3(x, y, z));
}

void TransformationMatrix::translate(const Vector3 &v) {
	/* As a minor optimisation, the 'w' component can be left out. This is safe
	 * if we assume that the matrix in question is not used for perspective
	 * calculations. Generally this is acceptable.
	 * If done, then _elements[15] should be set to 1.0f.
	 * It can also be safely assumed that v._w is 1.0f, for further optimisations.
	 */
	_matrix = glm::translate(_matrix, glm::vec3(v[0], v[1], v[2]));
}

void TransformationMatrix::scale(float x, float y, float z) {
	_matrix = glm::scale(_matrix, glm::vec3(x, y, z));
}

void TransformationMatrix::scale(const Vector3 &v) {
	_matrix = glm::scale(_matrix, glm::vec3(v[0], v[1], v[2]));
}

void TransformationMatrix::rotate(float angle, float x, float y, float z) {
	_matrix = glm::rotate(_matrix, deg2rad(angle), glm::vec3(x, y, z));
}

void TransformationMatrix::rotateAxisLocal(const Vector3 &v, float angle) {
	_matrix = glm::rotate(_matrix, deg2rad(angle), glm::vec3(v[0], v[1], v[2]));
}

void TransformationMatrix::rotateXAxisLocal(float angle) {
	_matrix = glm::rotate(_matrix, deg2rad(angle), glm::vec3(1.0f, 0.0f, 0.0f));
}

void TransformationMatrix::rotateYAxisLocal(float angle) {
	_matrix = glm::rotate(_matrix, deg2rad(angle), glm::vec3(0.0f, 1.0f, 0.0f));
}

void TransformationMatrix::rotateZAxisLocal(float angle) {
	_matrix = glm::rotate(_matrix, deg2rad(angle), glm::vec3(0.0f, 0.0f, 1.0f));
}

void TransformationMatrix::rotateAxisWorld(const Vector3 &v, float angle) {
	_matrix = glm::rotate(deg2rad(angle), glm::vec3(v[0], v[1], v[2])) * _matrix;
}

void TransformationMatrix::rotateXAxisWorld(float angle) {
	_matrix = glm::rotate(deg2rad(angle), glm::vec3(1.0f, 0.0f, 0.0f)) * _matrix;
}

void TransformationMatrix::rotateYAxisWorld(float angle) {
	_matrix = glm::rotate(deg2rad(angle), glm::vec3(0.0f, 1.0f, 0.0f)) * _matrix;
}

void TransformationMatrix::rotateZAxisWorld(float angle) {
	_matrix = glm::rotate(deg2rad(angle), glm::vec3(0.0f, 0.0f, 1.0f)) * _matrix;
}

void TransformationMatrix::setRotation(const TransformationMatrix &m) {
	_matrix += glm::mat4(glm::mat3(m._matrix)) - glm::mat4(glm::mat3(_matrix));
}

void TransformationMatrix::resetRotation() {
	setRotation(TransformationMatrix(true));
}

void TransformationMatrix::transform(const TransformationMatrix &m) {
	_matrix *= m._matrix;
}

void TransformationMatrix::transform(const TransformationMatrix &a, const TransformationMatrix &b) {
	_matrix = a._matrix * b._matrix;
}

TransformationMatrix TransformationMatrix::getInverse() {
	TransformationMatrix t(false);

	float det = glm::determinant(_matrix);
	if(fabs(det) <= 0.00001f) {
		t.loadIdentity();
		return t;
	}

	t._matrix = glm::inverse(_matrix);

	return t;
}

TransformationMatrix TransformationMatrix::transpose() {
	TransformationMatrix t(false);
	t._matrix = glm::transpose(_matrix);
	return t;
}

void TransformationMatrix::lookAt(const Vector3 &v) {
	/* [x,y,z] is the z-axis vector. Cross this with [0,1,0] to create the x-axis, and
	 * calculate the y-axis with (z-axis x x-axis).
	 * Most of this was taken from stock standard gluLookAt components, however there's
	 * one crucial difference. gluLookAt assumes that the world is rotated (standard
	 * OpenGL stuff); this assumes rotating some object.
	 */

	if(((v._x * v._x) < 0.00001f) && ((v._z * v._z) < 0.00001f))
		_matrix = glm::transpose(glm::lookAt(glm::vec3(), -glm::vec3(v[0], v[1], v[2]), glm::vec3(-1.0f, 0.0f, 0.0f)));
	else
		_matrix = glm::transpose(glm::lookAt(glm::vec3(), -glm::vec3(v[0], v[1], v[2]), glm::vec3(0.0f, 1.0f, 0.0f)));
}

void TransformationMatrix::perspective(float fovy, float aspectRatio, float znear, float zfar) {
	_matrix = glm::perspective(deg2rad(fovy), aspectRatio, znear, zfar);
}

void TransformationMatrix::ortho(float l, float r, float b, float t, float n, float f)
{
	_matrix = glm::ortho(l, r, b, t, n, f);
}

const TransformationMatrix &TransformationMatrix::operator=(const TransformationMatrix &m) {
	_matrix = m._matrix;
	return *this;
}

const TransformationMatrix &TransformationMatrix::operator=(const float *m) {
	_matrix = glm::make_mat4(m);
	return *this;
}

float &TransformationMatrix::operator[](unsigned int index) {
	return _matrix[index / 4][index % 4];
}

float  TransformationMatrix::operator[](unsigned int index) const {
	return _matrix[index / 4][index % 4];
}

float &TransformationMatrix::operator()(int row, int column) {
	return _matrix[column][row];
}

float  TransformationMatrix::operator()(int row, int column) const {
	return _matrix[column][row];
}

const TransformationMatrix &TransformationMatrix::operator*=(const TransformationMatrix &m) {
	this->transform(m);
	return *this;
}

TransformationMatrix TransformationMatrix::operator*(const TransformationMatrix &m) const {
	return TransformationMatrix(*this) *= m;
}

Vector3 TransformationMatrix::operator*(const Vector3 &v) const {
	const glm::vec4 w = _matrix * glm::vec4(v[0], v[1], v[2], 1.0f);
	return Vector3(w[0], w[1], w[2]);
}

Vector3 TransformationMatrix::vectorRotate(Vector3 &v) const {
	const glm::vec4 w = _matrix * glm::vec4(v[0], v[1], v[2], 1.0f);
	return Vector3(w[0], w[1], w[2]);
}

Vector3 TransformationMatrix::vectorRotateReverse(Vector3 &v) const {
	const glm::vec4 w = glm::vec4(v[0], v[1], v[2], 1.0f) * _matrix;
	return Vector3(w[0], w[1], w[2]);
}

} // End of namespace Common
