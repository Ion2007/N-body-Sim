#include <math.h>
#include <iostream>

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
Vector3::Vector3(double x, double y, double z) {
	this->x = x;
	this->y = y;
	this->z = z;
	//	std::cout << "hi";
}
double Vector3::magnitude() {
	double squareSum = x * x + y * y + z * z;
	if (squareSum <.001 || squareSum>-.001) {
		squareSum += .01;
	}
	return sqrt(squareSum);

}
Vector3 Vector3::unit() {
	double EuclidNorm = magnitude();
	return Vector3(x / EuclidNorm, y / EuclidNorm, z / EuclidNorm);
}
Vector3 Vector3::difference(Vector3 other) {
	return Vector3(other.x - x, other.y - y, other.z - z);
}

// Vector addition
Vector3 Vector3::operator+(const Vector3& other) const {
	return Vector3(x + other.x, y + other.y, z + other.z);
}

// Vector subtraction
Vector3 Vector3::operator-(const Vector3& other) const {
	return Vector3(x - other.x, y - other.y, z - other.z);
}

// Scalar multiplication
Vector3 Vector3::operator*(double scalar) const {
	return Vector3(x * scalar, y * scalar, z * scalar);
}
Vector3 Vector3::operator/(double scalar) const {
	return Vector3(x / scalar, y / scalar, z / scalar);
}
// Dot product
