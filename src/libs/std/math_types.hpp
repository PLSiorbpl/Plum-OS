#pragma once

namespace glm {
    struct vec2 {
        union {
            struct { float x, y; };
            struct { float r, g; };
        };

        vec2() = default;
        vec2(const float x, const float y) : x(x), y(y) {}
        explicit vec2(const float a) : x(a), y(a) {}

        vec2& operator+=(const vec2& other) {
            x += other.x;
            y += other.y;
            return *this;
        }
        vec2& operator-=(const vec2& other) {
            x -= other.x;
            y -= other.y;
            return *this;
        }
        vec2& operator*=(const vec2& other) {
            x *= other.x;
            y *= other.y;
            return *this;
        }
        vec2& operator/=(const vec2& other) {
            x /= other.x;
            y /= other.y;
            return *this;
        }
        friend vec2 operator+(const vec2& l, const vec2& r) {
            vec2 result = l;
            result += r;
            return result;
        }
        friend vec2 operator-(const vec2& l, const vec2& r) {
            vec2 result = l;
            result -= r;
            return result;
        }
        friend vec2 operator*(const vec2& l, const vec2& r) {
            vec2 result = l;
            result *= r;
            return result;
        }
        friend vec2 operator/(const vec2& l, const vec2& r) {
            vec2 result = l;
            result /= r;
            return result;
        }
        friend bool operator==(const vec2& l, const vec2& r) {
            return (l.x == r.x) && (l.y == r.y);
        }
        friend bool operator!=(const vec2& l, const vec2& r) {
            return (l.x != r.x) || (l.y != r.y);
        }
        float& operator[](const int index) {
            return (&x)[index];
        }
        const float &operator[](const int index) const {
            return (&x)[index];
        }
    };

    struct ivec2 {
        union {
            struct { int x, y; };
            struct { int r, g; };
        };

        ivec2() = default;
        ivec2(const int x, const int y) : x(x), y(y) {}
        explicit ivec2(const int a) : x(a), y(a) {}

        ivec2& operator+=(const ivec2& other) {
            x += other.x;
            y += other.y;
            return *this;
        }
        ivec2& operator-=(const ivec2& other) {
            x -= other.x;
            y -= other.y;
            return *this;
        }
        ivec2& operator*=(const ivec2& other) {
            x *= other.x;
            y *= other.y;
            return *this;
        }
        ivec2& operator/=(const ivec2& other) {
            x /= other.x;
            y /= other.y;
            return *this;
        }
        friend ivec2 operator+(const ivec2& l, const ivec2& r) {
            ivec2 result = l;
            result += r;
            return result;
        }
        friend ivec2 operator-(const ivec2& l, const ivec2& r) {
            ivec2 result = l;
            result -= r;
            return result;
        }
        friend ivec2 operator*(const ivec2& l, const ivec2& r) {
            ivec2 result = l;
            result *= r;
            return result;
        }
        friend ivec2 operator/(const ivec2& l, const ivec2& r) {
            ivec2 result = l;
            result /= r;
            return result;
        }
        friend bool operator==(const ivec2& l, const ivec2& r) {
            return (l.x == r.x) && (l.y == r.y);
        }
        friend bool operator!=(const ivec2& l, const ivec2& r) {
            return (l.x != r.x) || (l.y != r.y);
        }
        int& operator[](const int index) {
            return (&x)[index];
        }
        const int &operator[](const int index) const {
            return (&x)[index];
        }
    };

    struct vec3 {
        union {
            struct { float x, y, z; };
            struct { float r, g, b; };
        };

        vec3() = default;
        vec3(const float x, const float y, const float z) : x(x), y(y), z(z) {}
        // vec2 float
        vec3(const float x, const vec2 v2) : x(x), y(v2.x), z(v2.y) {}
        vec3(const vec2 v2, const float z) : x(v2.x), y(v2.y), z(z) {}

        explicit vec3(const float a) : x(a), y(a), z(a) {}

        vec2 get_v2() const {
            return vec2(x, y);
        }

        vec3& operator+=(const vec3& other) {
            x += other.x;
            y += other.y;
            z += other.z;
            return *this;
        }
        vec3& operator-=(const vec3& other) {
            x -= other.x;
            y -= other.y;
            z -= other.z;
            return *this;
        }
        vec3& operator*=(const vec3& other) {
            x *= other.x;
            y *= other.y;
            z *= other.z;
            return *this;
        }
        vec3& operator/=(const vec3& other) {
            x /= other.x;
            y /= other.y;
            z /= other.z;
            return *this;
        }
        friend vec3 operator+(const vec3& l, const vec3& r) {
            vec3 result = l;
            result += r;
            return result;
        }
        friend vec3 operator-(const vec3& l, const vec3& r) {
            vec3 result = l;
            result -= r;
            return result;
        }
        friend vec3 operator*(const vec3& l, const vec3& r) {
            vec3 result = l;
            result *= r;
            return result;
        }
        friend vec3 operator/(const vec3& l, const vec3& r) {
            vec3 result = l;
            result /= r;
            return result;
        }
        friend bool operator==(const vec3& l, const vec3& r) {
            return (l.x == r.x) && (l.y == r.y) && (l.z == r.z);
        }
        friend bool operator!=(const vec3& l, const vec3& r) {
            return (l.x != r.x) || (l.y != r.y) || (l.z != r.z);
        }
        float& operator[](const int index) {
            return (&x)[index];
        }
        const float &operator[](const int index) const {
            return (&x)[index];
        }
    };

    struct vec4 {
        union {
            struct { float x, y, z, w; };
            struct { float r, g, b, a; };
        };

        vec4() = default;
        vec4(const float x, const float y, const float z, const float w) : x(x), y(y), z(z), w(w) {}
        // vec3 float
        vec4(const float x, const vec3 v3) : x(x), y(v3.x), z(v3.y), w(v3.z) {}
        vec4(const vec3 v3, const float w) : x(v3.x), y(v3.y), z(v3.z), w(w) {}
        // vec2 float
        vec4(const float x, const float y, const vec2 v2) : x(x), y(y), z(v2.x), w(v2.y) {}
        vec4(const vec2 v2, const float z, const float w) : x(v2.x), y(v2.y), z(z), w(w) {}
        vec4(const float x, const vec2 v2, const float w) : x(x), y(v2.x), z(v2.y), w(w) {}
        // vec2 vec2
        vec4(const vec2 v2_1, const vec2 v2_2) : x(v2_1.x), y(v2_1.y), z(v2_2.x), w(v2_2.y) {}
        explicit vec4(const float a) : x(a), y(a), z(a), w(a) {}

        vec2 get_v2() const {
            return vec2(x, y);
        }

        vec3 get_v3() const {
            return vec3(x,y,z);
        }

        vec4& operator+=(const vec4& other) {
            x += other.x;
            y += other.y;
            z += other.z;
            w += other.w;
            return *this;
        }
        vec4& operator-=(const vec4& other) {
            x -= other.x;
            y -= other.y;
            z -= other.z;
            w -= other.w;
            return *this;
        }
        vec4& operator*=(const vec4& other) {
            x *= other.x;
            y *= other.y;
            z *= other.z;
            w *= other.w;
            return *this;
        }
        vec4& operator/=(const vec4& other) {
            x /= other.x;
            y /= other.y;
            z /= other.z;
            w /= other.w;
            return *this;
        }
        friend vec4 operator+(const vec4& l, const vec4& r) {
            vec4 result = l;
            result += r;
            return result;
        }
        friend vec4 operator-(const vec4& l, const vec4& r) {
            vec4 result = l;
            result -= r;
            return result;
        }
        friend vec4 operator*(const vec4& l, const vec4& r) {
            vec4 result = l;
            result *= r;
            return result;
        }
        friend vec4 operator/(const vec4& l, const vec4& r) {
            vec4 result = l;
            result /= r;
            return result;
        }
        friend bool operator==(const vec4& l, const vec4& r) {
            return (l.x == r.x) && (l.y == r.y) && (l.z == r.z) && (l.w == r.w);
        }
        friend bool operator!=(const vec4& l, const vec4& r) {
            return (l.x != r.x) || (l.y != r.y) || (l.z != r.z) || (l.w != r.w);
        }
        float& operator[](const int index) {
            return (&x)[index];
        }
        const float &operator[](const int index) const {
            return (&x)[index];
        }
    };

    struct mat4x4 {
        vec4 col[4] = {};

        mat4x4() {
            col[0] = {1.0f, 0.0f, 0.0f, 0.0f};
            col[1] = {0.0f, 1.0f, 0.0f, 0.0f};
            col[2] = {0.0f, 0.0f, 1.0f, 0.0f};
            col[3] = {0.0f, 0.0f, 0.0f, 1.0f};
        }

        explicit mat4x4(float val) {
            col[0]={val,0,0,0};
            col[1]={0,val,0,0};
            col[2]={0,0,val,0};
            col[3]={0,0,0,val};
        }

        mat4x4(const vec4 c0, const vec4 c1, const vec4 c2, const vec4 c3)
            : col{c0, c1, c2, c3} {}

        mat4x4(const vec3 v1, const vec3 v2, const vec3 v3, const vec4 v4) {
            col[0] = {v1.x, v1.y, v1.z, 0.0f};
            col[1] = {v2.x, v2.y, v2.z, 0.0f};
            col[2] = {v3.x, v3.y, v3.z, 0.0f};
            col[3] = {v4.x, v4.y, v4.z, v4.w};
        }

        vec4& operator[](const int col_idx) { return col[col_idx]; }
        const vec4& operator[](const int col_idx) const { return col[col_idx]; }

        float& operator()(const int row, const int col_idx) { return col[col_idx][row]; }
        float operator()(const int row, const int col_idx) const { return col[col_idx][row]; }

        mat4x4 operator*(const mat4x4& m) const {
            mat4x4 res;
            for (int i = 0; i < 4; i++) { // Column
                for (int j = 0; j < 4; j++) { // Row
                    res.col[i][j] =
                        col[0][j] * m.col[i][0] +
                        col[1][j] * m.col[i][1] +
                        col[2][j] * m.col[i][2] +
                        col[3][j] * m.col[i][3];
                }
            }
            return res;
        }

        friend vec4 operator*(const mat4x4& m, const vec4& v) {
            return vec4(
                m.col[0][0] * v.x + m.col[1][0] * v.y + m.col[2][0] * v.z + m.col[3][0] * v.w,
                m.col[0][1] * v.x + m.col[1][1] * v.y + m.col[2][1] * v.z + m.col[3][1] * v.w,
                m.col[0][2] * v.x + m.col[1][2] * v.y + m.col[2][2] * v.z + m.col[3][2] * v.w,
                m.col[0][3] * v.x + m.col[1][3] * v.y + m.col[2][3] * v.z + m.col[3][3] * v.w
            );
        }

        friend vec4 operator*(const vec4& v, const mat4x4& m) {
            return vec4(
                v.x * m.col[0][0] + v.y * m.col[1][0] + v.z * m.col[2][0] + v.w * m.col[3][0],
                v.x * m.col[0][1] + v.y * m.col[1][1] + v.z * m.col[2][1] + v.w * m.col[3][1],
                v.x * m.col[0][2] + v.y * m.col[1][2] + v.z * m.col[2][2] + v.w * m.col[3][2],
                v.x * m.col[0][3] + v.y * m.col[1][3] + v.z * m.col[2][3] + v.w * m.col[3][3]
            );
        }

        [[nodiscard]] mat4x4 transpose() const {
            mat4x4 res;
            for (int i = 0; i < 4; i++) {
                for (int j = 0; j < 4; j++) {
                    res.col[i][j] = col[j][i];
                }
            }
            return res;
        }
    };
}