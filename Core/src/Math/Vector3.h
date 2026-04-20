#ifndef VECTOR_3_H
#define VECTOR_3_H

#include <cmath>
#include "Util/Assert.h"

class Vector3
{
public:
	float x, y, z;

	Vector3(float x, float y, float z) : x(x), y(y), z(z) {}
	Vector3(float a) : x(a), y(a), z(a) {}
	Vector3() : x(0.0f), y(0.0f), z(0.0f) {}

	// Addition
	Vector3 operator+(const Vector3& rhs) const { return Vector3(x + rhs.x, y + rhs.y, z + rhs.z); }
	Vector3 operator+(float rhs)          const { return Vector3(x + rhs,   y + rhs,   z + rhs); }

	Vector3& operator+=(const Vector3& rhs) { x += rhs.x; y += rhs.y; z += rhs.z; return *this; }
	Vector3& operator+=(float rhs)          { x += rhs;   y += rhs;   z += rhs;   return *this; }

	// Subtraction
	Vector3 operator-(const Vector3& rhs) const { return Vector3(x - rhs.x, y - rhs.y, z - rhs.z); }
	Vector3 operator-(float rhs)          const { return Vector3(x - rhs,   y - rhs,   z - rhs); }
	Vector3 operator-()                   const { return Vector3(-x, -y, -z); }

	Vector3& operator-=(const Vector3& rhs) { x -= rhs.x; y -= rhs.y; z -= rhs.z; return *this; }
	Vector3& operator-=(float rhs)          { x -= rhs;   y -= rhs;   z -= rhs;   return *this; }

	// Multiplication
	Vector3 operator*(float rhs)          const { return Vector3(x * rhs,   y * rhs,   z * rhs); }
	Vector3 operator*(const Vector3& rhs) const { return Vector3(x * rhs.x, y * rhs.y, z * rhs.z); }

	Vector3& operator*=(float rhs)          { x *= rhs;   y *= rhs;   z *= rhs;   return *this; }
	Vector3& operator*=(const Vector3& rhs) { x *= rhs.x; y *= rhs.y; z *= rhs.z; return *this; }

	// Division
	Vector3 operator/(float rhs)          const { return Vector3(x / rhs,   y / rhs,   z / rhs); }
	Vector3 operator/(const Vector3& rhs) const { return Vector3(x / rhs.x, y / rhs.y, z / rhs.z); }

	Vector3& operator/=(float rhs)          { x /= rhs;   y /= rhs;   z /= rhs;   return *this; }
	Vector3& operator/=(const Vector3& rhs) { x /= rhs.x; y /= rhs.y; z /= rhs.z; return *this; }

	bool operator==(const Vector3& rhs) const { return x == rhs.x && y == rhs.y && z == rhs.z; }
	bool operator!=(const Vector3& rhs) const { return !(*this == rhs); }

	// Element access — asserts on out-of-range index
	float& operator[](int i)
	{
		CORE_ASSERT(i >= 0 && i < 3, "Vector3 index out of range");
		switch (i) { case 0: return x; case 1: return y; default: return z; }
	}

	float operator[](int i) const
	{
		CORE_ASSERT(i >= 0 && i < 3, "Vector3 index out of range");
		switch (i) { case 0: return x; case 1: return y; default: return z; }
	}

	// Magnitude
	float Magnitude() const { return std::sqrt(x*x + y*y + z*z); }
	static float Magnitude(const Vector3& v) { return v.Magnitude(); }

	// Normalize — mutates this vector in place, returns reference for chaining.
	// For a non-mutating version use the static overload.
	Vector3& Normalize()
	{
		float mag = Magnitude();
		if (mag > 0.0f) { x /= mag; y /= mag; z /= mag; }
		return *this;
	}

	// Returns a normalized copy without modifying the input.
	static Vector3 Normalize(const Vector3& v)
	{
		float mag = v.Magnitude();
		if (mag > 0.0f)
			return Vector3(v.x / mag, v.y / mag, v.z / mag);
		return v;
	}

	// Dot product
	float Dot(const Vector3& other) const
	{
		return x * other.x + y * other.y + z * other.z;
	}

	static float Dot(const Vector3& a, const Vector3& b)
	{
		return a.x*b.x + a.y*b.y + a.z*b.z;
	}

	// Cross product
	Vector3 Cross(const Vector3& other) const
	{
		return Vector3(
			y * other.z - z * other.y,
			z * other.x - x * other.z,
			x * other.y - y * other.x);
	}

	static Vector3 Cross(const Vector3& a, const Vector3& b)
	{
		return Vector3(
			a.y*b.z - a.z*b.y,
			a.z*b.x - a.x*b.z,
			a.x*b.y - a.y*b.x);
	}

	// Vector projection of this onto other
	Vector3 Project(const Vector3& other) const
	{
		float mag2 = other.x*other.x + other.y*other.y + other.z*other.z;
		float scalar = Dot(other) / mag2;
		return other * scalar;
	}

	static Vector3 Project(const Vector3& a, const Vector3& b)
	{
		float mag2 = b.x*b.x + b.y*b.y + b.z*b.z;
		float scalar = Dot(a, b) / mag2;
		return b * scalar;
	}

	// Linear interpolation
	Vector3 Lerp(const Vector3& other, float t) const
	{
		return Vector3(
			x + (other.x - x) * t,
			y + (other.y - y) * t,
			z + (other.z - z) * t);
	}

	static Vector3 Lerp(const Vector3& a, const Vector3& b, float t)
	{
		return Vector3(
			a.x + (b.x - a.x) * t,
			a.y + (b.y - a.y) * t,
			a.z + (b.z - a.z) * t);
	}

	// Spherical linear interpolation — inputs must be unit vectors.
	Vector3 Slerp(const Vector3& other, float t) const
	{
		CORE_ASSERT(std::fabsf(Magnitude() - 1.0f) < 1e-4f, "Slerp requires unit vectors");
		CORE_ASSERT(std::fabsf(other.Magnitude() - 1.0f) < 1e-4f, "Slerp requires unit vectors");
		float dot = std::fmaxf(std::fminf(Dot(other), 1.0f), -1.0f);
		float theta = std::acosf(dot) * t;
		Vector3 rel = Vector3::Normalize(other - (*this * dot));
		return (*this * std::cosf(theta)) + (rel * std::sinf(theta));
	}

	static Vector3 Slerp(const Vector3& a, const Vector3& b, float t)
	{
		CORE_ASSERT(std::fabsf(a.Magnitude() - 1.0f) < 1e-4f, "Slerp requires unit vectors");
		CORE_ASSERT(std::fabsf(b.Magnitude() - 1.0f) < 1e-4f, "Slerp requires unit vectors");
		float dot = std::fmaxf(std::fminf(Dot(a, b), 1.0f), -1.0f);
		float theta = std::acosf(dot) * t;
		Vector3 rel = Vector3::Normalize(b - (a * dot));
		return (a * std::cosf(theta)) + (rel * std::sinf(theta));
	}
};

#endif // VECTOR_3_H
