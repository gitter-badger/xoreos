#include <cmath>

#define CATCH_CONFIG_MAIN
#include <catch.hpp>

#include "common/transmatrix.h"

#define GLM_FORCE_RADIANS
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/io.hpp>

namespace Common {
	static inline std::ostream &operator<<(std::ostream &s, Common::TransformationMatrix const &m) {
		s << glm::make_mat4(m.get());
		return s;
	}

	inline bool operator==(Common::TransformationMatrix const &a, glm::mat4 const &b) {
		for (size_t i = 0; i < 16; ++i) {
			if (std::abs(a.get()[i] - glm::value_ptr(b)[i]) > 10 * std::numeric_limits<float>::epsilon())
				return false;
		}

		return true;
	}

	inline bool operator==(glm::mat4 const &b, Common::TransformationMatrix const &a) {
		for (size_t i = 0; i < 16; ++i) {
			if (std::abs(a.get()[i] - glm::value_ptr(b)[i]) > 10 * std::numeric_limits<float>::epsilon())
				return false;
		}

		return true;
	}

	inline bool operator==(Common::TransformationMatrix const &a, Common::TransformationMatrix const &b) {
		for (size_t i = 0; i < 16; ++i) {
			if (std::abs(a.get()[i] - b.get()[i]) > 10 * std::numeric_limits<float>::epsilon())
				return false;
		}

		return true;
	}
}
typedef float float16_t[16];

static float const kMatrixEmpty[16] = {
	0.0f, 0.0f, 0.0f, 0.0f,
	0.0f, 0.0f, 0.0f, 0.0f,
	0.0f, 0.0f, 0.0f, 0.0f,
	0.0f, 0.0f, 0.0f, 0.0f
};

static float const kMatrixIdentity[16] = {
	1.0f, 0.0f, 0.0f, 0.0f,
	0.0f, 1.0f, 0.0f, 0.0f,
	0.0f, 0.0f, 1.0f, 0.0f,
	0.0f, 0.0f, 0.0f, 1.0f
};

static float const kMatrixA[16] = {
	1.0f,  2.0f,  3.0f,  4.0f,
	5.0f,  6.0f,  7.0f,  8.0f,
	9.0f,  10.0f, 11.0f, 12.0f,
	13.0f, 14.0f, 15.0f, 16.0f
};

static float const kMatrixR[16] = {
	0.0f, 0.0f, 1.0f, 0.0f,
	1.0f, 0.0f, 0.0f, 0.0f,
	0.0f, 1.0f, 0.0f, 0.0f,
	0.0f, 0.0f, 0.0f, 1.0f
};

static float const kMatrixT[16] = {
	1.0f, 0.0f, 0.0f, 0.0f,
	0.0f, 1.0f, 0.0f, 0.0f,
	0.0f, 0.0f, 1.0f, 0.0f,
	1.0f, 2.0f, 3.0f, 1.0f
};

/*
   TEST_CASE("Matrix non-identity construction", "[matrix]") {
    auto A = Common::TransformationMatrix(false);

    REQUIRE(A == glm::make_mat4(kMatrixEmpty));
   }
 */

TEST_CASE("Matrix identity construction", "[matrix]") {
	auto A = Common::TransformationMatrix(true);

	REQUIRE(A == glm::make_mat4(kMatrixIdentity));
}

TEST_CASE("Matrix copy construction", "[matrix]") {
	auto A = Common::TransformationMatrix(true);
	auto B = Common::TransformationMatrix(A);

	REQUIRE(B == A);
}

TEST_CASE("Matrix float* construction", "[matrix]") {
	auto B = Common::TransformationMatrix(kMatrixA);

	REQUIRE(B == glm::make_mat4(kMatrixA));
}

TEST_CASE("Matrix getX(), getY(), getZ(), getPosition(...)", "[matrix]") {
	auto A = Common::TransformationMatrix(kMatrixA);

	REQUIRE(A.getX() == 13.0f);
	REQUIRE(A.getY() == 14.0f);
	REQUIRE(A.getZ() == 15.0f);

	float x = 0.0f, y = 0.0f, z = 0.0f;
	A.getPosition(x, y, z);
	REQUIRE(x == 13.0f);
	REQUIRE(y == 14.0f);
	REQUIRE(z == 15.0f);

	REQUIRE(glm::make_vec3(A.getPosition()) == glm::vec3(13.0f, 14.0f, 15.0f));
}

TEST_CASE("Matrix getXAxis(), getYAxis(), getZAxis()", "[matrix]") {
	auto A = Common::TransformationMatrix(kMatrixA);

	REQUIRE(glm::make_vec3(A.getXAxis()) == glm::vec3(1.0f, 2.0f, 3.0f));
	REQUIRE(glm::make_vec3(A.getYAxis()) == glm::vec3(5.0f, 6.0f, 7.0f));
	REQUIRE(glm::make_vec3(A.getZAxis()) == glm::vec3(9.0f, 10.0f, 11.0f));
}

TEST_CASE("Matrix loadIdentity()", "[matrix]") {
	auto A = Common::TransformationMatrix(kMatrixA);

	A.loadIdentity();
	REQUIRE(A == glm::make_mat4(kMatrixIdentity));
}

TEST_CASE("Matrix translate(...)", "[matrix]") {
	auto A = Common::TransformationMatrix(kMatrixIdentity);

	A.translate(1.0f, 2.0f, 3.0f);
	REQUIRE(A == glm::make_mat4(float16_t { 1.0f, 0.0f, 0.0f, 0.0f,
	                                        0.0f, 1.0f, 0.0f, 0.0f,
	                                        0.0f, 0.0f, 1.0f, 0.0f,
	                                        1.0f, 2.0f, 3.0f, 1.0f }));

	A.translate(Common::Vector3(-1.0f, -2.0f, -3.0f));
	REQUIRE(A == glm::make_mat4(kMatrixIdentity));
}

TEST_CASE("Matrix scale(...)", "[matrix]") {
	auto A = Common::TransformationMatrix(kMatrixIdentity);
	A.translate(1.0f, 2.0f, 3.0f);
	auto B = A;

	A.scale(2.0f, 3.0f, 4.0f);
	REQUIRE(A == glm::make_mat4(float16_t { 2.0f, 0.0f, 0.0f, 0.0f,
	                                        0.0f, 3.0f, 0.0f, 0.0f,
	                                        0.0f, 0.0f, 4.0f, 0.0f,
	                                        1.0f, 2.0f, 3.0f, 1.0f }));

	A.scale(Common::Vector3(1.0f / 2.0f, 1.0f / 3.0f, 1.0f / 4.0f));
	REQUIRE(A == B);
}

TEST_CASE("Matrix rotate(...)", "[matrix]") {
	auto A = Common::TransformationMatrix(kMatrixIdentity);
	A.translate(1.0f, 2.0f, 3.0f);
	A.scale(2.0f, 3.0f, 4.0f);
	auto B = A;

	A.rotate(90.0f, 1.0f, 0.0f, 0.0f);
	REQUIRE(A == glm::make_mat4(float16_t { 2.0f, 0.0f, 0.0f, 0.0f,
	                                        0.0f, 0.0f, 4.0f, 0.0f,
	                                        0.0f, -3.0f, 0.0f, 0.0f,
	                                        1.0f, 2.0f, 3.0f, 1.0f }));

	A.rotate(270.0f, 1.0f, 0.0f, 0.0f);
	REQUIRE(A == B);

	A.rotate(120.0f, 1.0f, 1.0f, 1.0f);
	REQUIRE(A == glm::make_mat4(float16_t { 0.0f, 3.0f, 0.0f, 0.0f,
	                                        0.0f, 0.0f, 4.0f, 0.0f,
	                                        2.0f, 0.0f, 0.0f, 0.0f,
	                                        1.0f, 2.0f, 3.0f, 1.0f }));

	A.rotate(240.0f, 1.0f, 1.0f, 1.0f);
	REQUIRE(A == B);
}

TEST_CASE("Matrix rotateAxisLocal(...)", "[matrix]") {
	auto A = Common::TransformationMatrix(kMatrixIdentity);
	A.translate(1.0f, 2.0f, 3.0f);
	A.scale(2.0f, 3.0f, 4.0f);
	auto B = A;

	A.rotateAxisLocal(Common::Vector3(1.0f, 0.0f, 0.0f), 90.0f);
	REQUIRE(A == glm::make_mat4(float16_t { 2.0f, 0.0f, 0.0f, 0.0f,
	                                        0.0f, 0.0f, 4.0f, 0.0f,
	                                        0.0f, -3.0f, 0.0f, 0.0f,
	                                        1.0f, 2.0f, 3.0f, 1.0f }));

	A.rotateAxisLocal(Common::Vector3(1.0f, 0.0f, 0.0f), 270.0f);
	REQUIRE(A == B);

	auto V = Common::Vector3(1.0f, 1.0f, 1.0f);
	V /= V.length();

	A.rotateAxisLocal(V, 120.0f);
	REQUIRE(A == glm::make_mat4(float16_t { 0.0f, 3.0f, 0.0f, 0.0f,
	                                        0.0f, 0.0f, 4.0f, 0.0f,
	                                        2.0f, 0.0f, 0.0f, 0.0f,
	                                        1.0f, 2.0f, 3.0f, 1.0f }));

	A.rotateAxisLocal(V, 240.0f);
	REQUIRE(A == B);
}

TEST_CASE("Matrix rotateXAxisLocal(), rotateYAxisLocal(), rotateZAxisLocal()", "[matrix]") {
	auto A = Common::TransformationMatrix(kMatrixIdentity);
	A.translate(1.0f, 2.0f, 3.0f);
	A.scale(2.0f, 3.0f, 4.0f);
	auto B = A;

	A.rotateXAxisLocal(90.0f);
	REQUIRE(A == glm::make_mat4(float16_t { 2.0f, 0.0f, 0.0f, 0.0f,
	                                        0.0f, 0.0f, 4.0f, 0.0f,
	                                        0.0f, -3.0f, 0.0f, 0.0f,
	                                        1.0f, 2.0f, 3.0f, 1.0f }));

	A.rotateYAxisLocal(90.0f);
	REQUIRE(A == glm::make_mat4(float16_t { 0.0f, 3.0f, 0.0f, 0.0f,
	                                        0.0f, 0.0f, 4.0f, 0.0f,
	                                        2.0f, 0.0f, 0.0f, 0.0f,
	                                        1.0f, 2.0f, 3.0f, 1.0f }));

	A.rotateZAxisLocal(90.0f);
	REQUIRE(A == glm::make_mat4(float16_t { 0.0f, 0.0f, 4.0f, 0.0f,
	                                        0.0f, -3.0f, 0.0f, 0.0f,
	                                        2.0f, 0.0f, 0.0f, 0.0f,
	                                        1.0f, 2.0f, 3.0f, 1.0f }));
}

TEST_CASE("Matrix rotateAxisWorld(...)", "[matrix]") {
	auto A = Common::TransformationMatrix(kMatrixIdentity);
	A.translate(1.0f, 2.0f, 3.0f);
	A.scale(2.0f, 3.0f, 4.0f);
	auto B = A;

	A.rotateAxisWorld(Common::Vector3(1.0f, 0.0f, 0.0f), 90.0f);
	REQUIRE(A == glm::make_mat4(float16_t { 2.0f, 0.0f, 0.0f, 0.0f,
	                                        0.0f, 0.0f, 3.0f, 0.0f,
	                                        0.0f, -4.0f, 0.0f, 0.0f,
	                                        1.0f, -3.0f, 2.0f, 1.0f }));

	A.rotateAxisWorld(Common::Vector3(1.0f, 0.0f, 0.0f), 270.0f);
	REQUIRE(A == B);

	auto V = Common::Vector3(1.0f, 1.0f, 1.0f);
	V /= V.length();

	A.rotateAxisWorld(V, 120.0f);
	REQUIRE(A == glm::make_mat4(float16_t { 0.0f, 2.0f, 0.0f, 0.0f,
	                                        0.0f, 0.0f, 3.0f, 0.0f,
	                                        4.0f, 0.0f, 0.0f, 0.0f,
	                                        3.0f, 1.0f, 2.0f, 1.0f }));

	A.rotateAxisWorld(V, 240.0f);
	REQUIRE(A == B);
}

TEST_CASE("Matrix rotateXAxisWorld(), rotateYAxisWorld(), rotateZAxisWorld()", "[matrix]") {
	auto A = Common::TransformationMatrix(kMatrixIdentity);
	A.translate(1.0f, 2.0f, 3.0f);
	A.scale(2.0f, 3.0f, 4.0f);
	auto B = A;

	A.rotateXAxisWorld(90.0f);
	REQUIRE(A == glm::make_mat4(float16_t { 2.0f, 0.0f, 0.0f, 0.0f,
	                                        0.0f, 0.0f, 3.0f, 0.0f,
	                                        0.0f, -4.0f, 0.0f, 0.0f,
	                                        1.0f, -3.0f, 2.0f, 1.0f }));

	A.rotateYAxisWorld(90.0f);
	REQUIRE(A == glm::make_mat4(float16_t { 0.0f, 0.0f, -2.0f, 0.0f,
	                                        3.0f, 0.0f, 0.0f, 0.0f,
	                                        0.0f, -4.0f, 0.0f, 0.0f,
	                                        2.0f, -3.0f, -1.0f, 1.0f }));

	A.rotateZAxisWorld(90.0f);
	REQUIRE(A == glm::make_mat4(float16_t { 0.0f, 0.0f, -2.0f, 0.0f,
	                                        0.0f, 3.0f, 0.0f, 0.0f,
	                                        4.0f, 0.0f, 0.0f, 0.0f,
	                                        3.0f, 2.0f, -1.0f, 1.0f }));
}

TEST_CASE("Matrix setRotation(...)", "[matrix]") {
	auto A = Common::TransformationMatrix(kMatrixIdentity);

	A.setRotation(kMatrixA);
	REQUIRE(A == glm::make_mat4(float16_t { 1.0f, 2.0f, 3.0f, 0.0f,
	                                        5.0f, 6.0f, 7.0f, 0.0f,
	                                        9.0f, 10.0f, 11.0f, 0.0f,
	                                        0.0f, 0.0f, 0.0f, 1.0f }));

	A.resetRotation();
	REQUIRE(A == glm::make_mat4(kMatrixIdentity));
}

TEST_CASE("Matrix transform(...)", "[matrix]") {
	auto A = Common::TransformationMatrix(kMatrixIdentity);
	auto B = Common::TransformationMatrix(kMatrixA);
	auto R = Common::TransformationMatrix(kMatrixR);
	auto T = Common::TransformationMatrix(kMatrixT);

	A.transform(B);
	REQUIRE(A == B);

	A.transform(T, R);
	REQUIRE(A == glm::make_mat4(float16_t { 0.0f, 0.0f, 1.0f, 0.0f,
	                                        1.0f, 0.0f, 0.0f, 0.0f,
	                                        0.0f, 1.0f, 0.0f, 0.0f,
	                                        1.0f, 2.0f, 3.0f, 1.0f }));
}

TEST_CASE("Matrix getInverse(...)", "[matrix]") {
	auto A = Common::TransformationMatrix(kMatrixIdentity);

	auto B = A.getInverse();
	REQUIRE(A == B);

	auto C = Common::TransformationMatrix(kMatrixR).getInverse();
	REQUIRE(C == glm::make_mat4(float16_t { 0.0f, 1.0f, 0.0f, 0.0f,
	                                        0.0f, 0.0f, 1.0f, 0.0f,
	                                        1.0f, 0.0f, 0.0f, 0.0f,
	                                        0.0f, 0.0f, 0.0f, 1.0f }));
}

TEST_CASE("Matrix transpose(...)", "[matrix]") {
	auto A = Common::TransformationMatrix(kMatrixIdentity);

	auto B = A.transpose();
	REQUIRE(A == B);

	auto C = Common::TransformationMatrix(kMatrixR).transpose();
	REQUIRE(C == glm::make_mat4(float16_t { 0.0f, 1.0f, 0.0f, 0.0f,
	                                        0.0f, 0.0f, 1.0f, 0.0f,
	                                        1.0f, 0.0f, 0.0f, 0.0f,
	                                        0.0f, 0.0f, 0.0f, 1.0f }));
}

TEST_CASE("Matrix lookAt(...)", "[matrix]") {
	auto A = Common::TransformationMatrix(kMatrixIdentity);

	A.lookAt(Common::Vector3(0.0f, 0.0f, 1.0f));
	REQUIRE(A == glm::make_mat4(float16_t { 1.0f, 0.0f, 0.0f, 0.0f,
	                                        0.0f, 1.0f, 0.0f, 0.0f,
	                                        0.0f, 0.0f, 1.0f, 0.0f,
	                                        0.0f, 0.0f, 0.0f, 1.0f }));

	A.lookAt(Common::Vector3(0.0f, 1.0f, 0.0f));
	REQUIRE(A == glm::make_mat4(float16_t { 0.0f, 0.0f, -1.0f, 0.0f,
	                                        -1.0f, 0.0f, 0.0f, 0.0f,
	                                        0.0f, 1.0f, 0.0f, 0.0f,
	                                        0.0f, 0.0f, 0.0f, 1.0f }));

	A.lookAt(Common::Vector3(1.0f, 0.0f, 0.0f));
	REQUIRE(A == glm::make_mat4(float16_t { 0.0f, 0.0f, -1.0f, 0.0f,
	                                        0.0f, 1.0f, 0.0f, 0.0f,
	                                        1.0f, 0.0f, 0.0f, 0.0f,
	                                        0.0f, 0.0f, 0.0f, 1.0f }));

	static float const pi = std::acos(-1);
	static float const c4 = std::cos(pi / 4.0f);
	static float const s3 = std::sqrt(3.0f);

	A.lookAt(Common::Vector3(1.0f, 1.0f, 1.0f));
	REQUIRE(A == glm::make_mat4(float16_t { c4, 0.0f, -c4, 0.0f,
	                                        -c4 / s3, 2 * c4 / s3, -c4 / s3, 0.0f,
	                                        1.0f / s3, 1.0f / s3, 1.0f / s3, 0.0f,
	                                        0.0f, 0.0f, 0.0f, 1.0f }));
}

TEST_CASE("Matrix perspective(...)", "[matrix]") {
	auto A = Common::TransformationMatrix(kMatrixIdentity);

	A.perspective(90.f, 1.0f, -1.0f, 1.0f);
	REQUIRE(A == glm::make_mat4(float16_t { 1.0f, 0.0f, 0.0f, 0.0f,
	                                        0.0f, 1.0f, 0.0f, 0.0f,
	                                        0.0f, 0.0f, 0.0f, -1.0f,
	                                        0.0f, 0.0f, 1.0f, 0.0f }));
}

TEST_CASE("Matrix ortho(...)", "[matrix]") {
	auto A = Common::TransformationMatrix(kMatrixIdentity);

	A.ortho(0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f);
	REQUIRE(A == glm::make_mat4(float16_t { 2.0f, 0.0f, 0.0f, 0.0f,
	                                        0.0f, 2.0f, 0.0f, 0.0f,
	                                        0.0f, 0.0f, -2.0f, 0.0f,
	                                        -1.0f, -1.0f, -1.0f, 1.0f }));
}

/*
    const TransformationMatrix &operator=(const TransformationMatrix &m);
    const TransformationMatrix &operator=(const float *m);

    float &operator[](unsigned int index);
    float  operator[](unsigned int index) const;
    float &operator()(int row, int column);
    float  operator()(int row, int column) const;

    const TransformationMatrix &operator*=(const TransformationMatrix &m);
    TransformationMatrix operator*(const TransformationMatrix &m) const;
    Vector3 operator*(const Vector3 &v) const;

    Vector3 vectorRotate(Vector3 &v) const;
    Vector3 vectorRotateReverse(Vector3 &v) const;
 */
