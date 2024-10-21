#pragma once
struct Vector3 {
	double x, y, z;
	Vector3() {};
	Vector3(double x, double y, double z);
	double magnitude();
	Vector3 unit();
	Vector3 difference(Vector3 other);
	Vector3 operator-(const Vector3& other) const;
	Vector3 operator+(const Vector3& other) const;
	Vector3 operator*(double scalar) const;
	Vector3 operator/(double scalar) const;
};
