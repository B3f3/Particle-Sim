#pragma once

#include <cmath>

struct Vec2{
    double x;
    double y;

    Vec2 operator+(const Vec2& p2) const{
        return {x + p2.x, y + p2.y};
    }

    Vec2 operator-(const Vec2& p2) const{
        return {x - p2.x, y - p2.y};
    }

    Vec2& operator+=(const Vec2& p2){
        x += p2.x;
        y += p2.y;
        return *this;
    }

    Vec2& operator-=(const Vec2& p2){
        x -= p2.x;
        y -= p2.y;
        return *this;
    }

    Vec2 operator*(double s) const {
        return {x * s, y * s};
    }

    Vec2 operator/(double s) const {
        return {x / s, y / s};
    }

    double dot(const Vec2& p2) const{
        return {x*p2.x + y*p2.y};
    }

    double lengthSquared() const{
        return {x*x + y*y};
    }

    double length() const{
        return sqrt(x*x + y*y);
    }

    Vec2 normalise() const{
        double len = length();
        if (len == 0.0) return {0.0, 0.0};
        return *this / len;
    }
    
};
