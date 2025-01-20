#pragma once

#include <cfloat>
#include <cstring>
#include <vector>

#include "../../thirdparty/vml/include/vector.h"
#include "../../thirdparty/vml/include/vector_functions.h"
#include "../../thirdparty/vml/include/matrix.h"

// Matrices are row major.
// Order is e.g.: viewProjection = projection * view.

namespace hlsl
{

    static const float EULER = 2.71828182845904523536f;
    static const float LOG2E = 1.44269504088896340736f;
    static const float LOG10E = 0.434294481903251827651f;
    static const float LN2 = 0.693147180559945309417f;
    static const float LN10 = 2.30258509299404568402f;
    static const float PI = 3.1415926535897932384626433f;
    static const float PI_2 = 1.57079632679489661923f;
    static const float PI_4 = 0.785398163397448309616f;
    static const float INV_PI = 0.318309886183790671538f;
    static const float INV_2PI = 0.15915494309189533577f;
    static const float INV_4PI = 0.07957747154594766788f;
    static const float SQRT2 = 1.41421356237309504880f;
    static const float SQRT3 = 1.732050807568877293528f;
    static const float DEG2RAD = PI / 180.0f;
    static const float RAD2DEG = 180.0f / PI;

    using float4 = vml::vector<float, 0, 1, 2, 3>;
    using float3 = vml::vector<float, 0, 1, 2>;
    using float2 = vml::vector<float, 0, 1>;
    using int4 = vml::vector<int, 0, 1, 2, 3>;
    using int3 = vml::vector<int, 0, 1, 2>;
    using int2 = vml::vector<int, 0, 1>;
    using uint4 = vml::vector<unsigned int, 0, 1, 2, 3>;
    using uint3 = vml::vector<unsigned int, 0, 1, 2>;
    using uint2 = vml::vector<unsigned int, 0, 1>;
    using bool4 = vml::vector<bool, 0, 1, 2, 3>;
    using bool3 = vml::vector<bool, 0, 1, 2>;
    using bool2 = vml::vector<bool, 0, 1>;

    using _01 = vml::indices_pack<0, 1>;
    using _012 = vml::indices_pack<0, 1, 2>;
    using _0123 = vml::indices_pack<0, 1, 2, 3>;
    using float2x2 = vml::matrix<float, vml::vector, _01, _01>;
    using float3x3 = vml::matrix<float, vml::vector, _012, _012>;
    using float4x4 = vml::matrix<float, vml::vector, _0123, _0123>;
    using float3x2 = vml::matrix<float, vml::vector, _012, _01>;
    using float4x2 = vml::matrix<float, vml::vector, _0123, _01>;
    using float4x3 = vml::matrix<float, vml::vector, _0123, _012>;
    using float2x3 = vml::matrix<float, vml::vector, _01, _012>;
    using float3x4 = vml::matrix<float, vml::vector, _012, _0123>;

    static const float4x4 float4x4_identity = float4x4( //
        float4(1.0f, 0.0f, 0.0f, 0.0f),               //
        float4(0.0f, 1.0f, 0.0f, 0.0f),               //
        float4(0.0f, 0.0f, 1.0f, 0.0f),               //
        float4(0.0f, 0.0f, 0.0f, 1.0f));

    static const float4x3 float4x3_identity = float4x3( //
        float4(1.0f, 0.0f, 0.0f, 0.0f),               //
        float4(0.0f, 1.0f, 0.0f, 0.0f),               //
        float4(0.0f, 0.0f, 1.0f, 0.0f));

    static const int2 INT2_MIN = int2(INT_MIN, INT_MIN);
    static const int3 INT3_MIN = int3(INT_MIN, INT_MIN, INT_MIN);
    static const int4 INT4_MIN = int4(INT_MIN, INT_MIN, INT_MIN, INT_MIN);

    static const int2 INT2_MAX = int2(INT_MAX, INT_MAX);
    static const int3 INT3_MAX = int3(INT_MAX, INT_MAX, INT_MAX);
    static const int4 INT4_MAX = int4(INT_MAX, INT_MAX, INT_MAX, INT_MAX);

    static const uint2 UINT2_MAX = uint2(UINT_MAX, UINT_MAX);
    static const uint3 UINT3_MAX = uint3(UINT_MAX, UINT_MAX, UINT_MAX);
    static const uint4 UINT4_MAX = uint4(UINT_MAX, UINT_MAX, UINT_MAX, UINT_MAX);

    static const float2 FLT2_MIN = float2(FLT_MIN, FLT_MIN);
    static const float3 FLT3_MIN = float3(FLT_MIN, FLT_MIN, FLT_MIN);
    static const float4 FLT4_MIN = float4(FLT_MIN, FLT_MIN, FLT_MIN, FLT_MIN);

    static const float2 FLT2_MAX = float2(FLT_MAX, FLT_MAX);
    static const float3 FLT3_MAX = float3(FLT_MAX, FLT_MAX, FLT_MAX);
    static const float4 FLT4_MAX = float4(FLT_MAX, FLT_MAX, FLT_MAX, FLT_MAX);

#define max3( x, y, z )    ( max( max( x, y ), z ) )
#define min3( x, y, z )    ( min( min( x, y ), z ) )
#define med3( x, y, z )    ( max( min( x, y ), min( max( x, y ), z ) ) )
#define min4( x, y, z, w ) ( min( min3( x, y, z ), w ) )
#define max4( x, y, z, w ) ( max( max3( x, y, z ), w ) )

    template <typename T>
    vml::vector<bool, 0, 1> operator==(const vml::vector<T, 0, 1>& l, const vml::vector<T, 0, 1>& r)
    {
        return bool2(l.x == r.x, l.y == r.y);
    }

    template <typename T>
    vml::vector<bool, 0, 1, 2> operator==(const vml::vector<T, 0, 1, 2>& l, const vml::vector<T, 0, 1, 2>& r)
    {
        return bool3(l.x == r.x, l.y == r.y, l.z == r.z);
    }

    template <typename T>
    vml::vector<bool, 0, 1, 2, 3> operator==(const vml::vector<T, 0, 1, 2, 3>& l, const vml::vector<T, 0, 1, 2, 3>& r)
    {
        return bool4(l.x == r.x, l.y == r.y, l.z == r.z, l.w == r.w);
    }

    template <typename T>
    vml::vector<bool, 0, 1> operator!=(const vml::vector<T, 0, 1>& l, const vml::vector<T, 0, 1>& r)
    {
        return bool2(l.x != r.x, l.y != r.y);
    }

    template <typename T>
    vml::vector<bool, 0, 1, 2> operator!=(const vml::vector<T, 0, 1, 2>& l, const vml::vector<T, 0, 1, 2>& r)
    {
        return bool3(l.x != r.x, l.y != r.y, l.z != r.z);
    }

    template <typename T>
    vml::vector<bool, 0, 1, 2, 3> operator!=(const vml::vector<T, 0, 1, 2, 3>& l, const vml::vector<T, 0, 1, 2, 3>& r)
    {
        return bool4(l.x != r.x, l.y != r.y, l.z != r.z, l.w != r.w);
    }

    template <typename T>
    float lengthSquared(const T& t)
    {
        return dot(t, t);
    }

    template <typename T>
    T Square(const T& t)
    {
        return t * t;
    }

    inline float4x4 inverse(const float4x4& _m)
    {
        const float* m = (const float*)&_m;

        float4x4 _inv;
        float* inv = (float*)&_inv;

        inv[0] = m[5] * m[10] * m[15] - m[5] * m[11] * m[14] - m[9] * m[6] * m[15] + m[9] * m[7] * m[14] + m[13] * m[6] * m[11] - m[13] * m[7] * m[10];
        inv[4] = -m[4] * m[10] * m[15] + m[4] * m[11] * m[14] + m[8] * m[6] * m[15] - m[8] * m[7] * m[14] - m[12] * m[6] * m[11] + m[12] * m[7] * m[10];
        inv[8] = m[4] * m[9] * m[15] - m[4] * m[11] * m[13] - m[8] * m[5] * m[15] + m[8] * m[7] * m[13] + m[12] * m[5] * m[11] - m[12] * m[7] * m[9];
        inv[12] = -m[4] * m[9] * m[14] + m[4] * m[10] * m[13] + m[8] * m[5] * m[14] - m[8] * m[6] * m[13] - m[12] * m[5] * m[10] + m[12] * m[6] * m[9];
        inv[1] = -m[1] * m[10] * m[15] + m[1] * m[11] * m[14] + m[9] * m[2] * m[15] - m[9] * m[3] * m[14] - m[13] * m[2] * m[11] + m[13] * m[3] * m[10];
        inv[5] = m[0] * m[10] * m[15] - m[0] * m[11] * m[14] - m[8] * m[2] * m[15] + m[8] * m[3] * m[14] + m[12] * m[2] * m[11] - m[12] * m[3] * m[10];
        inv[9] = -m[0] * m[9] * m[15] + m[0] * m[11] * m[13] + m[8] * m[1] * m[15] - m[8] * m[3] * m[13] - m[12] * m[1] * m[11] + m[12] * m[3] * m[9];
        inv[13] = m[0] * m[9] * m[14] - m[0] * m[10] * m[13] - m[8] * m[1] * m[14] + m[8] * m[2] * m[13] + m[12] * m[1] * m[10] - m[12] * m[2] * m[9];
        inv[2] = m[1] * m[6] * m[15] - m[1] * m[7] * m[14] - m[5] * m[2] * m[15] + m[5] * m[3] * m[14] + m[13] * m[2] * m[7] - m[13] * m[3] * m[6];
        inv[6] = -m[0] * m[6] * m[15] + m[0] * m[7] * m[14] + m[4] * m[2] * m[15] - m[4] * m[3] * m[14] - m[12] * m[2] * m[7] + m[12] * m[3] * m[6];
        inv[10] = m[0] * m[5] * m[15] - m[0] * m[7] * m[13] - m[4] * m[1] * m[15] + m[4] * m[3] * m[13] + m[12] * m[1] * m[7] - m[12] * m[3] * m[5];
        inv[14] = -m[0] * m[5] * m[14] + m[0] * m[6] * m[13] + m[4] * m[1] * m[14] - m[4] * m[2] * m[13] - m[12] * m[1] * m[6] + m[12] * m[2] * m[5];
        inv[3] = -m[1] * m[6] * m[11] + m[1] * m[7] * m[10] + m[5] * m[2] * m[11] - m[5] * m[3] * m[10] - m[9] * m[2] * m[7] + m[9] * m[3] * m[6];
        inv[7] = m[0] * m[6] * m[11] - m[0] * m[7] * m[10] - m[4] * m[2] * m[11] + m[4] * m[3] * m[10] + m[8] * m[2] * m[7] - m[8] * m[3] * m[6];
        inv[11] = -m[0] * m[5] * m[11] + m[0] * m[7] * m[9] + m[4] * m[1] * m[11] - m[4] * m[3] * m[9] - m[8] * m[1] * m[7] + m[8] * m[3] * m[5];
        inv[15] = m[0] * m[5] * m[10] - m[0] * m[6] * m[9] - m[4] * m[1] * m[10] + m[4] * m[2] * m[9] + m[8] * m[1] * m[6] - m[8] * m[2] * m[5];

        float det = m[0] * inv[0] + m[1] * inv[4] + m[2] * inv[8] + m[3] * inv[12];
        if (det == 0.0f)
            return float4x4_identity;
        det = 1.0f / det;

        float4x4 _result;
        float* result = (float*)&_result;
        for (unsigned int i = 0; i < 16; i++)
            result[i] = inv[i] * det;
        return _result;
    }

    inline float determinant(const float4x4& _m)
    {
        const float* m = (const float*)&_m;

        float4x4 _inv;
        float* inv = (float*)&_inv;

        inv[0] = m[5] * m[10] * m[15] - m[5] * m[11] * m[14] - m[9] * m[6] * m[15] + m[9] * m[7] * m[14] + m[13] * m[6] * m[11] - m[13] * m[7] * m[10];
        inv[4] = -m[4] * m[10] * m[15] + m[4] * m[11] * m[14] + m[8] * m[6] * m[15] - m[8] * m[7] * m[14] - m[12] * m[6] * m[11] + m[12] * m[7] * m[10];
        inv[8] = m[4] * m[9] * m[15] - m[4] * m[11] * m[13] - m[8] * m[5] * m[15] + m[8] * m[7] * m[13] + m[12] * m[5] * m[11] - m[12] * m[7] * m[9];
        inv[12] = -m[4] * m[9] * m[14] + m[4] * m[10] * m[13] + m[8] * m[5] * m[14] - m[8] * m[6] * m[13] - m[12] * m[5] * m[10] + m[12] * m[6] * m[9];

        return m[0] * inv[0] + m[1] * inv[4] + m[2] * inv[8] + m[3] * inv[12];
    }

    inline float4x4 transpose(const float4x4& m)
    {
        float4x4 result;
        for (unsigned int i = 0; i < 4; ++i)
            for (unsigned int j = 0; j < 4; ++j)
                result[i][j] = m[j][i];
        return result;
    }

    inline float4x3 to_float4x3(const float4x4& m)
    {
        float4x3 ret;
        memcpy(&ret, &m, sizeof(float4x3));
        return ret;
    }

    inline float4x4 perspective(float fovVerticalInDegrees, float aspect, float zNear, float zFar)
    {
        std::swap(zNear, zFar); // Reverse z-buffer.

        float yScale = 1.0f / tanf(fovVerticalInDegrees * DEG2RAD * 0.5f);
        float xScale = yScale / aspect;
        float nearmfar = zNear - zFar;

        return (float4x4(                                //
            float4(xScale, 0.0f, 0.0f, 0.0f),           //
            float4(0.0f, yScale, 0.0f, 0.0f),           //
            float4(0.0f, 0.0f, zFar / nearmfar, -1.0f), //
            float4(0.0f, 0.0f, zFar * zNear / nearmfar, 0.0f)));
    }

    inline float4x4 orthographic(float width, float height, float zNear, float zFar)
    {
        std::swap(zNear, zFar); // Reverse z-buffer.

        float yScale = 2.0f / width;
        float xScale = 2.0f / height;
        float nearmfar = zNear - zFar;

        return (float4x4(                               //
            float4(xScale, 0.0f, 0.0f, 0.0f),          //
            float4(0.0f, yScale, 0.0f, 0.0f),          //
            float4(0.0f, 0.0f, 1.0f / nearmfar, 0.0f), //
            float4(0.0f, 0.0f, zNear / nearmfar, 1.0f)));
    }

    inline float4x4 lookAt(const float3& from, const float3& to, const float3& up = float3(0.0f, 1.0f, 0.0f))
    {
        float3 z = normalize(from - to);
        float3 x = cross(normalize(up), z);
        float3 y = cross(z, x);

        return inverse(float4x4( //
            float4(x, 0.0f),    //
            float4(y, 0.0f),    //
            float4(z, 0.0f),    //
            float4(from, 1.0f)));
    }

    inline float4x4 world(const float3& position)
    {
        return float4x4(                      //
            float4(1.0f, 0.0f, 0.0f, 0.0f), //
            float4(0.0f, 1.0f, 0.0f, 0.0f), //
            float4(0.0f, 0.0f, 1.0f, 0.0f), //
            float4(position, 1.0f));
    }

    inline float4x4 world(const float3& position, const float3& direction, const float3& up = float3(0.0f, 1.0f, 0.0f), float scale = 1.0f)
    {
        float3 z = -normalize(direction);
        float3 x = cross(normalize(up), z);
        float3 y = cross(z, x);

        return float4x4(               //
            float4(x * scale, 0.0f), //
            float4(y * scale, 0.0f), //
            float4(z * scale, 0.0f), //
            float4(position, 1.0f));
    }

    inline float4x4 translation(const float3& position)
    {
        return float4x4(                      //
            float4(1.0f, 0.0f, 0.0f, 0.0f), //
            float4(0.0f, 1.0f, 0.0f, 0.0f), //
            float4(0.0f, 0.0f, 1.0f, 0.0f), //
            float4(position, 1.0f));
    }

    inline float4x4 translation(const float4x4& matrix)
    {
        return float4x4(                      //
            float4(1.0f, 0.0f, 0.0f, 0.0f), //
            float4(0.0f, 1.0f, 0.0f, 0.0f), //
            float4(0.0f, 0.0f, 1.0f, 0.0f), //
            float4(matrix[3].xyz, 1.0f));
    }

    inline float4x4 rotation(const float4x4& matrix)
    {
        return float4x4(                                //
            float4(normalize(matrix[0].xyz), 0.0f), //
            float4(normalize(matrix[1].xyz), 0.0f), //
            float4(normalize(matrix[2].xyz), 0.0f), //
            float4(0.0f, 0.0f, 0.0f, 1.0f));
    }

    inline float4x4 rotation(const float4& quat)
    {
        float4x4 m = {};

        float x = quat.x, y = quat.y, z = quat.z, w = quat.w;
        float x2 = x + x, y2 = y + y, z2 = z + z;
        float xx = x * x2, xy = x * y2, xz = x * z2;
        float yy = y * y2, yz = y * z2, zz = z * z2;
        float wx = w * x2, wy = w * y2, wz = w * z2;

        m[0][0] = 1.0f - (yy + zz);
        m[1][0] = xy - wz;
        m[2][0] = xz + wy;

        m[0][1] = xy + wz;
        m[1][1] = 1.0f - (xx + zz);
        m[2][1] = yz - wx;

        m[0][2] = xz - wy;
        m[1][2] = yz + wx;
        m[2][2] = 1.0f - (xx + yy);

        m[3][3] = 1.0f;

        return m;
    }

    inline float4x4 scale(const float4x4& matrix)
    {
        return float4x4(                                         //
            float4(length(matrix[0].xyz), 0.0f, 0.0f, 0.0f), //
            float4(0.0f, length(matrix[1].xyz), 0.0f, 0.0f), //
            float4(0.0f, 0.0f, length(matrix[2].xyz), 0.0f), //
            float4(0.0f, 0.0f, 0.0f, 1.0f));
    }

    inline float4x4 scale(const float3& scale)
    {
        return float4x4(                         //
            float4(scale.x, 0.0f, 0.0f, 0.0f), //
            float4(0.0f, scale.y, 0.0f, 0.0f), //
            float4(0.0f, 0.0f, scale.z, 0.0f), //
            float4(0.0f, 0.0f, 0.0f, 1.0f));
    }

    inline float4 slerp(const float4& qa, const float4& qb2, float t)
    {
        // [https://www.euclideanspace.com/maths/algebra/realNormedAlgebra/quaternions/slerp/index.htm]
        float cosHalfTheta = dot(qa, qb2);

        float4 qb = qb2;
        if (cosHalfTheta < 0.0f)
        {
            qb = float4(-qb.x, -qb.y, qb.z, -qb.w);
            cosHalfTheta = -cosHalfTheta;
        }

        // If qa=qb or qa=-qb then theta = 0 and we can return qa.
        if (abs(cosHalfTheta) >= 1.0f)
            return qa;

        float halfTheta = acos(cosHalfTheta);
        float sinHalfTheta = sqrt(1.0f - cosHalfTheta * cosHalfTheta);

        // If theta = 180 degrees then result is not fully defined.
        // We could rotate around any axis normal to qa or qb.
        if (fabs(sinHalfTheta) < 0.001f)
            return qa * 0.5f + qb * 0.5f;

        float ratioA = sin((1.0f - t) * halfTheta) / sinHalfTheta;
        float ratioB = sin(t * halfTheta) / sinHalfTheta;

        return /*normalize*/ (qa * ratioA + qb * ratioB);
    }

    template <typename T>
    T CubicSpline(const T& vert0, const T& tang0, const T& vert1, const T& tang1, float t)
    {
        float t2 = t * t;
        float t3 = t2 * t;
        float s2 = -2.0f * t3 + 3.0f * t2;
        float s3 = t3 - t2;
        float s0 = 1.0f - s2;
        float s1 = s3 - t2 + t;
        return s0 * vert0 + s1 * tang0 * t + s2 * vert1 + s3 * tang1 * t;
    }

    inline float4 EulerToQuaternion(const float3& euler) // x/y/z roll/pitch/yaw.
    {
#if 0 
        // #TODO: Need to handle this on Editor side.
        // This code handles pitch out of <-90,90> range by representing it in equivalent euler angles by fliping xz,
        // but it does not handle fact that after e.g. going over 90 we now mirror the pitch value by going down from 90,
        // yet user is still moving mouse to right and ImGui keeps increasing it instead.
        if (euler.y > PI_2)
        {
            euler.x += PI;
            euler.z += PI;
            euler.y = PI_2 - (euler.y - PI_2);
        }
        if (euler.y < -PI_2)
        {
            euler.x += PI;
            euler.z += PI;
            euler.y = -PI_2 - (euler.y + PI_2);
        }
#endif

        float cx = cosf(euler.x * 0.5f);
        float sx = sinf(euler.x * 0.5f);
        float cy = cosf(euler.y * 0.5f);
        float sy = sinf(euler.y * 0.5f);
        float cz = cosf(euler.z * 0.5f);
        float sz = sinf(euler.z * 0.5f);

        float4 q;
        q.x = sx * cy * cz - cx * sy * sz;
        q.y = cx * sy * cz + sx * cy * sz;
        q.z = cx * cy * sz - sx * sy * cz;
        q.w = cx * cy * cz + sx * sy * sz;
        return q;
    }

    inline float3 QuaternionToEuler(const float4& q) // Roll/pitch/yaw.
    {
        float test = clamp(2 * (q.w * q.y - q.x * q.z), -1.0f, 1.0f); // equals - 2 * ( x*z - w*y );
        float ry = asinf(test);

        float3 euler;
        if (fabsf(test) < 0.99999f)
        {
            float yy = q.y * q.y;
            float rz = atan2f(2.0f * (q.x * q.y + q.w * q.z), 1.0f - 2.0f * (yy + q.z * q.z));
            float rx = atan2f(2.0f * (q.y * q.z + q.w * q.x), 1.0f - 2.0f * (q.x * q.x + yy));
            euler = float3(rx, ry, rz);
        }
        else
        {
            float sign = test >= 0.0f ? 1.0f : -1.0f;
            float rz = 0.0f;
            float rx = atan2f(sign * 2.0f * (q.x * q.y - q.w * q.z), sign * 2.0f * (q.x * q.z + q.w * q.y));
            euler = float3(rx, ry, rz);
        }
        return euler;
    }

    inline void DecomposeMatrix(float4x4 matrix, float3& translation, float4& rotation, float3& scale)
    {
        translation = matrix[3].xyz;
        scale = float3(length(matrix[0].xyz), length(matrix[1].xyz), length(matrix[2].xyz));

        matrix[0].xyz = matrix[0].xyz / scale.x;
        matrix[1].xyz = matrix[1].xyz / scale.y;
        matrix[2].xyz = matrix[2].xyz / scale.z;

        float trace = matrix[0][0] + matrix[1][1] + matrix[2][2];
        if (trace > 0.0f)
        {
            float s = sqrt(trace + 1.0f);
            rotation.w = 0.5f * s;
            s = 0.5f / s;
            rotation.x = (matrix[1][2] - matrix[2][1]) * s;
            rotation.y = (matrix[2][0] - matrix[0][2]) * s;
            rotation.z = (matrix[0][1] - matrix[1][0]) * s;
        }
        else
        {
            int i = 0;
            if (matrix[1][1] > matrix[0][0])
                i = 1;
            if (matrix[2][2] > matrix[i][i])
                i = 2;

            int   next[3] = { 1, 2, 0 };
            int   j = next[i];
            int   k = next[j];
            float s = sqrt((matrix[i][i] - (matrix[j][j] + matrix[k][k])) + 1);
            rotation[i] = 0.5f * s;
            if (s != 0.0f)
                s = 0.5f / s;
            rotation.w = (matrix[j][k] - matrix[k][j]) * s;
            rotation[j] = (matrix[i][j] + matrix[j][i]) * s;
            rotation[k] = (matrix[i][k] + matrix[k][i]) * s;
        }
    }

    inline float4x4 ComposeMatrix(const float3& t, const float4& r, const float3& s)
    {
        return translation(t) * rotation(r) * scale(s);
    }

    inline std::vector<bool> epsilonNotEqual(hlsl::float3 const value1, hlsl::float3 value2, float epsilon)
    {
        std::vector<bool> result;
        // X
        if (fabs(value1.x - value2.x) > epsilon)
        {
            result.push_back(true);
        }
        else
        {
            result.push_back(false);
        }
        // Y
        if (fabs(value1.y - value2.y) > epsilon)
        {
            result.push_back(true);
        }
        else
        {
            result.push_back(false);

        }

        // Z
        if (fabs(value1.z - value2.z) > epsilon)
        {
            result.push_back(true);
        }
        else
        {
            result.push_back(false);
        }
        return result;
    }

    inline float3 normalizeSafe(float3 v)
    {
        float len = length(v);
        if (len > 0.0f)
            return v / len;
        return float3(0.0f, 0.0f, 0.0f);
    }
    inline float3 cross(float3 a, float3 b)
    {
        return float3(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x);
    }

    inline float2 normalize(const float2& value)
    {
        float len = length(value);
        if (len > 0.0f)
            return value / len;
        return float2(0.0f, 0.0f);
    }

    inline float3 min(const float3& left, const float3& right)
    {
        return float3(std::min(left.x, right.x), std::min(left.y, right.y), std::min(left.z, right.z));
    }

    inline float3 max(const float3& left, const float3& right)
    {
        return float3(std::max(left.x, right.x), std::max(left.y, right.y), std::max(left.z, right.z));
    }

    inline float3 abs(const float3& value)
    {
        return float3(fabs(value.x), fabs(value.y), fabs(value.z));
    }
    inline float length(const float3& value)
    {
        return sqrtf(value.x * value.x + value.y * value.y + value.z * value.z);
    }

    inline uint32_t divRoundUp(uint32_t numerator, uint32_t denominator)
    {
        return (numerator + denominator - 1) / denominator;
    }

} // namespace hlsl
