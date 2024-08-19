#ifndef GEOMETRY_HPP
#define GEOMETRY_HPP

#include "opengl.hpp"

#include <Eigen/Dense>

#include <nlohmann/json.hpp>
#include <tinyxml2/tinyxml2.h>

#include <vector>

typedef nlohmann::json json;

// --------------------------------------------------------------------------------

#define PI           3.14159265358979323846
#define EARTH_RADIUS 6378137.0

// --------------------------------------------------------------------------------

#define COLOR_WHITE       glm::vec4{ 1.0f,  1.0f,  1.0f, 1.0f}
#define COLOR_BRIGHT_GRAY glm::vec4{0.75f, 0.75f, 0.75f, 1.0f}
#define COLOR_GRAY        glm::vec4{ 0.5f,  0.5f,  0.5f, 1.0f}
#define COLOR_DARK_GRAY   glm::vec4{0.25f, 0.25f, 0.25f, 1.0f}
#define COLOR_BLACK       glm::vec4{ 0.0f,  0.0f,  0.0f, 1.0f}

#define COLOR_RED         glm::vec4{ 1.0f,  0.0f,  0.0f, 1.0f}
#define COLOR_YELLOW      glm::vec4{ 1.0f,  1.0f,  0.0f, 1.0f}
#define COLOR_GREEN       glm::vec4{ 0.0f,  1.0f,  0.0f, 1.0f}
#define COLOR_BLUE        glm::vec4{ 0.0f,  0.0f,  1.0f, 1.0f}
#define COLOR_FUCHSIA     glm::vec4{ 1.0f,  0.0f,  1.0f, 1.0f}

#define COLOR_SLATE       glm::vec4{ 0.2f,  0.3f,  0.3f, 1.0f}
#define COLOR_LAVENDER    glm::vec4{ 0.8f,  0.5f,  0.9f, 1.0f}
#define COLOR_VISTA       glm::vec4{ 0.5f,  0.6f,  0.9f, 1.0f}
#define COLOR_TURQUOISE   glm::vec4{ 0.2f,  0.8f,  0.8f, 1.0f}
#define COLOR_EMERALD     glm::vec4{ 0.5f,  0.9f,  0.6f, 1.0f}

// --------------------------------------------------------------------------------

inline float randf() { return rand() / (RAND_MAX + 1.0f); }

// --------------------------------------------------------------------------------

inline bool on_segment(glm::vec3 p, glm::vec3 q, glm::vec3 r) {
    if ((MIN(p.x, q.x) < r.x && r.x < MAX(p.x, q.x))  &&
        (MIN(p.z, q.z) < r.z && r.z < MAX(p.z, q.z))) {
        return true;
    } else {
        return false;
    }
}

inline bool collinear(glm::vec3 p, glm::vec3 q, glm::vec3 r) {
    return glm::dot(glm::normalize(q - p), glm::normalize(r - p)) >= 0.999f;
}

inline float orientation(glm::vec3 p, glm::vec3 q, glm::vec3 r) {
    return (r.x - p.x) * (q.z - p.z) - (q.x - p.x) * (r.z - p.z);
}

bool segments_intersect(glm::vec3 p0, glm::vec3 q0, glm::vec3 p1, glm::vec3 q1) {
    float o0 = orientation(p0, q0, p1);
    float o1 = orientation(p0, q0, q1);
    float o2 = orientation(p1, q1, p0);
    float o3 = orientation(p1, q1, q0);

    if (((o0 > 0.0f && o1 < 0.0f) || (o0 < 0.0f && o1 > 0.0f)) &&
        ((o2 > 0.0f && o3 < 0.0f) || (o2 < 0.0f && o3 > 0.0f))) {
        return true;
    }

    if (collinear(p0, q0, p1) && on_segment(p0, q0, p1)) {
        return true;
    } else if (collinear(p0, q0, q1) && on_segment(p0, q0, q1)) {
        return true;
    } else if (collinear(p1, q1, p0) && on_segment(p1, q1, p0)) {
        return true;
    } else if (collinear(p1, q1, q0) && on_segment(p1, q1, q0)) {
        return true;
    }
    /*
    if (o0 == 0.0f && on_segment(p0, q0, p1)) {
        return true;
    } else if (o1 == 0.0f && on_segment(p0, q0, q1)) {
        return true;
    } else if (o2 == 0.0f && on_segment(p1, q1, p0)) {
        return true;
    } else if (o3 == 0.0f && on_segment(p1, q1, q0)) {
        return true;
    }
    */

    return false;
}

// --------------------------------------------------------------------------------

std::vector<float> linspace(float min, float max, uint count) {
    std::vector<float> ret;

    if (count == 0) {
        return ret;
    }

    if (count == 1) {
        ret.push_back(min);
        return ret;
    }

    float delta = (max - min) / (count - 1);

    ret.reserve(count);
    for (uint i = 0; i < count - 1; ++i) {
        ret.push_back(min + delta * i);
    }
    ret.push_back(max);

    return ret;
}

// --------------------------------------------------------------------------------

void PCA_XZ(glm::mat2 &eigen_vecs, glm::vec2 &eigen_vals, glm::vec2 &mean,
            const std::vector<glm::vec3> &points) {
    size_t num_points = points.size();

    // Compute the mean of the points
    mean = {};
    for (const auto &point : points) {
        mean.x += point.x;
        mean.y += point.z;
    }
    mean /= static_cast<float>(num_points);

    // Center the points and compute the covariance matrix
    glm::mat2 cov_mat = {};
    glm::vec2 centered;
    for (const auto &point : points) {
        centered.x = point.x - mean.x;
        centered.y = point.z - mean.y;

        cov_mat += glm::outerProduct(centered, centered);
    }
    cov_mat /= static_cast<float>(num_points - 1);

    // Compute eigen values and eigen vectors
    Eigen::Matrix2f cov;
    cov << cov_mat[0][0], cov_mat[0][1],
           cov_mat[1][0], cov_mat[1][1];

    Eigen::SelfAdjointEigenSolver<Eigen::Matrix2f> solver(cov);
    ASSERT(solver.info() == Eigen::Success);

    auto evals = solver.eigenvalues();
    auto evecs = solver.eigenvectors();

    eigen_vals = glm::vec2(evals[1], evals[0]);
    eigen_vecs = glm::mat2(
        evecs(0, 1), evecs(0, 0),
        evecs(1, 1), evecs(1, 0)
    );
}

void transform_points(std::vector<glm::vec3> &dst, const std::vector<glm::vec3> &points) {
    dst.reserve(points.size());

    glm::mat2 eigen_vecs;
    glm::vec2 eigen_vals;
    glm::vec2 mean; //
    PCA_XZ(eigen_vecs, eigen_vals, mean, points);

    glm::vec2 proj_point;
    glm::vec2 centered_point; //
    glm::vec2 trans_point;

    for (const auto &point : points) {
        proj_point = {point.x, point.z};
        centered_point = proj_point - mean; //
        trans_point = eigen_vecs * centered_point; //

        trans_point += mean;

        dst.emplace_back(trans_point.x, point.y, trans_point.y);
    }
}

// --------------------------------------------------------------------------------

struct Collada_Source_Data {
    uint  type;
    uint  size;
    uint  stride;
    void *data;
};

void read_collada_source(Collada_Source_Data &dst, tinyxml2::XMLElement *source_elem) {
    const char *source_id = source_elem->Attribute("id");

    auto *arr = source_elem->FirstChildElement("float_array");
    if (arr == nullptr) {
        LOG_ERROR("Failed to get 'float_array' element from COLLADA source element with id=%s.",
                  source_id);
        return;
    }

    uint num_vals;
    arr->QueryUnsignedAttribute("count", &num_vals);
    dst.size = num_vals;

    uint stride;
    auto *accessor_elem = source_elem->FirstChildElement("technique_common")->FirstChildElement("accessor");
    if (accessor_elem == nullptr) {
        LOG_ERROR("Failed to get 'stride' attribute from COLLADA source element with id=%s.",
                  source_id);
        dst.stride = 1;
    } else {
        accessor_elem->QueryUnsignedAttribute("stride", &stride);
        dst.stride = stride;
    }

    dst.type = GL_FLOAT;
    dst.size *= sizeof(float);
    dst.data = static_cast<float *>(malloc(num_vals * sizeof(float)));
    ASSERT(dst.data != nullptr);

    const char *txt = arr->GetText();
    char *end;
    for (uint i = 0; i < num_vals; ++i) {
        float val = strtof(txt, &end);

        if (end == txt) {
            LOG_ERROR("Failed to convert 'float_array' element no. %u to float from COLLADA source element with id=%s.",
                      i, source_id);
            dst.data = nullptr;
            break;
        }

        reinterpret_cast<float*>(dst.data)[i] = val;
        txt = end;
    }
}

// --------------------------------------------------------------------------------

void read_mat4(glm::mat4 &dst, tinyxml2::XMLElement *mat_elem) {
    const char *txt = mat_elem->GetText();
    sscanf(
        txt, "%f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f",
        &dst[0][0], &dst[0][1], &dst[0][2], &dst[0][3],
        &dst[1][0], &dst[1][1], &dst[1][2], &dst[1][3],
        &dst[2][0], &dst[2][1], &dst[2][2], &dst[2][3],
        &dst[3][0], &dst[3][1], &dst[3][2], &dst[3][3]
    );
}

void read_vec3(glm::vec3 &dst, tinyxml2::XMLElement *vec_elem) {
    const char *txt = vec_elem->GetText();
    sscanf(txt, "%f %f %f", &dst[0], &dst[1], &dst[2]);
}

// --------------------------------------------------------------------------------

struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
};

// --------------------------------------------------------------------------------

glm::vec2 to_meters(double lng, double lat) {
    glm::vec2 ret;
    ret.x = static_cast<float>(EARTH_RADIUS * glm::radians(lng));
    ret.y = static_cast<float>(EARTH_RADIUS * log(tan(PI / 4 + glm::radians(lat) / 2)));

    return ret;
}

glm::vec3 to_vec3(const json &point) {
    double lng = point[0];
    double lat = point[1];

    const auto &meter_coords = to_meters(lng, lat);

    return {meter_coords.x, 0.0f, -meter_coords.y};
};

Vertex to_vertex(const json &point) {
    double lng = point[0];
    double lat = point[1];

    const auto &meter_coords = to_meters(lng, lat);

    return {glm::vec3(meter_coords.x, 0.0f, -meter_coords.y), glm::vec3(0.0f)};
};

// --------------------------------------------------------------------------------

struct Geometry {
    std::vector<Vertex> vertices;
    std::vector<uint>   indices;
};

const char *read_uint(uint &dst, const char *cur) {
    dst = 0;
    while (*cur >= '0' && *cur <= '9') {
        dst = dst * 10 + (*cur - '0');
        ++cur;
    }

    return cur;
}

const char *skip_non_digits(const char *cur) {
    while (*cur != '\0' && (*cur < '0' || *cur > '9')) {
        ++cur;
    }

    return cur;
}

void read_geometry(Geometry &dst, tinyxml2::XMLElement *triangles_elem, uint num_attribs,
                   const std::vector<glm::vec3> &positions, const std::vector<glm::vec3> &normals) {
    while (triangles_elem != nullptr) {
        const char *txt = triangles_elem->FirstChildElement("p")->GetText();
        const char *cur = txt;

        Vertex vert;
        uint position_idx, normal_idx, _ignored;

        while (*cur != '\0') {
            cur = read_uint(position_idx, cur);
            cur = skip_non_digits(cur);

            cur = read_uint(normal_idx, cur);
            cur = skip_non_digits(cur);

            for (uint i = 0; i < num_attribs - 2; ++i) {
                cur = read_uint(_ignored, cur);
                cur = skip_non_digits(cur);
            }

            vert.position = positions[position_idx];
            vert.normal = normals[normal_idx];

            dst.vertices.push_back(vert);
            dst.indices.push_back(dst.vertices.size() - 1);
        }

        triangles_elem = triangles_elem->NextSiblingElement("triangles");
    }
}

// --------------------------------------------------------------------------------

struct AABB {
    glm::vec3 min = { FLT_MAX,  FLT_MAX,  FLT_MAX};
    glm::vec3 max = {-FLT_MAX, -FLT_MAX, -FLT_MAX};

    void extend(glm::vec3 point) {
        min = glm::min(point, min);
        max = glm::max(point, max);
    }

    inline bool contains(glm::vec3 point) const {
        return (point.x >= min.x && point.x <= max.x &&
                point.y >= min.y && point.y <= max.y &&
                point.z >= min.z && point.z <= max.z);
    }

    std::vector<AABB> subdivide(int granularity) const {
        std::vector<AABB> ret;

        glm::vec3 size = max - min;
        glm::vec3 step = size / static_cast<float>(granularity);

        glm::vec3 voxel_min, voxel_max;

        for (int x = 0; x < granularity; ++x) {
            for (int y = 0; y < granularity; ++y) {
                for (int z = 0; z < granularity; ++z) {
                    voxel_min = min + glm::vec3(x, y, z) * step;
                    voxel_max = voxel_min + step;

                    ret.push_back({voxel_min, voxel_max});
                }
            }
        }

        return ret;
    }
};

std::vector<glm::vec3> get_aabb_centroids(const AABB &aabb, int granularity) {
    std::vector<glm::vec3> ret;

    const auto &voxels = aabb.subdivide(granularity);
    glm::vec3 centroid;

    for (const auto &voxel : voxels) {
        centroid = (voxel.min + voxel.max) * 0.5f;
        ret.push_back(centroid);
    }

    return ret;
}

// --------------------------------------------------------------------------------

enum Mesh_Type : ubyte {
    MESH_TYPE_FLAT,
    MESH_TYPE_WATER,
    MESH_TYPE_TREE,
    MESH_TYPE_BUILDING,
    MESH_TYPE_AMENITY,
    MESH_TYPE_LANDMARK,
    MESH_TYPE_MISC
};

struct Mesh {
    Mesh_Type             type             = MESH_TYPE_MISC;

    std::vector<Vertex>   vertices;
    std::vector<uint>     indices;

    uint                  base_vert_count  = 4;
    AABB                  aabb;

    glm::vec4             color            = {};

    Vertex_Array<Vertex>  vertex_array     = {};
    Vertex_Buffer         vertex_buffer    = {};
    Index_Buffer          index_buffer     = {};

// --------------------------------------------------------------------------------

    void init(Mesh_Type mesh_type) {
        type = (type > 0) ? type : mesh_type;

        vertex_array = make_vertex_array<Vertex>();
        vertex_buffer = make_vertex_buffer();
        index_buffer = make_index_buffer();

        vertex_array.bind();

        vertex_buffer.init(vertices.data(), vertices.size() * sizeof(Vertex));
        index_buffer.init(indices.data(), indices.size() * sizeof(uint));

        // Vertex position
        vertex_array.push<float>(3, 0);

        if (mesh_type > MESH_TYPE_FLAT) {
            // Vertex normal
            vertex_array.push<float>(3, offsetof(Vertex, normal));
        }
    }

// --------------------------------------------------------------------------------

    // IMPORTANT(paalf): only works if the mesh only has the base vertices
    void triangulate_face(uint min, uint max) {
        auto segment_intersects_base_polygon = [&](uint src, uint dst) {
            uint cur_idx, next_idx;

            for (size_t i = 0, vert_count = vertices.size(); i < vert_count; ++i) {
                cur_idx = i;
                next_idx = (i + 1) % vert_count;

                if (segments_intersect(vertices[src].position, vertices[dst].position,
                                       vertices[cur_idx].position, vertices[next_idx].position)) {
                    return true;
                }
            }

            return false;
        };

        uint open_count = max - min;
        std::vector<uint> open_indices;
        open_indices.reserve(open_count);

        for (uint i = min; i < max; ++i) {
            open_indices.push_back(i);
        }

        int count = -1;
        while (count != open_count && open_count >= 3) {
            count = open_count;
            for (uint i = 0; i < open_count; ++i) {
                uint mid = (i + 1) % open_count;

                uint idx0 = open_indices[i];
                uint idx1 = open_indices[mid];
                uint idx2 = open_indices[(i + 2) % open_count];

                const glm::vec3 &v0 = vertices[idx0].position;
                const glm::vec3 &v1 = vertices[idx1].position;
                const glm::vec3 &v2 = vertices[idx2].position;

                const glm::vec3 &d0 = v1 - v0;
                const glm::vec3 &d1 = v2 - v0;

                if ((d0.z * d1.x - d0.x * d1.z) < 0.0f) {
                    continue;
                }

                if (segment_intersects_base_polygon(idx0, idx2) || segment_intersects_base_polygon(idx1, idx2)) {
                    continue;
                }

                indices.push_back(idx0);
                indices.push_back(idx1);
                indices.push_back(idx2);

                open_count--;
                for (uint j = mid; j < open_count; ++j) {
                    open_indices[j] = open_indices[j + 1];
                }
            }
        }
    }

// --------------------------------------------------------------------------------

    std::vector<Vertex> subdivide(uint hres, uint vres) const {
        std::vector<Vertex> ret;
        ret.reserve(hres * vres * base_vert_count + 1);

        std::vector<float> hcoefs = linspace(0.0f, 1.0f, hres);
        std::vector<float> vcoefs = vres == hres ? hcoefs : linspace(0.0f, 1.0f, vres);

        size_t next_idx;
        glm::vec3 p_lower, p, p_upper, q_lower, q, q_upper;
        glm::vec3 h, v, face_normal;

        for (size_t j = 0; j < vcoefs.size(); ++j) {
            for (size_t i = 0; i < base_vert_count; ++i) {
                p_lower = vertices[i].position, p_upper = vertices[i + base_vert_count].position;

                p = glm::mix(p_lower, p_upper, vcoefs[j]);

                next_idx = (i + 1) % base_vert_count;
                q_lower = vertices[next_idx].position, q_upper = vertices[next_idx + base_vert_count].position;

                q = glm::mix(q_lower, q_upper, vcoefs[j]);

                h = glm::normalize(q_lower - p_lower), v = glm::normalize(p_upper - p_lower);
                face_normal = glm::normalize(glm::cross(h, v));

                for (size_t k = 0; k < hcoefs.size(); ++k) {
                    ret.push_back({glm::mix(p, q, hcoefs[k]), face_normal});
                }
            }
        }

        return ret;
    }

    std::vector<Vertex> subdivide_aabb(int granularity[4]) const {
        uint hres = granularity[0];
        uint vres = granularity[1];
        uint dres = granularity[2];

        std::vector<Vertex> ret;
        ret.reserve(hres * vres * dres * 4);

        std::vector<float> hcoefs = linspace(0.0f, 1.0f, hres);
        std::vector<float> vcoefs = vres == hres ? hcoefs : linspace(0.0f, 1.0f, vres);
        std::vector<float> dcoefs = dres == hres ? hcoefs : linspace(0.0f, 1.0f, dres);

        glm::vec3 corners[8] = {
            // Bottom
            aabb.min,
            {aabb.max.x, aabb.min.y, aabb.min.z},
            {aabb.max.x, aabb.min.y, aabb.max.z},
            {aabb.min.x, aabb.min.y, aabb.max.z},
            // Top
            {aabb.min.x, aabb.max.y, aabb.min.z},
            {aabb.max.x, aabb.max.y, aabb.min.z},
            aabb.max,
            {aabb.min.x, aabb.max.y, aabb.max.z}
        };

        size_t next_idx, back_idx;

        glm::vec3 p_lower_front, p_upper_front;
        glm::vec3 p_lower_back, p_upper_back;
        glm::vec3 p_front, p_back;
        glm::vec3 p;

        glm::vec3 q_lower_front, q_upper_front;
        glm::vec3 q_lower_back, q_upper_back;
        glm::vec3 q_front, q_back;
        glm::vec3 q;

        glm::vec3 h, v, face_normal;

        for (size_t j = 0; j < vcoefs.size(); ++j) {
            for (size_t i = 0; i < 4; ++i) {
                p_lower_front = corners[i];
                p_upper_front = corners[i + 4];

                if (i == 0) {
                    back_idx = 3;
                } else if (i == 1) {
                    back_idx = 2;
                } else if (i == 2) {
                    back_idx = 1;
                } else {
                    back_idx = 3;
                }

                p_lower_back = corners[back_idx];
                p_upper_back = corners[back_idx + 4];

                p_front = glm::mix(p_lower_front, p_upper_front, vcoefs[j]);
                p_back = glm::mix(p_lower_back, p_upper_back, vcoefs[j]);

                next_idx = (i + 1) % 4;
                back_idx = (next_idx + 3) % 4;

                q_lower_front = corners[next_idx];
                q_upper_front = corners[next_idx + 4];

                q_lower_back = corners[back_idx];
                q_upper_back = corners[back_idx + 4];

                p_front = glm::mix(q_lower_front, q_upper_front, vcoefs[j]);
                p_back = glm::mix(q_lower_back, q_upper_back, vcoefs[j]);

                h = q_lower_front - p_lower_front;
                v = p_upper_front - p_lower_front;
                face_normal = glm::normalize(glm::cross(v, h));

                for (size_t d = 0; d < dcoefs.size(); ++d) {
                    p = glm::mix(p_front, p_back, dcoefs[d]);
                    q = glm::mix(q_front, q_back, dcoefs[d]);

                    for (size_t k = 0; k < hcoefs.size(); ++k) {
                        ret.push_back({glm::mix(p, q, hcoefs[k]), face_normal});
                    }
                }
            }
        }

        return ret;
    }
};

// --------------------------------------------------------------------------------

#endif // GEOMETRY_HPP