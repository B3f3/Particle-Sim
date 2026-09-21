#pragma once

#include <cmath>

struct Vector2{
    double x;
    double y;

    Vector2 operator+(const Vector2& p2) const{
        return {x + p2.x, y + p2.y};
    }

    Vector2 operator-(const Vector2& p2) const{
        return {x - p2.x, y - p2.y};
    }

    Vector2& operator+=(const Vector2& p2){
        x += p2.x;
        y += p2.y;
        return *this;
    }

    Vector2& operator-=(const Vector2& p2){
        x -= p2.x;
        y -= p2.y;
        return *this;
    }

    Vector2 operator*(double s) const {
        return {x * s, y * s};
    }

    Vector2 operator/(double s) const {
        return {x / s, y / s};
    }

    double dot(const Vector2& p2) const{
        return {x*p2.x + y*p2.y};
    }

    double lengthSquared() const{
        return {x*x + y*y};
    }

    double length() const{
        return sqrt(x*x + y*y);
    }

    Vector2 normalise() const{
        double len = length();
        if (len == 0.0) return {0.0, 0.0};
        return *this / len;
    }
    
};
