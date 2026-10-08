#include "Calculation.h"
//#include <math.h>
#include <cmath>
#include "assert.h"
#include <algorithm>






// ベクトル

Vector3 AddVector3(Vector3 v1, Vector3 v2)
{
	Vector3 result;
	result.x = v1.x + v2.x;
	result.y = v1.y + v2.y;
	result.z = v1.z + v2.z;
	return result;
}

Vector3 SubtractVector3(Vector3 v1, Vector3 v2)
{
	Vector3 result;
	result.x = v1.x - v2.x;
	result.y = v1.y - v2.y;
	result.z = v1.z - v2.z;
	return result;
}

Vector3 MultiplyVector3(Vector3 v1, Vector3 v2)
{
	Vector3 result;
	result.x = v1.x * v2.x;
	result.y = v1.y * v2.y;
	result.z = v1.z * v2.z;
	return result;
}

Vector3 ScalarMultiply(Vector3 v, float s)
{
	Vector3 result;
	result = { v.x * s,v.y * s,v.z * s };
	return result;
}

float Length(Vector3 v)
{
	float result;
	result = static_cast<float>(sqrt(v.x * v.x + v.y * v.y + v.z * v.z));
	return result;
}

Vector3 Normalize(Vector3 v)
{
	Vector3 result{};
	float len = Length(v);
	if (len != 0.0f) {
		float invLen = 1.0f / len;
		result.x = v.x * invLen;
		result.y = v.y * invLen;
		result.z = v.z * invLen;
	}
	return result;
}

float Dot(Vector3 v1, Vector3 v2)
{
	float result;
	result = v1.x * v2.x + v1.y * v2.y + v1.z * v2.z;
	return result;
}

Vector3 Cross(const Vector3& v1, const Vector3& v2) {
	Vector3 result;
	result.x = v1.y * v2.z - v1.z * v2.y;
	result.y = v1.z * v2.x - v1.x * v2.z;
	result.z = v1.x * v2.y - v1.y * v2.x;
	return result;
}


float Lerp(float a, float b, float t)
{
	return a + (b - a) * t;
}

Vector3 Lerp(const Vector3& a, const Vector3& b, float t)
{
	return a * (1.0f - t) + b * t;
}

Quaternion Lerp(const Quaternion& a, const Quaternion& b, float t) {
	Quaternion result;
	result.x = a.x * (1.0f - t) + b.x * t;
	result.y = a.y * (1.0f - t) + b.y * t;
	result.z = a.z * (1.0f - t) + b.z * t;
	result.w = a.w * (1.0f - t) + b.w * t;
	// 正規化（補間後のクォータニオンが単位クォータニオンになるように）
	float len = std::sqrt(result.x * result.x + result.y * result.y + result.z * result.z + result.w * result.w);
	if (len != 0.0f) {
		result.x /= len;
		result.y /= len;
		result.z /= len;
		result.w /= len;
	}
	return result;
}

Quaternion Slerp(const Quaternion& a, const Quaternion& b, float t) {
    float dot = a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
    Quaternion q2 = b;
    
    // 最短経路での補間
    if (dot < 0.0f) {
        q2.x = -b.x;
        q2.y = -b.y;
        q2.z = -b.z;
        q2.w = -b.w;
        dot = -dot;
    }
    
    // ドット積が1に非常に近い場合（角度が非常に小さい場合）、ゼロ除算を防ぐためにLerpを使用
    const float DOT_THRESHOLD = 0.9995f;
    if (dot > DOT_THRESHOLD) {
        return Lerp(a, q2, t);
    }
    
    float theta_0 = std::acos(dot);
    float theta = theta_0 * t;
    
    float sin_theta = std::sin(theta);
    float sin_theta_0 = std::sin(theta_0);
    
    float s0 = std::cos(theta) - dot * sin_theta / sin_theta_0;
    float s1 = sin_theta / sin_theta_0;
    
    Quaternion result;
    result.x = (a.x * s0) + (q2.x * s1);
    result.y = (a.y * s0) + (q2.y * s1);
    result.z = (a.z * s0) + (q2.z * s1);
    result.w = (a.w * s0) + (q2.w * s1);
    return result;
}

float EaseInOutSine(float x) {
	return -(std::cos(3.14159265f * x) - 1.0f) / 2.0f;
}

// 行列

Matrix4x4 AddMatrix4x4(Matrix4x4 m1, Matrix4x4 m2)//o
{
	Matrix4x4 result;
	for (int i = 0; i < 4; i++)
	{
		for (int j = 0; j < 4; j++)
		{
			result.m[i][j] = m1.m[i][j] + m2.m[i][j];
		}
	}
	return result;
}

Matrix4x4 MultiplyMatrix4x4(Matrix4x4 m1, Matrix4x4 m2)//o
{
	Matrix4x4 result;
	for (int i = 0; i < 4; i++)
	{
		for (int j = 0; j < 4; j++)
		{
			result.m[i][j]
				= m1.m[i][0] * m2.m[0][j]
					+ m1.m[i][1] * m2.m[1][j]
						+ m1.m[i][2] * m2.m[2][j]
							+ m1.m[i][3] * m2.m[3][j];
		}
	}
	return result;
}

Matrix4x4 TransposeMatrix4x4(Matrix4x4 m)//o
{
	Matrix4x4 result;
	for (int i = 0; i < 4; i++)
	{
		for (int j = 0; j < 4; j++)
		{
			result.m[i][j] = m.m[j][i];
		}
	}
	return result;
}

Matrix4x4 Inverse(Matrix4x4 m)
{
	Matrix4x4 result;

	float det
		= m.m[0][0] * m.m[1][1] * m.m[2][2] * m.m[3][3]
		+ m.m[0][0] * m.m[1][2] * m.m[2][3] * m.m[3][1]
		+ m.m[0][0] * m.m[1][3] * m.m[2][1] * m.m[3][2]

		- m.m[0][0] * m.m[1][3] * m.m[2][2] * m.m[3][1]
		- m.m[0][0] * m.m[1][2] * m.m[2][1] * m.m[3][3]
		- m.m[0][0] * m.m[1][1] * m.m[2][3] * m.m[3][2]

		- m.m[0][1] * m.m[1][0] * m.m[2][2] * m.m[3][3]
		- m.m[0][2] * m.m[1][0] * m.m[2][3] * m.m[3][1]
		- m.m[0][3] * m.m[1][0] * m.m[2][1] * m.m[3][2]

		+ m.m[0][3] * m.m[1][0] * m.m[2][2] * m.m[3][1]
		+ m.m[0][2] * m.m[1][0] * m.m[2][1] * m.m[3][3]
		+ m.m[0][1] * m.m[1][0] * m.m[2][3] * m.m[3][2]

		+ m.m[0][1] * m.m[1][2] * m.m[2][0] * m.m[3][3]
		+ m.m[0][2] * m.m[1][3] * m.m[2][0] * m.m[3][1]
		+ m.m[0][3] * m.m[1][1] * m.m[2][0] * m.m[3][2]

		- m.m[0][3] * m.m[1][2] * m.m[2][0] * m.m[3][1]
		- m.m[0][2] * m.m[1][1] * m.m[2][0] * m.m[3][3]
		- m.m[0][1] * m.m[1][3] * m.m[2][0] * m.m[3][2]

		- m.m[0][1] * m.m[1][2] * m.m[2][3] * m.m[3][0]
		- m.m[0][2] * m.m[1][3] * m.m[2][1] * m.m[3][0]
		- m.m[0][3] * m.m[1][1] * m.m[2][2] * m.m[3][0]

		+ m.m[0][3] * m.m[1][2] * m.m[2][1] * m.m[3][0]
		+ m.m[0][2] * m.m[1][1] * m.m[2][3] * m.m[3][0]
		+ m.m[0][1] * m.m[1][3] * m.m[2][2] * m.m[3][0];

	result.m[0][0] = (m.m[1][1] * m.m[2][2] * m.m[3][3] + m.m[1][2] * m.m[2][3] * m.m[3][1] + m.m[1][3] * m.m[2][1] * m.m[3][2]
		- m.m[1][3] * m.m[2][2] * m.m[3][1] - m.m[1][2] * m.m[2][1] * m.m[3][3] - m.m[1][1] * m.m[2][3] * m.m[3][2]) / det;//o

	result.m[0][1] = (-m.m[0][1] * m.m[2][2] * m.m[3][3] - m.m[0][2] * m.m[2][3] * m.m[3][1] - m.m[0][3] * m.m[2][1] * m.m[3][2]
		+ m.m[0][3] * m.m[2][2] * m.m[3][1] + m.m[0][2] * m.m[2][1] * m.m[3][3] + m.m[0][1] * m.m[2][3] * m.m[3][2]) / det;//o

	result.m[0][2] = (m.m[0][1] * m.m[1][2] * m.m[3][3] + m.m[0][2] * m.m[1][3] * m.m[3][1] + m.m[0][3] * m.m[1][1] * m.m[3][2]
		- m.m[0][3] * m.m[1][2] * m.m[3][1] - m.m[0][2] * m.m[1][1] * m.m[3][3] - m.m[0][1] * m.m[1][3] * m.m[3][2]) / det;//o

	result.m[0][3] = (-m.m[0][1] * m.m[1][3] * m.m[2][3] - m.m[0][2] * m.m[1][3] * m.m[2][1] - m.m[0][3] * m.m[1][1] * m.m[2][2]
		+ m.m[0][3] * m.m[1][2] * m.m[2][1] + m.m[0][2] * m.m[1][1] * m.m[2][3] + m.m[0][1] * m.m[1][3] * m.m[2][2]) / det;//o


	result.m[1][0] = (-m.m[1][0] * m.m[2][2] * m.m[3][3] - m.m[1][2] * m.m[2][3] * m.m[3][0] - m.m[1][3] * m.m[2][0] * m.m[3][2]
		+ m.m[1][3] * m.m[2][2] * m.m[3][0] + m.m[1][2] * m.m[2][0] * m.m[3][3] + m.m[1][0] * m.m[2][3] * m.m[3][2]) / det;//o

	result.m[1][1] = (m.m[0][0] * m.m[2][2] * m.m[3][3] + m.m[0][2] * m.m[2][3] * m.m[3][0] + m.m[0][3] * m.m[2][0] * m.m[3][2]
		- m.m[0][3] * m.m[2][2] * m.m[3][0] - m.m[0][2] * m.m[2][0] * m.m[3][3] - m.m[0][0] * m.m[2][3] * m.m[3][2]) / det;//o

	result.m[1][2] = (-m.m[0][0] * m.m[1][2] * m.m[3][3] - m.m[0][2] * m.m[1][3] * m.m[3][0] - m.m[0][3] * m.m[1][0] * m.m[3][2]
		+ m.m[0][3] * m.m[1][2] * m.m[3][0] + m.m[0][2] * m.m[1][0] * m.m[3][3] + m.m[0][0] * m.m[1][3] * m.m[3][2]) / det;//o

	result.m[1][3] = (m.m[0][0] * m.m[1][2] * m.m[2][3] + m.m[0][2] * m.m[1][3] * m.m[2][0] + m.m[0][3] * m.m[1][0] * m.m[2][2]
		- m.m[0][3] * m.m[1][2] * m.m[2][0] - m.m[0][2] * m.m[1][0] * m.m[2][3] - m.m[0][0] * m.m[1][3] * m.m[2][2]) / det;//o


	result.m[2][0] = (m.m[1][0] * m.m[2][1] * m.m[3][3] + m.m[1][1] * m.m[2][3] * m.m[3][0] + m.m[1][3] * m.m[2][0] * m.m[3][1]
		- m.m[1][3] * m.m[2][1] * m.m[3][0] - m.m[1][1] * m.m[2][0] * m.m[3][3] - m.m[1][0] * m.m[2][3] * m.m[3][1]) / det;//o

	result.m[2][1] = (-m.m[0][0] * m.m[2][1] * m.m[3][3] - m.m[0][1] * m.m[2][3] * m.m[3][0] - m.m[0][3] * m.m[2][0] * m.m[3][1]
		+ m.m[0][3] * m.m[2][1] * m.m[3][0] + m.m[0][1] * m.m[2][0] * m.m[3][3] + m.m[0][0] * m.m[2][3] * m.m[3][1]) / det;//o

	result.m[2][2] = (m.m[0][0] * m.m[1][1] * m.m[3][3] + m.m[0][1] * m.m[1][3] * m.m[3][0] + m.m[0][3] * m.m[1][0] * m.m[3][1]
		- m.m[0][3] * m.m[1][1] * m.m[3][0] - m.m[0][1] * m.m[1][0] * m.m[3][3] - m.m[0][0] * m.m[1][3] * m.m[3][1]) / det;//o

	result.m[2][3] = (-m.m[0][0] * m.m[1][1] * m.m[2][3] - m.m[0][1] * m.m[1][3] * m.m[2][0] - m.m[0][3] * m.m[1][0] * m.m[2][0]
		+ m.m[0][3] * m.m[1][1] * m.m[2][0] + m.m[0][1] * m.m[1][0] * m.m[2][3] + m.m[0][0] * m.m[1][3] * m.m[2][1]) / det;//o


	result.m[3][0] = (-m.m[1][0] * m.m[2][1] * m.m[3][2] - m.m[1][1] * m.m[2][2] * m.m[3][0] - m.m[1][2] * m.m[2][0] * m.m[3][1]
		+ m.m[1][2] * m.m[2][1] * m.m[3][0] + m.m[1][1] * m.m[2][0] * m.m[3][2] + m.m[1][0] * m.m[2][2] * m.m[3][1]) / det;//o

	result.m[3][1] = (m.m[0][0] * m.m[2][1] * m.m[3][2] + m.m[0][1] * m.m[2][2] * m.m[3][0] + m.m[0][2] * m.m[2][0] * m.m[3][1]
		- m.m[0][2] * m.m[2][1] * m.m[3][0] - m.m[0][1] * m.m[2][0] * m.m[3][2] - m.m[0][0] * m.m[2][2] * m.m[3][1]) / det;//o

	result.m[3][2] = (-m.m[0][0] * m.m[1][1] * m.m[3][2] - m.m[0][1] * m.m[1][2] * m.m[3][0] - m.m[0][2] * m.m[1][0] * m.m[3][1]
		+ m.m[0][2] * m.m[1][1] * m.m[3][0] + m.m[0][1] * m.m[1][0] * m.m[3][2] + m.m[0][0] * m.m[1][2] * m.m[3][1]) / det;//o

	result.m[3][3] = (m.m[0][0] * m.m[1][1] * m.m[2][2] + m.m[0][1] * m.m[1][2] * m.m[2][0] + m.m[0][2] * m.m[1][0] * m.m[2][1]
		- m.m[0][2] * m.m[1][1] * m.m[2][0] - m.m[0][1] * m.m[1][0] * m.m[2][2] - m.m[0][0] * m.m[1][2] * m.m[2][1]) / det;//o

	return result;
}

Matrix4x4 ScaleMultiplyMatrix4x4(Matrix4x4 m, float s)
{
	Matrix4x4 result;
	for (int i = 0; i < 4; i++)
	{
		for (int j = 0; j < 4; j++)
		{
			result.m[i][j] = m.m[i][j] * s;
		}
	}
	return result;
}

Vector3 TransformMatrix(const Vector3& v, const Matrix4x4& m)
{
	Vector3 result{};

	result.x = v.x * m.m[0][0] + v.y * m.m[1][0] + v.z * m.m[2][0] + m.m[3][0];
	result.y = v.x * m.m[0][1] + v.y * m.m[1][1] + v.z * m.m[2][1] + m.m[3][1];
	result.z = v.x * m.m[0][2] + v.y * m.m[1][2] + v.z * m.m[2][2] + m.m[3][2];
	float w = v.x * m.m[0][3] + v.y * m.m[1][3] + v.z * m.m[2][3] + m.m[3][3];
	if (w != 0) {
		result.x /= w;
		result.y /= w;
		result.z /= w;
	}
	return result;
}

Matrix4x4 Scale(Vector3 scale)
{
	Matrix4x4 result;

	result.m[0][0] = 1.0f * scale.x;
	result.m[1][1] = 1.0f * scale.y;
	result.m[2][2] = 1.0f * scale.z;
	result.m[3][3] = 1.0f;
	return result;
}

Matrix4x4 RotationX(float angle)
{
	Matrix4x4 result;
	result.m[0][0] = 1.0f;
	result.m[1][1] = std::cos(angle);
	result.m[1][2] = std::sin(angle);
	result.m[2][1] = std::sin(-angle);
	result.m[2][2] = std::cos(angle);
	result.m[3][3] = 1.0f;
	return result;
}

Matrix4x4 RotationY(float angle)
{
	Matrix4x4 result;
	result.m[0][0] = std::cos(angle);
	result.m[0][2] = std::sin(-angle);
	result.m[1][1] = 1.0f;
	result.m[2][0] = std::sin(angle);
	result.m[2][2] = std::cos(angle);
	result.m[3][3] = 1.0f;
	return result;
}

Matrix4x4 RotationZ(float angle)
{
	Matrix4x4 result;
	result.m[0][0] = std::cos(angle);
	result.m[0][1] = std::sin(angle);
	result.m[1][0] = std::sin(-angle);
	result.m[1][1] = std::cos(angle);
	result.m[2][2] = 1.0f;
	result.m[3][3] = 1.0f;
	return result;
}

Matrix4x4 Rotation(Vector3 angle)
{
	Matrix4x4 result;
	result = MultiplyMatrix4x4(RotationX(angle.x), MultiplyMatrix4x4(RotationY(angle.y), RotationZ(angle.z)));
	return result;
}

Matrix4x4 Translation(Vector3 pos)
{
	Matrix4x4 result;
	result.m[0][0] = 1.0f;
	result.m[1][1] = 1.0f;
	result.m[2][2] = 1.0f;
	result.m[3][3] = 1.0f;

	result.m[3][0] = pos.x;
	result.m[3][1] = pos.y;
	result.m[3][2] = pos.z;

	return result;
}


Vector3 MakeWorldPos(PolarCoordinates pos) {
	float pai = 3.141592f;
	Vector3 offset;
	offset.x = pos.radius * sinf(pos.theta * pai) * cosf(pos.theta * pai);
	offset.y = pos.radius * sinf(pos.theta * pai) * sinf(pos.theta * pai);
	offset.z = pos.radius * cosf(pos.theta * pai);
	
	Vector3 worldPos;
	worldPos.x = pos.offset.x + offset.x;
	worldPos.y = pos.offset.y + offset.y;
	worldPos.z = pos.offset.z + offset.z;

	return worldPos;
}




Matrix4x4 MakeLookAtMatrix(const Vector3& eye, const Vector3& target, const Vector3& up)
{
	Vector3 zaxis = Normalize(target - eye);        // 前方向（カメラの向き）
	Vector3 xaxis = Normalize(Cross(up, zaxis));    // 右方向
	Vector3 yaxis = Cross(zaxis, xaxis);            // 上方向

	Matrix4x4 result = IdentityMatrix();

	result.m[0][0] = xaxis.x;
	result.m[1][0] = xaxis.y;
	result.m[2][0] = xaxis.z;
	result.m[3][0] = -Dot(xaxis, eye);

	result.m[0][1] = yaxis.x;
	result.m[1][1] = yaxis.y;
	result.m[2][1] = yaxis.z;
	result.m[3][1] = -Dot(yaxis, eye);

	result.m[0][2] = -zaxis.x;
	result.m[1][2] = -zaxis.y;
	result.m[2][2] = -zaxis.z;
	result.m[3][2] = Dot(zaxis, eye);

	result.m[0][3] = 0.0f;
	result.m[1][3] = 0.0f;
	result.m[2][3] = 0.0f;
	result.m[3][3] = 1.0f;

	return result;
}

Matrix4x4 IdentityMatrix()
{
	Matrix4x4 m = {
		1.0f,0.0f,0.0f,0.0f,
		0.0f,1.0f,0.0f,0.0f,
		0.0f,0.0f,1.0f,0.0f,
		0.0f,0.0f,0.0f,1.0f };
	return m;
}



Matrix4x4 MakeAffineMatrix(Vector3 pos, Vector3 scale, Vector3 angle)
{
	Matrix4x4 result;
	Matrix4x4 scaleMatrix = Scale(scale);
	Matrix4x4 rotationMatrix = Rotation(angle);
	Matrix4x4 translationMatrix = Translation(pos);

	result.m[0][0] = scaleMatrix.m[0][0] * rotationMatrix.m[0][0];
	result.m[0][1] = scaleMatrix.m[0][0] * rotationMatrix.m[0][1];
	result.m[0][2] = scaleMatrix.m[0][0] * rotationMatrix.m[0][2];

	result.m[1][0] = scaleMatrix.m[1][1] * rotationMatrix.m[1][0];
	result.m[1][1] = scaleMatrix.m[1][1] * rotationMatrix.m[1][1];
	result.m[1][2] = scaleMatrix.m[1][1] * rotationMatrix.m[1][2];

	result.m[2][0] = scaleMatrix.m[2][2] * rotationMatrix.m[2][0];
	result.m[2][1] = scaleMatrix.m[2][2] * rotationMatrix.m[2][1];
	result.m[2][2] = scaleMatrix.m[2][2] * rotationMatrix.m[2][2];

	result.m[3][0] = translationMatrix.m[3][0];
	result.m[3][1] = translationMatrix.m[3][1];
	result.m[3][2] = translationMatrix.m[3][2];
	result.m[3][3] = 1.0f;

	return result;
}

Matrix4x4 MakeAffineMatrix(Vector3 pos, Vector3 scale, Quaternion rotate)
{
	Matrix4x4 result{};

	// Quaternionから回転行列の要素を計算
	float xx = rotate.x * rotate.x;
	float yy = rotate.y * rotate.y;
	float zz = rotate.z * rotate.z;
	float xy = rotate.x * rotate.y;
	float xz = rotate.x * rotate.z;
	float yz = rotate.y * rotate.z;
	float wx = rotate.w * rotate.x;
	float wy = rotate.w * rotate.y;
	float wz = rotate.w * rotate.z;

	// 回転行列 × スケール行列を合成
	result.m[0][0] = (1.0f - 2.0f * (yy + zz)) * scale.x;
	result.m[0][1] = (2.0f * (xy + wz)) * scale.x;
	result.m[0][2] = (2.0f * (xz - wy)) * scale.x;
	result.m[0][3] = 0.0f;

	result.m[1][0] = (2.0f * (xy - wz)) * scale.y;
	result.m[1][1] = (1.0f - 2.0f * (xx + zz)) * scale.y;
	result.m[1][2] = (2.0f * (yz + wx)) * scale.y;
	result.m[1][3] = 0.0f;

	result.m[2][0] = (2.0f * (xz + wy)) * scale.z;
	result.m[2][1] = (2.0f * (yz - wx)) * scale.z;
	result.m[2][2] = (1.0f - 2.0f * (xx + yy)) * scale.z;
	result.m[2][3] = 0.0f;

	// 平行移動
	result.m[3][0] = pos.x;
	result.m[3][1] = pos.y;
	result.m[3][2] = pos.z;
	result.m[3][3] = 1.0f;

	return result;
}

Transform DecomposeMatrix(const Matrix4x4& m) {
    Transform result;
    // 平行移動成分の抽出
    result.translate.x = m.m[3][0];
    result.translate.y = m.m[3][1];
    result.translate.z = m.m[3][2];

    // スケール成分の抽出
    result.scale.x = std::sqrt(m.m[0][0] * m.m[0][0] + m.m[0][1] * m.m[0][1] + m.m[0][2] * m.m[0][2]);
    result.scale.y = std::sqrt(m.m[1][0] * m.m[1][0] + m.m[1][1] * m.m[1][1] + m.m[1][2] * m.m[1][2]);
    result.scale.z = std::sqrt(m.m[2][0] * m.m[2][0] + m.m[2][1] * m.m[2][1] + m.m[2][2] * m.m[2][2]);

    // 回転行列成分の抽出（スケールを分離）
    Matrix4x4 rot = m;
    if (result.scale.x != 0.0f) { rot.m[0][0] /= result.scale.x; rot.m[0][1] /= result.scale.x; rot.m[0][2] /= result.scale.x; }
    if (result.scale.y != 0.0f) { rot.m[1][0] /= result.scale.y; rot.m[1][1] /= result.scale.y; rot.m[1][2] /= result.scale.y; }
    if (result.scale.z != 0.0f) { rot.m[2][0] /= result.scale.z; rot.m[2][1] /= result.scale.z; rot.m[2][2] /= result.scale.z; }

    // オイラー角（X->Y->Zの順でMakeAffineMatrixが作成されていると仮定）
    result.rotate.y = std::asin(-std::clamp(rot.m[0][2], -1.0f, 1.0f));

    if (std::abs(rot.m[0][2]) < 0.99999f) {
        result.rotate.x = std::atan2(rot.m[1][2], rot.m[2][2]);
        result.rotate.z = std::atan2(rot.m[0][1], rot.m[0][0]);
    } else {
        result.rotate.x = std::atan2(-rot.m[2][1], rot.m[1][1]);
        result.rotate.z = 0.0f;
    }

    return result;
}

Matrix4x4 MakeAffineMatrix(Matrix4x4 translationMatrix, Vector3 scale, Matrix4x4 rotationMatrix)
{
	Matrix4x4 result;
	Matrix4x4 scaleMatrix = Scale(scale);

	result.m[0][0] = scaleMatrix.m[0][0] * rotationMatrix.m[0][0];
	result.m[0][1] = scaleMatrix.m[0][0] * rotationMatrix.m[0][1];
	result.m[0][2] = scaleMatrix.m[0][0] * rotationMatrix.m[0][2];

	result.m[1][0] = scaleMatrix.m[1][1] * rotationMatrix.m[1][0];
	result.m[1][1] = scaleMatrix.m[1][1] * rotationMatrix.m[1][1];
	result.m[1][2] = scaleMatrix.m[1][1] * rotationMatrix.m[1][2];

	result.m[2][0] = scaleMatrix.m[2][2] * rotationMatrix.m[2][0];
	result.m[2][1] = scaleMatrix.m[2][2] * rotationMatrix.m[2][1];
	result.m[2][2] = scaleMatrix.m[2][2] * rotationMatrix.m[2][2];

	result.m[3][0] = translationMatrix.m[3][0];
	result.m[3][1] = translationMatrix.m[3][1];
	result.m[3][2] = translationMatrix.m[3][2];
	result.m[3][3] = 1.0f;

	return result;
}

Matrix4x4 MakeOrthographicMatrix(float l, float r, float t, float b, float zn, float zf)
{
	Matrix4x4 result;
	result.m[0][0] = 2.0f / (r - l);
	result.m[1][1] = 2.0f / (t - b);
	result.m[2][2] = 1.0f / (zf - zn);
	result.m[3][0] = (l + r) / (l - r);
	result.m[3][1] = (t + b) / (b - t);
	result.m[3][2] = (zn) / (zn - zf);
	result.m[3][3] = 1.0f;
	return result;
}

Matrix4x4 MakePerspectiveFovMatrix(float fovY, float a, float zn, float zf)
{
	Matrix4x4 result;
	result.m[0][0] = static_cast<float>(1.0f / a * (1.0f / tan(fovY / 2.0f)));
	result.m[1][1] = static_cast<float>(1.0f / tan(fovY / 2.0f));
	result.m[2][2] = static_cast<float>(zf / (zf - zn));
	result.m[2][3] = 1.0f;
	result.m[3][2] = static_cast<float>(-zn * zf / (zf - zn));
	return result;
}

Matrix4x4 MakeViewPortMatrix(float width, float height, float left, float top, float minD, float maxD)
{
	Matrix4x4 result;
	result.m[0][0] = width / 2.0f;
	result.m[1][1] = -height / 2.0f;
	result.m[2][2] = maxD - minD;
	result.m[3][0] = left + width / 2.0f;
	result.m[3][1] = top + height / 2.0f;
	result.m[3][2] = minD;
	result.m[3][3] = 1.0f;
	return result;
}

Matrix4x4 MakeLookAtLH(const Vector3& eye, const Vector3& target, const Vector3& up){
	Vector3 zAxis = Normalize(target - eye);
	Vector3 xAxis = Normalize(Cross(up, zAxis));
	Vector3 yAxis = Cross(zAxis, xAxis);

	Matrix4x4 result; 

	result.m[0][0] = xAxis.x;      result.m[0][1] = yAxis.x;      result.m[0][2] = zAxis.x;      result.m[0][3] = 0.0f;
	result.m[1][0] = xAxis.y;      result.m[1][1] = yAxis.y;      result.m[1][2] = zAxis.y;      result.m[1][3] = 0.0f;
	result.m[2][0] = xAxis.z;      result.m[2][1] = yAxis.z;      result.m[2][2] = zAxis.z;      result.m[2][3] = 0.0f;

	result.m[3][0] = -Dot(xAxis, eye);
	result.m[3][1] = -Dot(yAxis, eye);
	result.m[3][2] = -Dot(zAxis, eye);
	result.m[3][3] = 1.0f;

	return result;
}


AABB GetAABB(Transform transform_,float kWidth,float kHeight) {
	Vector3 worldPos = transform_.translate;
	AABB aabb;
	aabb.min = { worldPos.x - kWidth / 2.0f, worldPos.y - kHeight / 2.0f, worldPos.z - kWidth / 2.0f };
	aabb.max = { worldPos.x + kWidth / 2.0f, worldPos.y + kHeight / 2.0f, worldPos.z + kWidth / 2.0f };

	return aabb;
}

#include <algorithm>

bool CheckRayAABB(const Ray& ray, const AABB& aabb, float& outDistance) {
	float tMin = -FLT_MAX;
	float tMax = FLT_MAX;

	// X軸
	if (abs(ray.direction.x) > 0.0001f) {
		float t1 = (aabb.min.x - ray.origin.x) / ray.direction.x;
		float t2 = (aabb.max.x - ray.origin.x) / ray.direction.x;
		tMin = max(tMin, min(t1, t2));
		tMax = min(tMax, max(t1, t2));
	} else if (ray.origin.x < aabb.min.x || ray.origin.x > aabb.max.x) {
		return false;
	}

	// Y軸
	if (abs(ray.direction.y) > 0.0001f) {
		float t1 = (aabb.min.y - ray.origin.y) / ray.direction.y;
		float t2 = (aabb.max.y - ray.origin.y) / ray.direction.y;
		tMin = max(tMin, min(t1, t2));
		tMax = min(tMax, max(t1, t2));
	} else if (ray.origin.y < aabb.min.y || ray.origin.y > aabb.max.y) {
		return false;
	}

	// Z軸
	if (abs(ray.direction.z) > 0.0001f) {
		float t1 = (aabb.min.z - ray.origin.z) / ray.direction.z;
		float t2 = (aabb.max.z - ray.origin.z) / ray.direction.z;
		tMin = max(tMin, min(t1, t2));
		tMax = min(tMax, max(t1, t2));
	} else if (ray.origin.z < aabb.min.z || ray.origin.z > aabb.max.z) {
		return false;
	}

	if (tMin > tMax || tMax < 0.0f) {
		return false;
	}

	outDistance = tMin > 0.0f ? tMin : tMax;
	return true;
}

Frustum Frustum::FromViewProjection(const Matrix4x4& vp) {
	Frustum f;

	auto normalizePlane = [](Plane& p) {
		float len = std::sqrt(p.normal.x * p.normal.x + p.normal.y * p.normal.y + p.normal.z * p.normal.z);
		if (len > 0.000001f) {
			p.normal.x /= len;
			p.normal.y /= len;
			p.normal.z /= len;
			p.distance /= len;
		}
	};

	// Near plane: Cz >= 0
	f.planes[kNear].normal.x = vp.m[0][2];
	f.planes[kNear].normal.y = vp.m[1][2];
	f.planes[kNear].normal.z = vp.m[2][2];
	f.planes[kNear].distance = vp.m[3][2];
	normalizePlane(f.planes[kNear]);

	// Far plane: Cw - Cz >= 0
	f.planes[kFar].normal.x = vp.m[0][3] - vp.m[0][2];
	f.planes[kFar].normal.y = vp.m[1][3] - vp.m[1][2];
	f.planes[kFar].normal.z = vp.m[2][3] - vp.m[2][2];
	f.planes[kFar].distance = vp.m[3][3] - vp.m[3][2];
	normalizePlane(f.planes[kFar]);

	// Left plane: Cw + Cx >= 0
	f.planes[kLeft].normal.x = vp.m[0][3] + vp.m[0][0];
	f.planes[kLeft].normal.y = vp.m[1][3] + vp.m[1][0];
	f.planes[kLeft].normal.z = vp.m[2][3] + vp.m[2][0];
	f.planes[kLeft].distance = vp.m[3][3] + vp.m[3][0];
	normalizePlane(f.planes[kLeft]);

	// Right plane: Cw - Cx >= 0
	f.planes[kRight].normal.x = vp.m[0][3] - vp.m[0][0];
	f.planes[kRight].normal.y = vp.m[1][3] - vp.m[1][0];
	f.planes[kRight].normal.z = vp.m[2][3] - vp.m[2][0];
	f.planes[kRight].distance = vp.m[3][3] - vp.m[3][0];
	normalizePlane(f.planes[kRight]);

	// Bottom plane: Cw + Cy >= 0
	f.planes[kBottom].normal.x = vp.m[0][3] + vp.m[0][1];
	f.planes[kBottom].normal.y = vp.m[1][3] + vp.m[1][1];
	f.planes[kBottom].normal.z = vp.m[2][3] + vp.m[2][1];
	f.planes[kBottom].distance = vp.m[3][3] + vp.m[3][1];
	normalizePlane(f.planes[kBottom]);

	// Top plane: Cw - Cy >= 0
	f.planes[kTop].normal.x = vp.m[0][3] - vp.m[0][1];
	f.planes[kTop].normal.y = vp.m[1][3] - vp.m[1][1];
	f.planes[kTop].normal.z = vp.m[2][3] - vp.m[2][1];
	f.planes[kTop].distance = vp.m[3][3] - vp.m[3][1];
	normalizePlane(f.planes[kTop]);

	return f;
}

bool Frustum::ContainsSphere(const Vector3& center, float radius) const {
	for (int i = 0; i < kPlaneCount; ++i) {
		if (planes[i].DotCoord(center) < -radius) {
			return false; // 視錐台の完全に外側
		}
	}
	return true;
}

bool Frustum::ContainsAABB(const Vector3& min, const Vector3& max) const {
	for (int i = 0; i < kPlaneCount; ++i) {
		// 平面法線方向で最もプラス側にある頂点を判定
		Vector3 p;
		p.x = (planes[i].normal.x >= 0.0f) ? max.x : min.x;
		p.y = (planes[i].normal.y >= 0.0f) ? max.y : min.y;
		p.z = (planes[i].normal.z >= 0.0f) ? max.z : min.z;

		if (planes[i].DotCoord(p) < 0.0f) {
			return false; // 最も内側の頂点すら平面の外側にあるため、完全に外
		}
	}
	return true;
}

AABB TransformAABB(const AABB& aabb, const Matrix4x4& worldMatrix) {
	Vector3 center = {
		(aabb.min.x + aabb.max.x) * 0.5f,
		(aabb.min.y + aabb.max.y) * 0.5f,
		(aabb.min.z + aabb.max.z) * 0.5f
	};
	Vector3 extents = {
		(aabb.max.x - aabb.min.x) * 0.5f,
		(aabb.max.y - aabb.min.y) * 0.5f,
		(aabb.max.z - aabb.min.z) * 0.5f
	};

	Vector3 worldCenter = TransformMatrix(center, worldMatrix);

	Vector3 worldExtents = {
		std::abs(worldMatrix.m[0][0]) * extents.x + std::abs(worldMatrix.m[1][0]) * extents.y + std::abs(worldMatrix.m[2][0]) * extents.z,
		std::abs(worldMatrix.m[0][1]) * extents.x + std::abs(worldMatrix.m[1][1]) * extents.y + std::abs(worldMatrix.m[2][1]) * extents.z,
		std::abs(worldMatrix.m[0][2]) * extents.x + std::abs(worldMatrix.m[1][2]) * extents.y + std::abs(worldMatrix.m[2][2]) * extents.z
	};

	AABB result;
	result.min = { worldCenter.x - worldExtents.x, worldCenter.y - worldExtents.y, worldCenter.z - worldExtents.z };
	result.max = { worldCenter.x + worldExtents.x, worldCenter.y + worldExtents.y, worldCenter.z + worldExtents.z };
	return result;
}

BoundingSphere TransformBoundingSphere(const BoundingSphere& sphere, const Matrix4x4& worldMatrix) {
	BoundingSphere result;
	result.center = TransformMatrix(sphere.center, worldMatrix);

	float sx = std::sqrt(worldMatrix.m[0][0] * worldMatrix.m[0][0] + worldMatrix.m[0][1] * worldMatrix.m[0][1] + worldMatrix.m[0][2] * worldMatrix.m[0][2]);
	float sy = std::sqrt(worldMatrix.m[1][0] * worldMatrix.m[1][0] + worldMatrix.m[1][1] * worldMatrix.m[1][1] + worldMatrix.m[1][2] * worldMatrix.m[1][2]);
	float sz = std::sqrt(worldMatrix.m[2][0] * worldMatrix.m[2][0] + worldMatrix.m[2][1] * worldMatrix.m[2][1] + worldMatrix.m[2][2] * worldMatrix.m[2][2]);
	float maxScale = (std::max)({ sx, sy, sz });

	result.radius = sphere.radius * maxScale;
	return result;
}

// =================================================================
// スカラー数学・角度・数値ユーティリティ
// =================================================================

float ToRadian(float degree) {
	return degree * kDeg2Rad;
}

float ToDegree(float radian) {
	return radian * kRad2Deg;
}

float Clamp(float value, float minVal, float maxVal) {
	if (value < minVal) return minVal;
	if (value > maxVal) return maxVal;
	return value;
}

float Saturate(float value) {
	return Clamp(value, 0.0f, 1.0f);
}

float Sign(float value) {
	if (value > 0.0f) return 1.0f;
	if (value < 0.0f) return -1.0f;
	return 0.0f;
}

bool Approximately(float a, float b, float epsilon) {
	return std::abs(a - b) <= epsilon;
}

float WrapAngle(float angleRad) {
	angleRad = std::fmod(angleRad + kPi, k2Pi);
	if (angleRad < 0.0f) angleRad += k2Pi;
	return angleRad - kPi;
}

float LerpAngle(float a, float b, float t) {
	float diff = WrapAngle(b - a);
	return a + diff * t;
}

// =================================================================
// 二次元ベクトル (Vector2) の計算
// =================================================================

Vector2 AddVector2(const Vector2& v1, const Vector2& v2) {
	return { v1.x + v2.x, v1.y + v2.y };
}

Vector2 SubtractVector2(const Vector2& v1, const Vector2& v2) {
	return { v1.x - v2.x, v1.y - v2.y };
}

Vector2 MultiplyVector2(const Vector2& v1, const Vector2& v2) {
	return { v1.x * v2.x, v1.y * v2.y };
}

Vector2 ScalarMultiply(const Vector2& v, float s) {
	return { v.x * s, v.y * s };
}

float Length(const Vector2& v) {
	return std::sqrt(v.x * v.x + v.y * v.y);
}

float LengthSq(const Vector2& v) {
	return v.x * v.x + v.y * v.y;
}

Vector2 Normalize(const Vector2& v) {
	float len = Length(v);
	if (len != 0.0f) {
		return { v.x / len, v.y / len };
	}
	return { 0.0f, 0.0f };
}

float Dot(const Vector2& v1, const Vector2& v2) {
	return v1.x * v2.x + v1.y * v2.y;
}

float Cross(const Vector2& v1, const Vector2& v2) {
	return v1.x * v2.y - v1.y * v2.x;
}

float Distance(const Vector2& v1, const Vector2& v2) {
	return Length(SubtractVector2(v1, v2));
}

float DistanceSq(const Vector2& v1, const Vector2& v2) {
	return LengthSq(SubtractVector2(v1, v2));
}

Vector2 Lerp(const Vector2& a, const Vector2& b, float t) {
	return { a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t };
}

Vector2 RotateVector(const Vector2& v, float angleRad) {
	float c = std::cos(angleRad);
	float s = std::sin(angleRad);
	return { v.x * c - v.y * s, v.x * s + v.y * c };
}

// =================================================================
// 三次元ベクトル (Vector3) 拡張計算
// =================================================================

float LengthSq(const Vector3& v) {
	return v.x * v.x + v.y * v.y + v.z * v.z;
}

float Distance(const Vector3& v1, const Vector3& v2) {
	return Length(SubtractVector3(v1, v2));
}

float DistanceSq(const Vector3& v1, const Vector3& v2) {
	return LengthSq(SubtractVector3(v1, v2));
}

Vector3 Reflect(const Vector3& incident, const Vector3& normal) {
	float d = Dot(incident, normal);
	return SubtractVector3(incident, ScalarMultiply(normal, 2.0f * d));
}

Vector3 Project(const Vector3& v, const Vector3& normal) {
	float lenSq = LengthSq(normal);
	if (lenSq == 0.0f) return { 0.0f, 0.0f, 0.0f };
	float d = Dot(v, normal);
	return ScalarMultiply(normal, d / lenSq);
}

Vector3 Reject(const Vector3& v, const Vector3& normal) {
	return SubtractVector3(v, Project(v, normal));
}

float Angle(const Vector3& from, const Vector3& to) {
	float lenProd = Length(from) * Length(to);
	if (lenProd == 0.0f) return 0.0f;
	float cosVal = Clamp(Dot(from, to) / lenProd, -1.0f, 1.0f);
	return std::acos(cosVal);
}

Vector3 ClampLength(const Vector3& v, float maxLength) {
	float sq = LengthSq(v);
	if (sq > maxLength * maxLength && sq > 0.0f) {
		float scale = maxLength / std::sqrt(sq);
		return ScalarMultiply(v, scale);
	}
	return v;
}

Vector3 Min(const Vector3& a, const Vector3& b) {
	return { (std::min)(a.x, b.x), (std::min)(a.y, b.y), (std::min)(a.z, b.z) };
}

Vector3 Max(const Vector3& a, const Vector3& b) {
	return { (std::max)(a.x, b.x), (std::max)(a.y, b.y), (std::max)(a.z, b.z) };
}

Vector3 Clamp(const Vector3& v, const Vector3& minVal, const Vector3& maxVal) {
	return {
		Clamp(v.x, minVal.x, maxVal.x),
		Clamp(v.y, minVal.y, maxVal.y),
		Clamp(v.z, minVal.z, maxVal.z)
	};
}

Vector3 TransformNormal(const Vector3& v, const Matrix4x4& m) {
	Vector3 result{};
	result.x = v.x * m.m[0][0] + v.y * m.m[1][0] + v.z * m.m[2][0];
	result.y = v.x * m.m[0][1] + v.y * m.m[1][1] + v.z * m.m[2][1];
	result.z = v.x * m.m[0][2] + v.y * m.m[1][2] + v.z * m.m[2][2];
	return result;
}

// =================================================================
// 補間 (Vector4 / 曲線)
// =================================================================

Vector4 Lerp(const Vector4& a, const Vector4& b, float t) {
	return {
		a.x + (b.x - a.x) * t,
		a.y + (b.y - a.y) * t,
		a.z + (b.z - a.z) * t,
		a.w + (b.w - a.w) * t
	};
}

Vector3 Bezier(const Vector3& p0, const Vector3& p1, const Vector3& p2, float t) {
	float u = 1.0f - t;
	float tt = t * t;
	float uu = u * u;
	return p0 * uu + p1 * (2.0f * u * t) + p2 * tt;
}

Vector3 Bezier(const Vector3& p0, const Vector3& p1, const Vector3& p2, const Vector3& p3, float t) {
	float u = 1.0f - t;
	float tt = t * t;
	float uu = u * u;
	float uuu = uu * u;
	float ttt = tt * t;
	return p0 * uuu + p1 * (3.0f * uu * t) + p2 * (3.0f * u * tt) + p3 * ttt;
}

Vector3 CatmullRom(const Vector3& p0, const Vector3& p1, const Vector3& p2, const Vector3& p3, float t) {
	float t2 = t * t;
	float t3 = t2 * t;

	Vector3 a = p1 * 2.0f;
	Vector3 b = (p2 - p0) * t;
	Vector3 c = (p0 * 2.0f - p1 * 5.0f + p2 * 4.0f - p3) * t2;
	Vector3 d = (-p0 + p1 * 3.0f - p2 * 3.0f + p3) * t3;

	return (a + b + c + d) * 0.5f;
}

// =================================================================
// イージング関数 (Easing Functions)
// =================================================================

float EaseInQuad(float t) {
	return t * t;
}

float EaseOutQuad(float t) {
	return 1.0f - (1.0f - t) * (1.0f - t);
}

float EaseInOutQuad(float t) {
	return t < 0.5f ? 2.0f * t * t : 1.0f - std::pow(-2.0f * t + 2.0f, 2.0f) / 2.0f;
}

float EaseInCubic(float t) {
	return t * t * t;
}

float EaseOutCubic(float t) {
	return 1.0f - std::pow(1.0f - t, 3.0f);
}

float EaseInOutCubic(float t) {
	return t < 0.5f ? 4.0f * t * t * t : 1.0f - std::pow(-2.0f * t + 2.0f, 3.0f) / 2.0f;
}

float EaseInBack(float t, float s) {
	return (s + 1.0f) * t * t * t - s * t * t;
}

float EaseOutBack(float t, float s) {
	float tMinus1 = t - 1.0f;
	return 1.0f + (s + 1.0f) * tMinus1 * tMinus1 * tMinus1 + s * tMinus1 * tMinus1;
}

float EaseInOutBack(float t, float s) {
	float c2 = s * 1.525f;
	if (t < 0.5f) {
		return (std::pow(2.0f * t, 2.0f) * ((c2 + 1.0f) * 2.0f * t - c2)) / 2.0f;
	} else {
		return (std::pow(2.0f * t - 2.0f, 2.0f) * ((c2 + 1.0f) * (t * 2.0f - 2.0f) + c2) + 2.0f) / 2.0f;
	}
}

float EaseOutBounce(float t) {
	const float n1 = 7.5625f;
	const float d1 = 2.75f;
	if (t < 1.0f / d1) {
		return n1 * t * t;
	} else if (t < 2.0f / d1) {
		t -= 1.5f / d1;
		return n1 * t * t + 0.75f;
	} else if (t < 2.5f / d1) {
		t -= 2.25f / d1;
		return n1 * t * t + 0.9375f;
	} else {
		t -= 2.625f / d1;
		return n1 * t * t + 0.984375f;
	}
}

float SmoothStep(float edge0, float edge1, float x) {
	if (edge0 == edge1) return edge0 <= x ? 1.0f : 0.0f;
	float t = Clamp((x - edge0) / (edge1 - edge0), 0.0f, 1.0f);
	return t * t * (3.0f - 2.0f * t);
}

// =================================================================
// クォータニオン (Quaternion) の計算
// =================================================================

Quaternion IdentityQuaternion() {
	return { 0.0f, 0.0f, 0.0f, 1.0f };
}

Quaternion Multiply(const Quaternion& lhs, const Quaternion& rhs) {
	Quaternion result;
	result.w = lhs.w * rhs.w - lhs.x * rhs.x - lhs.y * rhs.y - lhs.z * rhs.z;
	result.x = lhs.w * rhs.x + lhs.x * rhs.w + lhs.y * rhs.z - lhs.z * rhs.y;
	result.y = lhs.w * rhs.y - lhs.x * rhs.z + lhs.y * rhs.w + lhs.z * rhs.x;
	result.z = lhs.w * rhs.z + lhs.x * rhs.y - lhs.y * rhs.x + lhs.z * rhs.w;
	return result;
}

Quaternion Conjugate(const Quaternion& q) {
	return { -q.x, -q.y, -q.z, q.w };
}

Quaternion Inverse(const Quaternion& q) {
	float lenSq = q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w;
	if (lenSq != 0.0f) {
		float inv = 1.0f / lenSq;
		return { -q.x * inv, -q.y * inv, -q.z * inv, q.w * inv };
	}
	return { 0.0f, 0.0f, 0.0f, 0.0f };
}

Quaternion Normalize(const Quaternion& q) {
	float len = std::sqrt(q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w);
	if (len != 0.0f) {
		return { q.x / len, q.y / len, q.z / len, q.w / len };
	}
	return { 0.0f, 0.0f, 0.0f, 1.0f };
}

float Dot(const Quaternion& a, const Quaternion& b) {
	return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
}

Vector3 RotateVector(const Vector3& v, const Quaternion& q) {
	Vector3 qv = { q.x, q.y, q.z };
	Vector3 t = Cross(qv, v);
	t.x *= 2.0f; t.y *= 2.0f; t.z *= 2.0f;
	Vector3 crossQvT = Cross(qv, t);
	return {
		v.x + q.w * t.x + crossQvT.x,
		v.y + q.w * t.y + crossQvT.y,
		v.z + q.w * t.z + crossQvT.z
	};
}

Quaternion MakeRotateAxisAngle(const Vector3& axis, float angleRad) {
	Vector3 normAxis = Normalize(axis);
	float halfAngle = angleRad * 0.5f;
	float sinHalf = std::sin(halfAngle);
	return {
		normAxis.x * sinHalf,
		normAxis.y * sinHalf,
		normAxis.z * sinHalf,
		std::cos(halfAngle)
	};
}

Matrix4x4 MakeRotateMatrix(const Quaternion& q) {
	Matrix4x4 result{};
	float xx = q.x * q.x;
	float yy = q.y * q.y;
	float zz = q.z * q.z;
	float xy = q.x * q.y;
	float xz = q.x * q.z;
	float yz = q.y * q.z;
	float wx = q.w * q.x;
	float wy = q.w * q.y;
	float wz = q.w * q.z;

	result.m[0][0] = 1.0f - 2.0f * (yy + zz);
	result.m[0][1] = 2.0f * (xy + wz);
	result.m[0][2] = 2.0f * (xz - wy);
	result.m[0][3] = 0.0f;

	result.m[1][0] = 2.0f * (xy - wz);
	result.m[1][1] = 1.0f - 2.0f * (xx + zz);
	result.m[1][2] = 2.0f * (yz + wx);
	result.m[1][3] = 0.0f;

	result.m[2][0] = 2.0f * (xz + wy);
	result.m[2][1] = 2.0f * (yz - wx);
	result.m[2][2] = 1.0f - 2.0f * (xx + yy);
	result.m[2][3] = 0.0f;

	result.m[3][0] = 0.0f;
	result.m[3][1] = 0.0f;
	result.m[3][2] = 0.0f;
	result.m[3][3] = 1.0f;

	return result;
}

// =================================================================
// 衝突判定 (Collision)
// =================================================================

bool CheckSphereSphere(const BoundingSphere& a, const BoundingSphere& b) {
	float distSq = DistanceSq(a.center, b.center);
	float radiusSum = a.radius + b.radius;
	return distSq <= (radiusSum * radiusSum);
}

bool CheckAABBAABB(const AABB& a, const AABB& b) {
	return (a.min.x <= b.max.x && a.max.x >= b.min.x) &&
	       (a.min.y <= b.max.y && a.max.y >= b.min.y) &&
	       (a.min.z <= b.max.z && a.max.z >= b.min.z);
}

bool CheckSphereAABB(const BoundingSphere& sphere, const AABB& aabb) {
	Vector3 closest = ClosestPointOnAABB(sphere.center, aabb);
	return DistanceSq(sphere.center, closest) <= (sphere.radius * sphere.radius);
}

bool CheckRaySphere(const Ray& ray, const BoundingSphere& sphere, float& outDistance) {
	Vector3 m = SubtractVector3(ray.origin, sphere.center);
	float b = Dot(m, ray.direction);
	float c = Dot(m, m) - sphere.radius * sphere.radius;

	// レイの原点が球の外側にあり、球から離れる方向を向いている場合は外れ
	if (c > 0.0f && b > 0.0f) {
		return false;
	}

	float discr = b * b - c;
	if (discr < 0.0f) {
		return false;
	}

	float t = -b - std::sqrt(discr);
	if (t < 0.0f) {
		t = 0.0f; // 球の内部にレイ原点がある場合
	}
	outDistance = t;
	return true;
}

bool CheckRayPlane(const Ray& ray, const Plane& plane, float& outDistance) {
	float denom = Dot(plane.normal, ray.direction);
	// 平面とレイがほぼ平行な場合は交差しない
	if (std::abs(denom) < 1e-6f) {
		return false;
	}
	float t = -(Dot(plane.normal, ray.origin) + plane.distance) / denom;
	if (t < 0.0f) {
		return false;
	}
	outDistance = t;
	return true;
}

// =================================================================
// 最近接点計算 (Closest Point)
// =================================================================

Vector3 ClosestPointOnAABB(const Vector3& point, const AABB& aabb) {
	return {
		Clamp(point.x, aabb.min.x, aabb.max.x),
		Clamp(point.y, aabb.min.y, aabb.max.y),
		Clamp(point.z, aabb.min.z, aabb.max.z)
	};
}

Vector3 ClosestPointOnSegment(const Vector3& point, const Vector3& segA, const Vector3& segB) {
	Vector3 ab = SubtractVector3(segB, segA);
	float abLenSq = LengthSq(ab);
	if (abLenSq == 0.0f) {
		return segA; // 縮退した線分(点)
	}
	Vector3 ap = SubtractVector3(point, segA);
	float t = Clamp(Dot(ap, ab) / abLenSq, 0.0f, 1.0f);
	return AddVector3(segA, ScalarMultiply(ab, t));
}

// =================================================================
// カメラ・スクリーン・ワールド座標変換
// =================================================================

Ray ScreenToWorldRay(const Vector2& screenPos, const Matrix4x4& viewMatrix, const Matrix4x4& projMatrix, float screenWidth, float screenHeight) {
	// 画面座標 (0..width, 0..height) を NDC (-1..1, 1..-1) に変換
	float ndcX = (screenPos.x / screenWidth) * 2.0f - 1.0f;
	float ndcY = 1.0f - (screenPos.y / screenHeight) * 2.0f;

	// View-Projection 行列の逆行列
	Matrix4x4 vp = MultiplyMatrix4x4(viewMatrix, projMatrix);
	Matrix4x4 invVP = Inverse(vp);

	// NDC上の近クリップ点 (z=0) と遠クリップ点 (z=1) をワールド空間へ逆変換
	Vector3 nearPoint = TransformMatrix({ ndcX, ndcY, 0.0f }, invVP);
	Vector3 farPoint  = TransformMatrix({ ndcX, ndcY, 1.0f }, invVP);

	Ray ray;
	ray.origin = nearPoint;
	ray.direction = Normalize(SubtractVector3(farPoint, nearPoint));
	return ray;
}

Vector2 WorldToScreenPoint(const Vector3& worldPos, const Matrix4x4& viewMatrix, const Matrix4x4& projMatrix, float screenWidth, float screenHeight) {
	Matrix4x4 vp = MultiplyMatrix4x4(viewMatrix, projMatrix);
	Vector3 clip = TransformMatrix(worldPos, vp);

	// クリップ座標 (-1..1) からスクリーン座標 (0..width, 0..height) へ
	float screenX = (clip.x + 1.0f) * 0.5f * screenWidth;
	float screenY = (1.0f - clip.y) * 0.5f * screenHeight;

	return { screenX, screenY };
}