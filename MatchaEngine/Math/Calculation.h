#pragma once
#include "../Core/VariableTypes.h"

// =================================================================
// 数学定数
// =================================================================
constexpr float kPi = 3.14159265358979323846f;
constexpr float k2Pi = kPi * 2.0f;
constexpr float kPiOver2 = kPi * 0.5f;
constexpr float kDeg2Rad = kPi / 180.0f;
constexpr float kRad2Deg = 180.0f / kPi;

// =================================================================
// スカラー数学・角度・数値ユーティリティ
// =================================================================

/// <summary>
/// 度(Degree)からラジアン(Radian)に変換します。
/// </summary>
float ToRadian(float degree);

/// <summary>
/// ラジアン(Radian)から度(Degree)に変換します。
/// </summary>
float ToDegree(float radian);

/// <summary>
/// 値を指定された範囲内に制限(クランプ)します。
/// </summary>
float Clamp(float value, float min, float max);

/// <summary>
/// 値を 0.0f ～ 1.0f の範囲に制限します。
/// </summary>
float Saturate(float value);

/// <summary>
/// 値の符号を取得します (正なら1.0f, 負なら-1.0f, 0なら0.0f)。
/// </summary>
float Sign(float value);

/// <summary>
/// 2つの浮動小数点数が許容誤差内で等しいか判定します。
/// </summary>
bool Approximately(float a, float b, float epsilon = 1e-5f);

/// <summary>
/// 角度(ラジアン)を [-π, π] の範囲に正規化します。
/// </summary>
float WrapAngle(float angleRad);

/// <summary>
/// 最短経路で2つの角度(ラジアン)を線形補間します。
/// </summary>
float LerpAngle(float a, float b, float t);

// =================================================================
// 二次元ベクトル (Vector2) の計算
// =================================================================

Vector2 AddVector2(const Vector2& v1, const Vector2& v2);
Vector2 SubtractVector2(const Vector2& v1, const Vector2& v2);
Vector2 MultiplyVector2(const Vector2& v1, const Vector2& v2);
Vector2 ScalarMultiply(const Vector2& v, float s);
float Length(const Vector2& v);
float LengthSq(const Vector2& v);
Vector2 Normalize(const Vector2& v);
float Dot(const Vector2& v1, const Vector2& v2);
float Cross(const Vector2& v1, const Vector2& v2);
float Distance(const Vector2& v1, const Vector2& v2);
float DistanceSq(const Vector2& v1, const Vector2& v2);
Vector2 Lerp(const Vector2& a, const Vector2& b, float t);
Vector2 RotateVector(const Vector2& v, float angleRad);

// =================================================================
// 三次元ベクトル (Vector3) の計算
// =================================================================

/// <summary>
/// この関数は、2つのベクトルの和を計算します。
/// </summary>
Vector3 AddVector3(Vector3 v1, Vector3 v2);

/// <summary>
/// この関数は、2つのベクトルの差を計算します。
/// </summary>
Vector3 SubtractVector3(Vector3 v1, Vector3 v2);

/// <summary>
/// この関数は二つのベクトルの積を計算します
/// </summary>
Vector3 MultiplyVector3(Vector3 v1, Vector3 v2);

/// <summary>
/// この関数は、指定されたスカラー倍をベクトルの各要素に適用します。
/// </summary>
Vector3 ScalarMultiply(Vector3 v, float s);

/// <summary>
/// この関数は、ベクトルの長さを計算します。
/// </summary>
float Length(Vector3 v);

/// <summary>
/// この関数は、ベクトルの長さの2乗を計算します(高速な距離比較向け)。
/// </summary>
float LengthSq(const Vector3& v);

/// <summary>
/// この関数は、ベクトルを正規化します。
/// </summary>
Vector3 Normalize(Vector3 v);

/// <summary>
/// この関数は、2つのベクトルの内積を計算します。
/// </summary>
float Dot(Vector3 v1, Vector3 v2);

/// <summary>
/// この関数は、2つのベクトルのクロス積(外積)を計算します。
/// </summary>
Vector3 Cross(const Vector3& v1, const Vector3& v2);

/// <summary>
/// 2点間の距離を計算します。
/// </summary>
float Distance(const Vector3& v1, const Vector3& v2);

/// <summary>
/// 2点間の距離の2乗を計算します(平方根を省く高速距離比較)。
/// </summary>
float DistanceSq(const Vector3& v1, const Vector3& v2);

/// <summary>
/// 入射ベクトルと法線ベクトルから反射ベクトルを計算します。
/// </summary>
Vector3 Reflect(const Vector3& incident, const Vector3& normal);

/// <summary>
/// ベクトル v を指定法線方向 normal に正射影したベクトルを計算します。
/// </summary>
Vector3 Project(const Vector3& v, const Vector3& normal);

/// <summary>
/// ベクトル v から法線方向 normal に平行な成分を除去した垂直成分を計算します。
/// </summary>
Vector3 Reject(const Vector3& v, const Vector3& normal);

/// <summary>
/// 2つのベクトルのなす角(ラジアン)を計算します。
/// </summary>
float Angle(const Vector3& from, const Vector3& to);

/// <summary>
/// ベクトルの長さを maxLength 以下に制限します。
/// </summary>
Vector3 ClampLength(const Vector3& v, float maxLength);

/// <summary>
/// 2つのベクトルの各成分の最小値からなるベクトルを返します。
/// </summary>
Vector3 Min(const Vector3& a, const Vector3& b);

/// <summary>
/// 2つのベクトルの各成分の最大値からなるベクトルを返します。
/// </summary>
Vector3 Max(const Vector3& a, const Vector3& b);

/// <summary>
/// ベクトルの各成分を指定範囲にクランプします。
/// </summary>
Vector3 Clamp(const Vector3& v, const Vector3& min, const Vector3& max);

/// <summary>
/// 行列により法線や方向ベクトルを変換します(平行移動を無視)。
/// </summary>
Vector3 TransformNormal(const Vector3& v, const Matrix4x4& m);

// =================================================================
// 補間 (Lerp / Slerp / 曲線)
// =================================================================

float Lerp(float a, float b, float t);
Vector3 Lerp(const Vector3& a, const Vector3& b, float t);
Vector4 Lerp(const Vector4& a, const Vector4& b, float t);
Quaternion Lerp(const Quaternion& a, const Quaternion& b, float t);
Quaternion Slerp(const Quaternion& a, const Quaternion& b, float t);

// 2次ベジェ曲線補間
Vector3 Bezier(const Vector3& p0, const Vector3& p1, const Vector3& p2, float t);
// 3次ベジェ曲線補間
Vector3 Bezier(const Vector3& p0, const Vector3& p1, const Vector3& p2, const Vector3& p3, float t);
// Catmull-Rom スプライン曲線補間 (p1～p2間を通過)
Vector3 CatmullRom(const Vector3& p0, const Vector3& p1, const Vector3& p2, const Vector3& p3, float t);

// =================================================================
// イージング関数 (Easing Functions)
// =================================================================

float EaseInOutSine(float x);

float EaseInQuad(float t);
float EaseOutQuad(float t);
float EaseInOutQuad(float t);

float EaseInCubic(float t);
float EaseOutCubic(float t);
float EaseInOutCubic(float t);

float EaseInBack(float t, float s = 1.70158f);
float EaseOutBack(float t, float s = 1.70158f);
float EaseInOutBack(float t, float s = 1.70158f);

float EaseOutBounce(float t);

float SmoothStep(float edge0, float edge1, float x);

// =================================================================
// クォータニオン (Quaternion) の計算
// =================================================================

/// <summary>
/// 単位クォータニオン (x=0, y=0, z=0, w=1) を返します。
/// </summary>
Quaternion IdentityQuaternion();

/// <summary>
/// 2つのクォータニオンの積(回転の合成)を計算します。
/// </summary>
Quaternion Multiply(const Quaternion& lhs, const Quaternion& rhs);

/// <summary>
/// 共役クォータニオンを計算します。
/// </summary>
Quaternion Conjugate(const Quaternion& q);

/// <summary>
/// 逆クォータニオンを計算します。
/// </summary>
Quaternion Inverse(const Quaternion& q);

/// <summary>
/// クォータニオンを正規化します。
/// </summary>
Quaternion Normalize(const Quaternion& q);

/// <summary>
/// クォータニオンの内積を計算します。
/// </summary>
float Dot(const Quaternion& a, const Quaternion& b);

/// <summary>
/// クォータニオンによってベクトルを回転させます。
/// </summary>
Vector3 RotateVector(const Vector3& v, const Quaternion& q);

/// <summary>
/// 任意の回転軸と回転角(ラジアン)からクォータニオンを作成します。
/// </summary>
Quaternion MakeRotateAxisAngle(const Vector3& axis, float angleRad);

/// <summary>
/// クォータニオンから4x4回転行列を作成します。
/// </summary>
Matrix4x4 MakeRotateMatrix(const Quaternion& q);

// =================================================================
// 行列の計算
// =================================================================

/// <summary>
/// この関数は、2つの行列の和を計算します。
/// </summary>
/// <param name="m1">行列1</param>
/// <param name="m2">行列2</param>
/// <returns>2つの行列の和</returns>
Matrix4x4 AddMatrix4x4(Matrix4x4 m1, Matrix4x4 m2);


/// <summary>
/// この関数は、2つの行列の積を計算します。
/// </summary>
/// <param name="m1">行列1</param>
/// <param name="m2">行列2</param>
/// <returns>2つの行列の積</returns>
Matrix4x4 MultiplyMatrix4x4(Matrix4x4 m1, Matrix4x4 m2);

/// <summary>
/// この関数は、行列にスカラー倍を適用します。
/// </summary>
/// <param name="m">行列</param>
/// <param name="s">スカラー値</param>
/// <returns>スカラー倍された行列</returns>
Matrix4x4 ScaleMultiplyMatrix4x4(Matrix4x4 m, float s);

/// <summary>
/// この関数は、行列の転置を計算します。
/// </summary>
/// <param name="m">行列</param>
/// <returns>転置された行列</returns>
Matrix4x4 TransposeMatrix4x4(Matrix4x4 m);




/// <summary>
/// この関数は、行列の逆行列を計算します。
/// </summary>
/// <param name="m">行列</param>
/// <returns>逆行列</returns>
Matrix4x4 Inverse(Matrix4x4 m);

/// <summary>
/// この関数は、ベクトルを行列で変換します。
/// </summary>
/// <param name="v">ベクトル</param>
/// <param name="m">行列</param>
/// <returns>変換されたベクトル</returns>
Vector3 TransformMatrix(const Vector3& v, const Matrix4x4& m);


/// <summary>
/// 単位行列を出します
/// </summary>
Matrix4x4 IdentityMatrix();

///アフィン行列

/// <summary>
/// この関数は、行列をスケーリングします。
/// </summary>
/// <param name="scale">スケーリング値</param>
/// <returns>スケーリング行列</returns>
Matrix4x4 Scale(Vector3 scale);

/// <summary>
/// この関数は、X軸を中心に回転する行列を計算します。
/// </summary>
/// <param name="angle">回転角度</param>
/// <returns>回転行列</returns>
Matrix4x4 RotationX(float angle);

/// <summary>
/// この関数は、Y軸を中心に回転する行列を計算します。
/// </summary>
/// <param name="angle">回転角度</param>
/// <returns>回転行列</returns>
Matrix4x4 RotationY(float angle);

/// <summary>
/// この関数は、Z軸を中心に回転する行列を計算します。
/// </summary>
/// <param name="angle">回転角度</param>
/// <returns>回転行列</returns>
Matrix4x4 RotationZ(float angle);

/// <summary>
/// この関数は、各軸(X, Y, Z)の回転角度から回転行列を計算します。
/// </summary>
/// <param name="angle">各軸の回転角度(X, Y, Z)</param>
/// <returns>回転行列</returns>
Matrix4x4 Rotation(Vector3 angle);

/// <summary>
/// この関数は、平行移動行列を作成します。
/// </summary>
/// <param name="pos">平行移動量</param>
/// <returns>平行移動行列</returns>
Matrix4x4 Translation(Vector3 pos);

Vector3 MakeWorldPos(PolarCoordinates pos);

Matrix4x4 MakeLookAtMatrix(
    const Vector3& eye,     // カメラの位置
    const Vector3& target,  // 見たい場所
    const Vector3& up       // 上方向ベクトル（たいてい {0,1,0}）
);

/// <summary>
/// この関数は、アフィン行列を作成します。
/// </summary>
/// <param name="pos">位置</param>
/// <param name="scale">スケーリング値</param>
/// <param name="angle">回転角度</param>
/// <returns>アフィン行列</returns>
Matrix4x4 MakeAffineMatrix(Vector3 pos, Vector3 scale, Vector3 angle);

/// <summary>
/// 4x4行列からスケール、回転(オイラー角)、平行移動を抽出します。
/// </summary>
Transform DecomposeMatrix(const Matrix4x4& m);

Matrix4x4 MakeAffineMatrix(Vector3 pos, Vector3 scale, Quaternion rotate);


Matrix4x4 MakeAffineMatrix(Matrix4x4 translationMatrix, Vector3 scale, Matrix4x4 rotationMatrix);

/// <summary>
/// この関数は、正射影行列を作成します。
/// </summary>
/// <param name="l">左</param>
/// <param name="r">右</param>
/// <param name="t">上</param>
/// <param name="b">下</param>
/// <param name="zn">近クリップ面</param>
/// <param name="zf">遠クリップ面</param>
/// <returns>正射影行列</returns>
Matrix4x4 MakeOrthographicMatrix(float l, float r, float t, float b, float zn, float zf);

/// <summary>
/// この関数は、透視投影行列を作成します。
/// </summary>
/// <param name="fov">視野角</param>
/// <param name="aspect">アスペクト比</param>
/// <param name="zn">近クリップ面</param>
/// <param name="zf">遠クリップ面</param>
/// <returns>透視投影行列</returns>
Matrix4x4 MakePerspectiveFovMatrix(float fovY, float a, float zn, float zf);

/// <summary>
/// この関数は、ビューポート行列を作成します。
/// </summary>
/// <param name="width">幅</param>
/// <param name="height">高さ</param>
/// <param name="left">左</param>
/// <param name="top">上</param>
/// <param name="minD">最小深度値</param>
/// <param name="maxD">最大深度値</param>
/// <returns>ビューポート行列</returns>
Matrix4x4 MakeViewPortMatrix(float width, float height, float left, float top, float minD, float maxD);

Matrix4x4 MakeLookAtLH(const Vector3& eye, const Vector3& target, const Vector3& up);

AABB GetAABB(Transform transform_, float kWidth, float kHeight);

// レイ(光線)構造体
struct Ray {
	Vector3 origin;
	Vector3 direction;
};

/// <summary>
/// レイとAABBの交差判定を計算します
/// </summary>
/// <param name="ray">レイ</param>
/// <param name="aabb">AABB</param>
/// <param name="outDistance">交差点までの距離(当たった場合のみ有効)</param>
/// <returns>交差していればtrue</returns>
bool CheckRayAABB(const Ray& ray, const AABB& aabb, float& outDistance);

// 平面構造体 (dot(normal, p) + distance = 0, 法線は内側向き)
struct Plane {
	Vector3 normal = { 0.0f, 1.0f, 0.0f };
	float distance = 0.0f;

	float DotCoord(const Vector3& p) const {
		return normal.x * p.x + normal.y * p.y + normal.z * p.z + distance;
	}
};

// 視錐台構造体 (カメラの6クリッピング平面)
struct Frustum {
	enum PlaneIndex {
		kNear = 0,
		kFar,
		kLeft,
		kRight,
		kTop,
		kBottom,
		kPlaneCount = 6
	};

	Plane planes[kPlaneCount];

	// View-Projection行列から視錐台の6平面を抽出 (Gribb-Hartmann法)
	static Frustum FromViewProjection(const Matrix4x4& vp);

	// 判定関数
	bool ContainsSphere(const Vector3& center, float radius) const;
	bool ContainsSphere(const BoundingSphere& sphere) const {
		return ContainsSphere(sphere.center, sphere.radius);
	}

	bool ContainsAABB(const Vector3& min, const Vector3& max) const;
	bool ContainsAABB(const AABB& aabb) const {
		return ContainsAABB(aabb.min, aabb.max);
	}
};

// AABBを行列で変換した後の新たなワールドAABBを算出
AABB TransformAABB(const AABB& aabb, const Matrix4x4& worldMatrix);

// BoundingSphereを行列で変換した後の新たなワールドBoundingSphereを算出
BoundingSphere TransformBoundingSphere(const BoundingSphere& sphere, const Matrix4x4& worldMatrix);

// =================================================================
// 衝突判定 (Collision)
// =================================================================

/// <summary>
/// 2つの球(BoundingSphere)の交差判定を行います。
/// </summary>
bool CheckSphereSphere(const BoundingSphere& a, const BoundingSphere& b);

/// <summary>
/// 2つのAABBの交差判定を行います。
/// </summary>
bool CheckAABBAABB(const AABB& a, const AABB& b);

/// <summary>
/// 球(BoundingSphere)とAABBの交差判定を行います。
/// </summary>
bool CheckSphereAABB(const BoundingSphere& sphere, const AABB& aabb);

/// <summary>
/// レイと球(BoundingSphere)の交差判定を行います。
/// </summary>
bool CheckRaySphere(const Ray& ray, const BoundingSphere& sphere, float& outDistance);

/// <summary>
/// レイと平面(Plane)の交差判定を行います。
/// </summary>
bool CheckRayPlane(const Ray& ray, const Plane& plane, float& outDistance);

// =================================================================
// 最近接点計算 (Closest Point)
// =================================================================

/// <summary>
/// 任意の点からAABB上の最近接点を求めます。
/// </summary>
Vector3 ClosestPointOnAABB(const Vector3& point, const AABB& aabb);

/// <summary>
/// 任意の点から線分 (segA～segB) 上の最近接点を求めます。
/// </summary>
Vector3 ClosestPointOnSegment(const Vector3& point, const Vector3& segA, const Vector3& segB);

// =================================================================
// カメラ・スクリーン・ワールド座標変換
// =================================================================

/// <summary>
/// 2Dスクリーン座標(マウス位置等)から、3Dワールド空間へ向かうRay(光線)を生成します(マウスピッキング用)。
/// </summary>
/// <param name="screenPos">スクリーン上のピクセル座標(左上原点)</param>
/// <param name="viewMatrix">ビュー行列</param>
/// <param name="projMatrix">射影行列</param>
/// <param name="screenWidth">画面幅</param>
/// <param name="screenHeight">画面高さ</param>
Ray ScreenToWorldRay(const Vector2& screenPos, const Matrix4x4& viewMatrix, const Matrix4x4& projMatrix, float screenWidth, float screenHeight);

/// <summary>
/// 3Dワールド座標から2Dスクリーン座標(ピクセル)へ変換します(UI追従マーカー等)。
/// </summary>
/// <param name="worldPos">ワールド座標</param>
/// <param name="viewMatrix">ビュー行列</param>
/// <param name="projMatrix">射影行列</param>
/// <param name="screenWidth">画面幅</param>
/// <param name="screenHeight">画面高さ</param>
Vector2 WorldToScreenPoint(const Vector3& worldPos, const Matrix4x4& viewMatrix, const Matrix4x4& projMatrix, float screenWidth, float screenHeight);

