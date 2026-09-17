#pragma once
#include "math.hpp"
#include "math_types.hpp"
#include "trigonometry.hpp"

namespace glm {
    inline float dot(const vec3& a, const vec3& b) {
        return a.x * b.x + a.y * b.y + a.z * b.z;
    }

    inline vec3 cross(const vec3& a, const vec3& b) {
        return {
            a.y * b.z - a.z * b.y,
            a.z * b.x - a.x * b.z,
            a.x * b.y - a.y * b.x
        };
    }

    inline vec3 normalize(const vec3& v) {
        const float len = std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
        if (len != 0) return { v.x / len, v.y / len, v.z / len };
        return { 0, 0, 0 };
    }

    inline mat4x4 look_at(const vec3& eye, const vec3& center, const vec3& up) {
        const vec3 f = normalize(center - eye);
        vec3 s = normalize(cross(f, up));
        vec3 u = cross(s, f);

        mat4x4 res;
        res.col[0] = { s.x, u.x, -f.x, 0.0f };
        res.col[1] = { s.y, u.y, -f.y, 0.0f };
        res.col[2] = { s.z, u.z, -f.z, 0.0f };
        res.col[3] = {-dot(s, eye), -dot(u, eye), dot(f, eye), 1.0f};

        return res;
    }

    inline float radians(const float degrees) {
        return degrees * (std::PI / 180.0f);
    }

    inline float degrees(const float radians_val) {
        return radians_val * (180.0f / std::PI);
    }

    inline mat4x4 perspective(const float fov, const float aspect, const float near_p, const float far_p) {
        const float tan_half_fov = std::tan(radians(fov) * 0.5f);
        const float z_range = far_p - near_p;

        mat4x4 res;
        res.col[0] = { 1.0f / (aspect * tan_half_fov), 0.0f, 0.0f, 0.0f };
        res.col[1] = { 0.0f, 1.0f / tan_half_fov, 0.0f, 0.0f };
        res.col[2] = { 0.0f, 0.0f, -(far_p + near_p) / z_range, -1.0f };
        res.col[3] = { 0.0f, 0.0f, -(2.0f * far_p * near_p) / z_range, 0.0f };

        return res;
    }
}
