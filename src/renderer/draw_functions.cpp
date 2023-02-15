#include "draw_functions.hpp"

#include <tracy/Tracy.hpp>

#include "extension/fmt.hpp"
#include "extension/fmt_geometry.hpp"
#include "general/hash.hpp"

namespace spellbook {

namespace icosphere {

const float IX = 0.525731112119133606f;
const float IZ = 0.850650808352039932f;
const float IN = 0.0f;

static const vector<Vertex> vertices = {
    {v3{-IX, IN, IZ}}, {v3{ IX, IN,  IZ}}, {v3{-IX,  IN, -IZ}}, {v3{ IX,  IN, -IZ}},
    {v3{ IN, IZ, IX}}, {v3{ IN, IZ, -IX}}, {v3{ IN, -IZ,  IX}}, {v3{ IN, -IZ, -IX}},
    {v3{ IZ, IX, IN}}, {v3{-IZ, IX,  IN}}, {v3{ IZ, -IX,  IN}}, {v3{-IZ, -IX,  IN}}
};

static const vector<u32> triangles = {
    0, 4, 1,   0, 9, 4,   9, 5, 4,   4, 5, 8,   4, 8, 1,   8, 10, 1,   8, 3,10, 
    5, 3, 8,   5, 2, 3,   2, 7, 3,   7,10, 3,   7, 6,10,   7, 11, 6,  11, 0, 6, 
    0, 1, 6,   6, 1,10,   9, 0,11,   9,11, 2,   9, 2, 5,   7, 2, 11
};

v2 position2uv(v3 v) {
	return v2(math::atan2(-v.x, v.y) * 0.1591 + 0.5, math::asin(v.z) * 0.3183 + 0.5);
}

} // namespace icosphere


MeshCPU generate_cube(v3 center, v3 extents, Color vertex_color) {
    ZoneScoped;
    string name = fmt_("cube_center:{:.2f}_extents:{:.2f}", center, extents);
    
	return MeshCPU{name, vector<Vertex>  {
        // back
        Vertex {center + extents * v3{-1, -1, -1}, {0, 0, -1}, {-1, 0, 0}, vertex_color.rgb, {1, 1}},
        Vertex {center + extents * v3{1, 1, -1}, {0, 0, -1}, {-1, 0, 0}, vertex_color.rgb, {0, 0}},
        Vertex {center + extents * v3{1, -1, -1}, {0, 0, -1}, {-1, 0, 0}, vertex_color.rgb, {0, 1}},
        Vertex {center + extents * v3{1, 1, -1}, {0, 0, -1}, {-1, 0, 0}, vertex_color.rgb, {0, 0}},
        Vertex {center + extents * v3{-1, -1, -1}, {0, 0, -1}, {-1, 0, 0}, vertex_color.rgb, {1, 1}},
        Vertex {center + extents * v3{-1, 1, -1}, {0, 0, -1}, {-1, 0, 0}, vertex_color.rgb, {1, 0}},
        // front
        Vertex {center + extents * v3{-1, -1, 1}, {0, 0, 1}, {1, 0.0, 0}, vertex_color.rgb, {0, 1}},
        Vertex {center + extents * v3{1, -1, 1}, {0, 0, 1}, {1, 0.0, 0}, vertex_color.rgb, {1, 1}},
        Vertex {center + extents * v3{1, 1, 1}, {0, 0, 1}, {1, 0.0, 0}, vertex_color.rgb, {1, 0}},
        Vertex {center + extents * v3{1, 1, 1}, {0, 0, 1}, {1, 0.0, 0}, vertex_color.rgb, {1, 0}},
        Vertex {center + extents * v3{-1, 1, 1}, {0, 0, 1}, {1, 0.0, 0}, vertex_color.rgb, {0, 0}},
        Vertex {center + extents * v3{-1, -1, 1}, {0, 0, 1}, {1, 0.0, 0}, vertex_color.rgb, {0, 1}},
        // left
        Vertex {center + extents * v3{-1, 1, -1}, {-1, 0, 0}, {0, 0, 1}, vertex_color.rgb, {0, 0}},
        Vertex {center + extents * v3{-1, -1, -1}, {-1, 0, 0}, {0, 0, 1}, vertex_color.rgb, {0, 1}},
        Vertex {center + extents * v3{-1, 1, 1}, {-1, 0, 0}, {0, 0, 1}, vertex_color.rgb, {1, 0}},
        Vertex {center + extents * v3{-1, -1, -1}, {-1, 0, 0}, {0, 0, 1}, vertex_color.rgb, {0, 1}},
        Vertex {center + extents * v3{-1, -1, 1}, {-1, 0, 0}, {0, 0, 1}, vertex_color.rgb, {1, 1}},
        Vertex {center + extents * v3{-1, 1, 1}, {-1, 0, 0}, {0, 0, 1}, vertex_color.rgb, {1, 0}},
        // right
        Vertex {center + extents * v3{1, 1, 1}, {1, 0, 0}, {0, 0, -1}, vertex_color.rgb, {0, 0}},
        Vertex {center + extents * v3{1, -1, -1}, {1, 0, 0}, {0, 0, -1}, vertex_color.rgb, {1, 1}},
        Vertex {center + extents * v3{1, 1, -1}, {1, 0, 0}, {0, 0, -1}, vertex_color.rgb, {1, 0}},
        Vertex {center + extents * v3{1, -1, -1}, {1, 0, 0}, {0, 0, -1}, vertex_color.rgb, {1, 1}},
        Vertex {center + extents * v3{1, 1, 1}, {1, 0, 0}, {0, 0, -1}, vertex_color.rgb, {0, 0}},
        Vertex {center + extents * v3{1, -1, 1}, {1, 0, 0}, {0, 0, -1}, vertex_color.rgb, {0, 1}},
        // bottom
        Vertex {center + extents * v3{-1, -1, -1}, {0, -1, 0}, {1, 0, 0}, vertex_color.rgb, {0, 1}},
        Vertex {center + extents * v3{1, -1, -1}, {0, -1, 0}, {1, 0, 0}, vertex_color.rgb, {1, 1}},
        Vertex {center + extents * v3{1, -1, 1}, {0, -1, 0}, {1, 0, 0}, vertex_color.rgb, {1, 0}},
        Vertex {center + extents * v3{1, -1, 1}, {0, -1, 0}, {1, 0, 0}, vertex_color.rgb, {1, 0}},
        Vertex {center + extents * v3{-1, -1, 1}, {0, -1, 0}, {1, 0, 0}, vertex_color.rgb, {0, 0}},
        Vertex {center + extents * v3{-1, -1, -1}, {0, -1, 0}, {1, 0, 0}, vertex_color.rgb, {0, 1}},
        // top
        Vertex {center + extents * v3{-1, 1, -1}, {0, 1, 0}, {1, 0, 0}, vertex_color.rgb, {0, 0}},
        Vertex {center + extents * v3{1, 1, 1}, {0, 1, 0}, {1, 0, 0}, vertex_color.rgb, {1, 1}},
        Vertex {center + extents * v3{1, 1, -1}, {0, 1, 0}, {1, 0, 0}, vertex_color.rgb, {1, 0}},
        Vertex {center + extents * v3{1, 1, 1}, {0, 1, 0}, {1, 0, 0}, vertex_color.rgb, {1, 1}},
        Vertex {center + extents * v3{-1, 1, -1}, {0, 1, 0}, {1, 0, 0}, vertex_color.rgb, {0, 0}},
        Vertex {center + extents * v3{-1, 1, 1}, {0, 1, 0}, {1, 0, 0}, vertex_color.rgb, {0, 1}}},
        {0,	1,	2,	3,	4,	5,	6,	7,	8,	9,	10, 11, 12, 13, 14, 15, 16, 17,
        18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35}
    };
}

MeshCPU generate_icosphere(int subdivisions) {
    string name = fmt_("icosphere_subdivisions:{}", subdivisions);
    
    vector<u32> index_list = icosphere::triangles;
    vector<Vertex> vertex_list = icosphere::vertices;
    for (int s = 0; s < subdivisions; s++) {
        vector<u32> new_index_list;
        for (int i = 0; i+2 < index_list.size(); i+=3) {
            v3 p1 = vertex_list[index_list[i+0]].position;
            v3 p2 = vertex_list[index_list[i+1]].position;
            v3 p3 = vertex_list[index_list[i+2]].position;
            u32 new_index1 = vertex_list.size();
            u32 new_index2 = new_index1 + 1;
            u32 new_index3 = new_index1 + 2;
            vertex_list.emplace_back(math::normalize(p1 + p2));
            vertex_list.emplace_back(math::normalize(p2 + p3));
            vertex_list.emplace_back(math::normalize(p3 + p1));

            new_index_list.push_back(index_list[i+0]);
            new_index_list.push_back(new_index1);
            new_index_list.push_back(new_index3);

            new_index_list.push_back(new_index1);
            new_index_list.push_back(index_list[i+1]);
            new_index_list.push_back(new_index2);

            new_index_list.push_back(new_index3);
            new_index_list.push_back(index_list[i+2]);
            new_index_list.push_back(new_index2);

            new_index_list.push_back(new_index1);
            new_index_list.push_back(new_index2);
            new_index_list.push_back(new_index3);
        }
        index_list = new_index_list;
    }

    for (Vertex& v : vertex_list) {
        v.normal = math::normalize(v.position);
        v.tangent = math::normalize(math::cross(v.position, v3::Z));
        if (math::length(v.tangent) != 1.0f)
            v.tangent = v3::X;
        v.uv = icosphere::position2uv(v.position);
        v.color = v3(0,0,0);
    }
    
    return MeshCPU(name, vertex_list, index_list);
}

MeshCPU generate_formatted_line(Camera* camera, vector<FormattedVertex> vertices) {
    string name = fmt_("line_hash:{:#x}", hash_data(vertices.data(), vertices.bsize()));
    
    if (!(vertices.size() >= 2))
        return {};
    struct Segment {
        v3 left;
        v3 right;
        Color color;
        bool separate = false;
    };

    vector<Segment> segments;
    segments.reserve(vertices.size() + (vertices.size() - 2) * 2);
    for (u32 i = 0; i < vertices.size(); i++) {
        FormattedVertex& vertex = vertices[i];
        v3 cam_vec = math::normalize(vertex.position - camera->position);

        if (vertex.color.a == 0.0f) {
            segments.push_back({.separate = true});
            continue;
        }

        // none_before/none_after is to skip the corner smoothing
        bool none_before = i == 0;
        bool none_after = (i + 1) == vertices.size();
        if (!none_before && !none_after) {
            FormattedVertex& vertex1 = vertices[i-1];
            FormattedVertex& vertex3 = vertices[i+1];
            none_before |= vertex1.color.a == 0.0f;
            none_after |= vertex3.color.a == 0.0f;
        }

        if (none_before && none_after)
            continue;
        
        if (none_before || none_after) {
            int index1 = none_after ? i-1 : i+0;
            int index2 = none_after ? i+0 : i+1;

            v3 line_vec = math::normalize(vertices[index2].position - vertices[index1].position);
            if (line_vec == v3(0))
                line_vec = v3(1,0,0);

            v3 right = math::normalize(math::cross(cam_vec, line_vec));

            segments.emplace_back(
                vertex.position + (right * vertex.width),
                vertex.position - (right * vertex.width),
                vertex.color
            );
        }
        // Take both rights of corner, use middle
        else {
            FormattedVertex& vertex1 = vertices[i-1];
            FormattedVertex& vertex3 = vertices[i+1];
            v3 vec1 = (vertex.position - vertex1.position);
            v3 vec2 = (vertex3.position - vertex.position);
            if (vec1 == v3(0)) vec1 = v3(1,0,0);
            if (vec2 == v3(0)) vec2 = v3(1,0,0);
            v3 right1 = math::normalize(math::cross(cam_vec, vec1));
            v3 right2 = math::normalize(math::cross(cam_vec, vec2));
            v3 right = math::normalize(right1 + right2);

            segments.emplace_back(
                vertex.position + (right1 * vertex.width),
                vertex.position - (right1 * vertex.width),

                vertex.color
            );   
            segments.emplace_back(
                vertex.position + (right * vertex.width),
                vertex.position - (right * vertex.width),

                vertex.color
            );    
            segments.emplace_back(
                vertex.position + (right2 * vertex.width),
                vertex.position - (right2 * vertex.width),

                vertex.color
            );  
        }
    }


    MeshCPU mesh_cpu;
    mesh_cpu.file_path = name;
    int quad_count = segments.size() - 1;
    mesh_cpu.vertices.reserve(quad_count * 6);
    mesh_cpu.indices.reserve(quad_count * 6);
    
    for (int i = 0, j = 0; i < segments.size() - 1; i++) {
        Segment& seg1 = segments[i+0];
        Segment& seg2 = segments[i+1];
        if (seg1.separate || seg2.separate)
            continue;

        mesh_cpu.vertices.emplace_back(seg1.left, v3(0,0,1), v3(1,0,0), seg1.color.rgb, v2(0));
        mesh_cpu.vertices.emplace_back(seg1.right, v3(0,0,1), v3(1,0,0), seg1.color.rgb, v2(0));
        mesh_cpu.vertices.emplace_back(seg2.right, v3(0,0,1), v3(1,0,0), seg2.color.rgb, v2(0));
        mesh_cpu.vertices.emplace_back(seg1.left, v3(0,0,1), v3(1,0,0), seg1.color.rgb, v2(0));
        mesh_cpu.vertices.emplace_back(seg2.right, v3(0,0,1), v3(1,0,0), seg2.color.rgb, v2(0));
        mesh_cpu.vertices.emplace_back(seg2.left, v3(0,0,1), v3(1,0,0), seg2.color.rgb, v2(0));
        mesh_cpu.indices.push_back(j*6+0);
        mesh_cpu.indices.push_back(j*6+1);
        mesh_cpu.indices.push_back(j*6+2);
        mesh_cpu.indices.push_back(j*6+3);
        mesh_cpu.indices.push_back(j*6+4);
        mesh_cpu.indices.push_back(j*6+5);
        j++;
    }
    return mesh_cpu;
}

}
