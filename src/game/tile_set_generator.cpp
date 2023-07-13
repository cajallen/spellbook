#include "tile_set_generator.hpp"

#include <tinygltf/tiny_gltf.h>

#include "editor/console.hpp"
#include "extension/fmt.hpp"
#include "general/logger.hpp"
#include "renderer/assets/mesh.hpp"

namespace spellbook {

static v3 flip_v3(v3 v, bool hori, bool vert) {
    return v3(v.x, hori ? -v.y : v.y, vert ? -v.z : v.z);
}

static void add_vert_type(vector<v3>& mesh, const vector<v3>& profile, uint32 rotation, v3 offset) {
    for (const v3& v : profile) {
        mesh.push_back(math::rotate(v, rotation) + offset);
    }
}

static void add_hori_type(vector<v3>& mesh, const vector<v3>& profile, uint32 rotation, bool flip, bool top) {
    for (const v3& v : profile) {
        mesh.push_back(math::rotate(flip_v3(v, flip, top), rotation));
    }
}

VisualTileMesh generate_visual_tile(const TileSetGeneratorSettings& settings, uint8 clear, uint8 type1, uint8 type2) {
    VisualTileMesh ret;

    for (int i = 0; i <= 0b111; i++) {
        if (type1 & (0b1 << DirectionBits(i))) {
            v3 v = to_vec(DirectionBits(i));
            ret.debug_mesh.push_back({v * 0.5f});
            ret.debug_mesh.push_back({v * 0.4f});
        }

        if (type2 & (0b1 << DirectionBits(i))) {
            v3 v = to_vec(DirectionBits(i));
            ret.debug_mesh.push_back({v.x * 0.5f, v.y * 0.5f, v.z * 0.5f});
            ret.debug_mesh.push_back({v.x * 0.4f, v.y * 0.5f, v.z * 0.5f});
            ret.debug_mesh.push_back({v.x * 0.5f, v.y * 0.5f, v.z * 0.5f});
            ret.debug_mesh.push_back({v.x * 0.5f, v.y * 0.4f, v.z * 0.5f});
            ret.debug_mesh.push_back({v.x * 0.5f, v.y * 0.5f, v.z * 0.5f});
            ret.debug_mesh.push_back({v.x * 0.5f, v.y * 0.5f, v.z * 0.4f});
        }
    }
    
    {
        // top
        for (int top = 0; top < 2; top++) {
            uint32 bot_empty_mask = 0b1 << NNN | 0b1 << NPN | 0b1 << PNN | 0b1 << PPN;
            uint32 top_empty_mask = 0b1 << NNP | 0b1 << NPP | 0b1 << PNP | 0b1 << PPP;
            int32 empty_spaces = math::csb(clear & (top ? top_empty_mask : bot_empty_mask));
            if (empty_spaces != 0) {
                if (empty_spaces == 1) {
                    for (int rotation = 0; rotation < 4; rotation++) {
                        if (clear & 0b1 << rotate_bits(top ? NNP : NNN, rotation)) {
                            // type 1
                            if ((type1 & 0b1 << rotate_bits(top ? NPP : NPN, rotation)) && (type1 & 0b1 << rotate_bits(top ? PNP : PNN, rotation))) {
                                add_vert_type(ret.mesh1, settings.type1_vertical_inside, rotation, v3(0.0f, 0.0f, top ? 0.5f : -0.5f));
                                break;
                            }
                            // type 2
                            if ((type2 & 0b1 << rotate_bits(top ? NPP : NPN, rotation)) && (type2 & 0b1 << rotate_bits(top ? PNP : PNN, rotation))) {
                                add_vert_type(ret.mesh2, settings.type2_vertical_inside, rotation, v3(0.0f, 0.0f, top ? 0.5f : -0.5f));
                                break;
                            }
                            // mixed
                            if ((type1 & 0b1 << rotate_bits(top ? NPP : NPN, rotation)) && (type2 & 0b1 << rotate_bits(top ? PNP : PNN, rotation))) {
                                // if we swap, rely on rotation to handle
                                add_vert_type(ret.mesh1, settings.mixed1_vertical_inside, rotation, v3(0.0f, 0.0f, top ? 0.5f : -0.5f));
                                add_vert_type(ret.mesh2, settings.mixed2_vertical_inside, rotation, v3(0.0f, 0.0f, top ? 0.5f : -0.5f));
                                break;
                            }
                        }
                    }
                }
                if (empty_spaces == 2) {
                    // corners
                    if ((clear & 0b1 << (top ? NNP : NNN) && clear & 0b1 << (top ? PPP : PPN)) || (clear & 0b1 << (top ? NPP : NPN) && clear & 0b1 << (top ? PNP : PNN))) {
                        for (int rotation = 0; rotation < 4; rotation++) {
                            if ((type1 & 0b1 << rotate_bits(top ? NNP : NNN, rotation)))
                                add_vert_type(ret.mesh1, settings.type1_vertical_outside, rotation, v3(0.0f, 0.0f, top ? 0.5f : -0.5f));
                            else if ((type2 & 0b1 << rotate_bits(top ? NNP : NNN, rotation)))
                                add_vert_type(ret.mesh2, settings.type2_vertical_outside, rotation, v3(0.0f, 0.0f, top ? 0.5f : -0.5f));
                        }
                    }
                    // line
                    else {
                        // along x
                        for (int rotation = 0; rotation < 4; rotation++) {
                            if ((clear & 0b1 << rotate_bits(top ? NNP : NNN, rotation)) && (clear & 0b1 << rotate_bits(top ? NPP : NPN, rotation))) {
                                bool neg_1 = (type1 & 0b1 << rotate_bits(top ? PNP : PNN, rotation));
                                (neg_1 ? ret.mesh1 : ret.mesh2).push_back({math::rotate(v3{0.0f, -0.5f, top ? 0.5f : -0.5f}, rotation)});
                                (neg_1 ? ret.mesh1 : ret.mesh2).push_back({math::rotate(v3{0.0f, 0.0f, top ? 0.5f : -0.5f}, rotation)});
                                bool pos_1 = (type1 & 0b1 << rotate_bits(top ? PPP : PPN, rotation));
                                (pos_1 ? ret.mesh1 : ret.mesh2).push_back({math::rotate(v3{0.0f, 0.0f, top ? 0.5f : -0.5f}, rotation)});
                                (pos_1 ? ret.mesh1 : ret.mesh2).push_back({math::rotate(v3{0.0f, 0.5f, top ? 0.5f : -0.5f}, rotation)});
                                break;
                            }
                        }
                        
                    }
                }
                if (empty_spaces == 3) {
                    for (int rotation = 0; rotation < 4; rotation++) {
                        // type 1
                        if ((type1 & 0b1 << rotate_bits(top ? NNP : NNN, rotation))) {
                            add_vert_type(ret.mesh1, settings.type1_vertical_outside, rotation, v3(0.0f, 0.0f, top ? 0.5f : -0.5f));
                            break;
                        }
                        // type 2
                        else if ((type2 & 0b1 << rotate_bits(top ? NNP : NNN, rotation))) {
                            add_vert_type(ret.mesh2, settings.type2_vertical_outside, rotation, v3(0.0f, 0.0f, top ? 0.5f : -0.5f));
                            break;
                        }
                    }
                }
            }
        }
    }

    for (int axis = 0; axis < 4; axis++) {
        int32 empty_spaces = math::csb(clear & (
            (0b1 << rotate_bits(NNP, axis)) | (0b1 << rotate_bits(NPP, axis)) |
            (0b1 << rotate_bits(NNN, axis)) | (0b1 << rotate_bits(NPN, axis)))
        );
        if (empty_spaces == 1) {
            for (int flip_v = 0; flip_v < 2; flip_v++) {
                for (int flip_h = 0; flip_h < 2; flip_h++) {
                    uint32 clear_space_bitidx = rotate_bits(NNN | (flip_v ? NNP : 0) | (flip_h ? NPN : 0), axis);
                    uint32 hori_neighbor_bitidx = rotate_bits(NNN | (flip_v ? NNP : 0) | (!flip_h ? NPN : 0), axis);
                    uint32 vert_neighbor_bitidx = rotate_bits(NNN | (!flip_v ? NNP : 0) | (flip_h ? NPN : 0), axis);
                    
                    if ((clear & 0b1 << clear_space_bitidx)) {
                        if ((type1 & 0b1 << hori_neighbor_bitidx) && (type1 & 0b1 << vert_neighbor_bitidx)) {
                            add_hori_type(ret.mesh1, settings.type1_horizontal_inside, axis, flip_h, flip_v);
                        }
                        else if ((type2 & 0b1 << hori_neighbor_bitidx) && (type2 & 0b1 << vert_neighbor_bitidx)) {
                            add_hori_type(ret.mesh2, settings.type2_horizontal_inside, axis, flip_h, flip_v);
                        }
                        else {
                            if ((type1 & 0b1 << hori_neighbor_bitidx) && (type2 & 0b1 << vert_neighbor_bitidx)) {
                                add_hori_type(ret.mesh1, settings.mixed_side_vertical_1_inside, axis, flip_h, flip_v);
                                add_hori_type(ret.mesh2, settings.mixed_side_horizontal_2_inside, axis, flip_h, flip_v);
                            } else if ((type2 & 0b1 << hori_neighbor_bitidx) && (type1 & 0b1 << vert_neighbor_bitidx)) {
                                add_hori_type(ret.mesh1, settings.mixed_side_horizontal_1_inside, axis, flip_h, flip_v);
                                add_hori_type(ret.mesh2, settings.mixed_side_vertical_2_inside, axis, flip_h, flip_v);
                            }
                        }
                    }
                }
            }
        }
        
        if (empty_spaces == 2) {
            for (int flip_v = 0; flip_v < 2; flip_v++) {
                for (int flip_h = 0; flip_h < 2; flip_h++) {
                    uint32 nn_bitidx = rotate_bits(DirectionBits(NNN | (flip_v ? NNP : 0) | (flip_h ? NPN : 0)), axis);
                    uint32 pn_bitidx = rotate_bits(DirectionBits(NNN | (flip_v ? NNP : 0) | (!flip_h ? NPN : 0)), axis);
                    uint32 pp_bitidx = rotate_bits(DirectionBits(NNN | (!flip_v ? NNP : 0) | (!flip_h ? NPN : 0)), axis);
                    uint32 np_bitidx = rotate_bits(DirectionBits(NNN | (!flip_v ? NNP : 0) | (flip_h ? NPN : 0)), axis);

                    // corners
                    if ((clear & 0b1 << np_bitidx) && (clear & 0b1 << pn_bitidx)) {
                        if ((type1 & 0b1 << nn_bitidx))
                            add_hori_type(ret.mesh1, settings.type1_horizontal_outside, axis, flip_h, flip_v);
                        if ((type1 & 0b1 << pp_bitidx))
                            add_hori_type(ret.mesh1, settings.type1_horizontal_outside, axis, !flip_h, !flip_v);
                        if ((type2 & 0b1 << nn_bitidx))
                            add_hori_type(ret.mesh2, settings.type2_horizontal_outside, axis, flip_h, flip_v);
                        if ((type2 & 0b1 << pp_bitidx))
                            add_hori_type(ret.mesh2, settings.type2_horizontal_outside, axis, !flip_h, !flip_v);
                    }
                    // line
                    else {
                        // hori
                        if ((clear & 0b1 << np_bitidx) && (clear & 0b1 << pp_bitidx)) {
                            bool neg_1 = type1 & 0b1 << nn_bitidx;
                            bool pos_1 = type1 & 0b1 << pn_bitidx;
                            (neg_1 ? ret.mesh1 : ret.mesh2).push_back({math::rotate(flip_v3(v3{-0.5f, -0.5f, 0.0f}, flip_h, flip_v), axis)});
                            (neg_1 ? ret.mesh1 : ret.mesh2).push_back({math::rotate(flip_v3(v3{-0.5f, 0.0f, 0.0f}, flip_h, flip_v), axis)});
                            (pos_1 ? ret.mesh1 : ret.mesh2).push_back({math::rotate(flip_v3(v3{-0.5f, 0.0f, 0.0f}, flip_h, flip_v), axis)});
                            (pos_1 ? ret.mesh1 : ret.mesh2).push_back({math::rotate(flip_v3(v3{-0.5f, 0.5f, 0.0f}, flip_h, flip_v), axis)});
                        }
                        // vert
                        if ((clear & 0b1 << pn_bitidx) && (clear & 0b1 << pp_bitidx)) {
                            bool neg_1 = type1 & 0b1 << nn_bitidx;
                            bool pos_1 = type1 & 0b1 << np_bitidx;
                            (neg_1 ? ret.mesh1 : ret.mesh2).push_back({math::rotate(flip_v3(v3{-0.5f, 0.0f, -0.5f}, flip_h, flip_v), axis)});
                            (neg_1 ? ret.mesh1 : ret.mesh2).push_back({math::rotate(flip_v3(v3{-0.5f, 0.0f, 0.0f}, flip_h, flip_v), axis)});
                            (pos_1 ? ret.mesh1 : ret.mesh2).push_back({math::rotate(flip_v3(v3{-0.5f, 0.0f, 0.0f}, flip_h, flip_v), axis)});
                            (pos_1 ? ret.mesh1 : ret.mesh2).push_back({math::rotate(flip_v3(v3{-0.5f, 0.0f, 0.5f}, flip_h, flip_v), axis)});
                        }
                    }
                }
            }
        }
        
        if (empty_spaces == 3) {
            for (int top = 0; top < 2; top++) {
                for (int flip = 0; flip < 2; flip++) {
                    uint32 solid_space_bitidx = rotate_bits(NNN | (top ? NNP : 0) | (flip ? NPN : 0), axis);

                    if ((type1 & 0b1 << solid_space_bitidx)) {
                        add_hori_type(ret.mesh1, settings.type1_horizontal_outside, axis, flip, top);
                        break;
                    }
                    else if ((type2 & 0b1 << solid_space_bitidx)) {
                        add_hori_type(ret.mesh2, settings.type2_horizontal_outside, axis, flip, top);
                        break;
                    }
                }
            }
        }
    }

    VisualTileMesh REAL_ret;
    for (int i = 0; (i + 1) < ret.mesh1.size(); i += 2) {
        REAL_ret.mesh1.push_back(ret.mesh1[i + 0]);
        REAL_ret.mesh1.push_back(ret.mesh1[i + 1]);
        REAL_ret.mesh1.push_back(v3(0.0f));
    }
    for (int i = 0; (i + 1) < ret.mesh2.size(); i += 2) {
        REAL_ret.mesh2.push_back(ret.mesh2[i + 0]);
        REAL_ret.mesh2.push_back(ret.mesh2[i + 1]);
        REAL_ret.mesh2.push_back(v3(0.0f));
    }
    REAL_ret.debug_mesh = ret.debug_mesh;
    
    return REAL_ret;
}

static uint32 quick_hash( uint8 clear, uint8 type1, uint8 type2) {
    uint32 clear_bits_set = math::csb(clear);
    uint32 type1_bits_set = math::csb(type1);
    uint32 type2_bits_set = math::csb(type2);
    return clear_bits_set << 16 | type1_bits_set << 8 | type2_bits_set;
}

#define CCTS(var) var == 1 ? "E" : var == 2 ? "1" : "2"

void generate_tile_set(const TileSetGeneratorSettings& settings) {
    umap<uint32, vector<VisualTileCorners>> processed_tiles;
    uint32 count = 0;
    vector<VisualTileMesh> meshes;
    for (uint32 clear = 0; clear <= 0b11111111; clear++) {
        for (uint32 type1 = 0; type1 <= 0b11111111; type1++) {
            if (type1 & clear)
                continue;
            for (uint32 type2 = 0; type2 <= 0b11111111; type2++) {
                if (type2 & clear)
                    continue;
                if (type1 & type2)
                    continue;
                uint8 unset = ~uint8(type1 | type2 | clear);
                if (unset)
                    continue;

                uint8 solid = type1 | type2;
                uint8 hidden = 0;

                // Check that if there's an axis shaped solid set, we make the inner corner ambiguous
                if (solid & 0b1 << NNN && solid & 0b1 << PNN && solid & 0b1 << NPN && solid & 0b1 << NNP)
                    hidden |= 0b1 << NNN;
                if (solid & 0b1 << PNN && solid & 0b1 << NNN && solid & 0b1 << PPN && solid & 0b1 << PNP)
                    hidden |= 0b1 << PNN;
                if (solid & 0b1 << NPN && solid & 0b1 << NNN && solid & 0b1 << PPN && solid & 0b1 << NPP)
                    hidden |= 0b1 << NPN;
                if (solid & 0b1 << PPN && solid & 0b1 << NPN && solid & 0b1 << PNN && solid & 0b1 << PPP)
                    hidden |= 0b1 << PPN;

                if (solid & 0b1 << NNP && solid & 0b1 << NNN && solid & 0b1 << NPP && solid & 0b1 << PPP)
                    hidden |= 0b1 << NNP;
                if (solid & 0b1 << PNP && solid & 0b1 << NNP && solid & 0b1 << PPP && solid & 0b1 << PNN)
                    hidden |= 0b1 << PNP;
                if (solid & 0b1 << NPP && solid & 0b1 << NNP && solid & 0b1 << PPP && solid & 0b1 << NPN)
                    hidden |= 0b1 << NPP;
                if (solid & 0b1 << PPP && solid & 0b1 << NPP && solid & 0b1 << PNP && solid & 0b1 << PPN)
                    hidden |= 0b1 << PPP;

                type1 |= hidden;
                type2 |= hidden;

                VisualTileCorners this_corners;
                for (uint8 i = 0; i < 8; i++) {
                    // (0b1 << i)                                    get the bit for our corner
                    // clear & (0b1 << i)                            check if it's set for our type
                    // clear & (0b1 << i) ? 1 : 0                    set it to 1 if so
                    // (clear & (0b1 << i) ? 1 : 0) << clear_index   set our output bit for this type
                    this_corners[i] = uint8((clear & (0b1 << i) ? 1 : 0) << 0 | (type1 & (0b1 << i) ? 1 : 0) << 1 | (type2 & (0b1 << i) ? 1 : 0) << 2);
                }
                
                bool this_duplicate = false;
                uint32 hashed = quick_hash(clear, type1, type2);

                if (!processed_tiles.contains(hashed))
                    processed_tiles[hashed] = {};
                
                for (VisualTileCorners& existing_corners : processed_tiles[hashed]) {
                    VisualTileRotation discard;
                    if (get_rotation(this_corners, existing_corners, discard, 0, true)) {
                        this_duplicate = true;
                        break;
                    }
                }
                if (this_duplicate)
                    continue;
                
                processed_tiles[hashed].push_back(this_corners);
                meshes.push_back(generate_visual_tile(settings, clear, type1, type2));
                meshes.back().name = fmt_("{}: NNN{} NNP{}  NPN{} NPP{}  |  PNN{} PNP{}  PPN{} PPP{}", count,
                    CCTS(this_corners[NNN]), CCTS(this_corners[NNP]), CCTS(this_corners[NPN]), CCTS(this_corners[NPP]),
                    CCTS(this_corners[PNN]), CCTS(this_corners[PNP]), CCTS(this_corners[PPN]), CCTS(this_corners[PPP]));
                count++;
            }
        }
    }

    if (meshes.empty())
        return;
    
    // Model
    tinygltf::Model model;
    tinygltf::Scene scene;
    
    vector<v3> raw_vertex_vector;
    vector<uint16> raw_index_vector;
    for (VisualTileMesh& mesh : meshes) {
        tinygltf::Node node;
        tinygltf::Mesh gltf_mesh;
        node.name = mesh.name;
        gltf_mesh.name = gltf_mesh.name;
        
        for (int i = 0; i < 3; i++) {
            vector<v3>& selected = i == 0 ? mesh.mesh1 : i == 1 ? mesh.mesh2 : mesh.debug_mesh;
            string selected_affix = i == 0 ? "_mesh1" : i == 1 ? "_mesh2" : "_debug_mesh";
            // Index buffer accessor
            tinygltf::Accessor indexAccessor;
            indexAccessor.bufferView = 0;
            indexAccessor.byteOffset = raw_index_vector.bsize();
            indexAccessor.componentType = TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT;
            indexAccessor.count = selected.size();
            indexAccessor.type = TINYGLTF_TYPE_SCALAR;
        
            // Vertex buffer accessor
            tinygltf::Accessor vertexAccessor;
            vertexAccessor.bufferView = 1;
            vertexAccessor.byteOffset = raw_vertex_vector.bsize();
            vertexAccessor.componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
            vertexAccessor.count = selected.size();
            vertexAccessor.type = TINYGLTF_TYPE_VEC3;

            for (v3& p : selected) {
                raw_index_vector.push_back(selected.index(p));
                raw_vertex_vector.push_back(p);
            }
            
            // Build the mesh primitive and add it to the mesh
            tinygltf::Primitive primitive;
            
            primitive.indices = model.accessors.size();
            model.accessors.push_back(indexAccessor);
            
            primitive.attributes["POSITION"] = model.accessors.size();
            model.accessors.push_back(vertexAccessor);
            
            primitive.material = i;
            primitive.mode = i < 2 ? TINYGLTF_MODE_TRIANGLES : TINYGLTF_MODE_LINE;
            gltf_mesh.primitives.push_back(primitive);
        }
        // Other tie ups
        node.translation = {2.0 * (model.meshes.size() % 24), 0.0, 2.0 * (model.meshes.size() / 24)};
        node.rotation = {0.0, 0.0, 0.0, 1.0};
        node.mesh = model.meshes.size();
        model.meshes.push_back(gltf_mesh);
        
        scene.nodes.push_back(model.nodes.size());
        model.nodes.push_back(node);
    }

    // Set raw buffer data
    tinygltf::Buffer buffer;
    buffer.data.resize(raw_vertex_vector.bsize() + raw_index_vector.bsize());
    
    memcpy(buffer.data.data(), raw_index_vector.data(), raw_index_vector.bsize());
    memcpy(buffer.data.data() + raw_index_vector.bsize(), raw_vertex_vector.data(), raw_vertex_vector.bsize());

    // Set buffer, byte offset, byte length, and target on index buffer view
    tinygltf::BufferView indexBufferView;
    indexBufferView.buffer = 0;
    indexBufferView.byteOffset = 0;
    indexBufferView.byteLength = raw_index_vector.bsize();
    indexBufferView.target = TINYGLTF_TARGET_ELEMENT_ARRAY_BUFFER;


    // Set buffer, byte offset, byte length, and target on vertex buffer view
    tinygltf::BufferView vertexBufferView;
    vertexBufferView.buffer = 0;
    vertexBufferView.byteOffset = raw_index_vector.bsize();
    vertexBufferView.byteLength = raw_vertex_vector.bsize();
    vertexBufferView.target = TINYGLTF_TARGET_ARRAY_BUFFER;
    

    // Create a simple material
    tinygltf::Material mat1;
    mat1.pbrMetallicRoughness.baseColorFactor = {1.0, 0.6, 0.6, 1.0};  
    mat1.doubleSided = true;
    model.materials.push_back(mat1);
    tinygltf::Material mat2;
    mat2.pbrMetallicRoughness.baseColorFactor = {0.6, 0.8, 1.0, 1.0};  
    mat2.doubleSided = true;
    model.materials.push_back(mat2);
    tinygltf::Material debug_mat;
    debug_mat.pbrMetallicRoughness.baseColorFactor = {0.6, 0.6, 0.6, 1.0};  
    debug_mat.doubleSided = true;
    model.materials.push_back(debug_mat);
    
    // Define the asset. The version is required
    tinygltf::Asset asset;
    asset.version = "2.0";
    asset.generator = "tinygltf";
    
    model.scenes.push_back(scene);
    model.buffers.push_back(buffer);
    model.bufferViews.push_back(indexBufferView);
    model.bufferViews.push_back(vertexBufferView);
    model.asset = asset;
    
    // Save it to a file
    tinygltf::TinyGLTF gltf;
    gltf.WriteGltfSceneToFile(&model, "tile_set.gltf",
                             true, // embedImages
                             true, // embedBuffers
                             true, // pretty print
                             false); // write binary
}

void generate_example_set() {
    TileSetGeneratorSettings settings;
    settings.type1_vertical_inside = { // 1 bevel
        {0.0f, -0.5f, 0.0f}, {0.0f, -0.1f, 0.0f}, {0.0f, -0.1f, 0.0f},
        {-0.1f, 0.0f, 0.0f}, {-0.1f, 0.0f, 0.0f}, {-0.5f, 0.0f, 0.0f}
    };
    settings.type1_vertical_outside = { // 2 bevel
        {0.0f, -0.5f, 0.0f}, {0.0f, -0.15f, 0.0f}, {0.0f, -0.15f, 0.0f},
        {-0.05f, -0.05f, 0.0f}, {-0.05f, -0.05f, 0.0f},
        {-0.15f, 0.0f, 0.0f}, {-0.15f, 0.0f, 0.0f}, {-0.5f, 0.0f, 0.0f}
    };
    settings.type2_vertical_inside = {
        {0.0f, -0.5f, 0.0f}, {0.0f, 0.0f, 0.0f},
        {0.0f, 0.0f, 0.0f}, {-0.5f, 0.0f, 0.0f}
    };
    settings.type2_vertical_outside = {
        {0.0f, -0.5f, 0.0f}, {0.0f, 0.0f, 0.0f},
        {0.0f, 0.0f, 0.0f}, {-0.5f, 0.0f, 0.0f}
    };
    settings.mixed1_vertical_inside = {
        {0.0f, -0.5f, 0.0f}, {0.0f, 0.0f, 0.0f}
    };
    settings.mixed2_vertical_inside = {
        {0.0f, 0.0f, 0.0f}, {-0.5f, 0.0f, 0.0f}
    };

    
    settings.type1_horizontal_inside = {
        {-0.5f, 0.0f, -0.5f}, {-0.5f, 0.0f, -0.1f},
        {-0.5f, 0.0f, -0.1f}, {-0.5f, -0.1f, 0.0f},
        {-0.5f, -0.1f, 0.0f}, {-0.5f, -0.5f, 0.0f}
    };
    settings.type1_horizontal_outside = {
        {-0.5f, 0.0f, -0.5f}, {-0.5f, 0.0f, -0.15f},
        {-0.5f, 0.0f, -0.15f}, {-0.5f, -0.05f, -0.05f},
        {-0.5f, -0.05f, -0.05f}, {-0.5f, -0.15f, 0.0f},
        {-0.5f, -0.15f, 0.0f}, {-0.5f, -0.5f, 0.0f}
    };
    settings.type2_horizontal_inside = {
        {-0.5f, 0.0f, -0.5f}, {-0.5f, 0.0f, 0.0f},
        {-0.5f, 0.0f, 0.0f}, {-0.5f, -0.5f, 0.0f}
    };
    settings.type2_horizontal_outside = {
        {-0.5f, 0.0f, -0.5f}, {-0.5f, 0.0f, 0.0f},
        {-0.5f, 0.0f, 0.0f}, {-0.5f, -0.5f, 0.0f}
    };
    settings.mixed_side_vertical_1_inside = {
        {-0.5f, 0.0f, -0.5f}, {-0.5f, 0.0f, 0.0f}
    };
    settings.mixed_side_horizontal_1_inside = {
        {-0.5f, 0.0f, 0.0f}, {-0.5f, -0.5f, 0.0f}
    };
    settings.mixed_side_horizontal_2_inside = {
        {-0.5f, 0.0f, 0.0f}, {-0.5f, -0.5f, 0.0f}
    };
    settings.mixed_side_vertical_2_inside = {
        {-0.5f, 0.0f, -0.5f}, {-0.5f, 0.0f, 0.0f}
    };
    generate_tile_set(settings);
}


}
