#ifndef MODEL_HPP
#define MODEL_HPP

#include "../global.hpp"

// --------------------------------------------------------------------------------

struct Model {
    glm::vec3         position = {};
    std::vector<Mesh> meshes;

    ~Model() {
        for (auto &mesh : meshes) {
            destroy(mesh.vertex_array);
            destroy(mesh.vertex_buffer);
            destroy(mesh.index_buffer);
        }
    }

// --------------------------------------------------------------------------------

    void init_geojson(const char *filepath) {
        FILE *file = fopen(filepath, "rb");
        const auto &data = json::parse(file);
        LOG_TRACE("Done parsing GEOJSON.");
        fclose(file);

        const auto &features = data["features"];

        Mesh_Type mesh_type;

        for (const auto &feature : features) {
        //for (size_ f = 0; f < features.size(); ++f) {
            //const auto &feature = features[f];
            //const auto &feature = features[4];
            //const auto &feature = features[48];
            //const auto &feature = features[85];
            //const auto &feature = features[71];
            //const auto &feature = features[29];
            //const auto &feature = features[56];
            //const auto &feature = features[27];
            //const auto &feature = features[82];
            //const auto &feature = features[58];
            //const auto &feature = features[23];

            const auto &properties = feature["properties"];

            float height = 0.0f;
            if (properties.find("height") != properties.end()) {
                const std::string &height_str = properties["height"];
                height = std::stof(height_str);

                mesh_type = MESH_TYPE_BUILDING;
            } else {
                mesh_type = MESH_TYPE_FLAT;
            }

            const auto &geometry = feature["geometry"];
            const auto &geometry_type = geometry["type"];
            const auto &geometry_coordinates = geometry["coordinates"];

            constexpr glm::vec3 max_vec3(FLT_MAX);

            Mesh mesh = {};
            mesh.vertices.reserve(geometry_coordinates.size());

            Vertex bottom_vert, top_vert;

            glm::vec3 prev_point(FLT_MAX), cur_point, next_point;

            if (geometry_type == "Polygon") {
                for (auto &coordinate : geometry_coordinates) {
                    for (size_t i = 0, point_count = coordinate.size() - 1; i < point_count; ++i) {
                        cur_point = to_vec3(coordinate[i]);

                        if (prev_point != max_vec3) {
                            next_point = to_vec3(coordinate[(i + 1) % point_count]);

                            while (i + 1 <= point_count && collinear(prev_point, cur_point, next_point)) {
                                ++i;
                                next_point = to_vec3(coordinate[(i + 1) % point_count]);
                            }
                        }

                        if (i == point_count) {
                            break;
                        }

                        bottom_vert = to_vertex(coordinate[i]);

                        mesh.vertices.push_back(bottom_vert);

                        prev_point = bottom_vert.position;
                    }
                }

                mesh.base_vert_count = mesh.vertices.size();

                if (mesh_type == MESH_TYPE_FLAT) {
                    // Base face triangulation
                    mesh.triangulate_face(0, mesh.base_vert_count);
                } else if (mesh_type == MESH_TYPE_BUILDING) {
                    
                    for (uint i = 0; i < mesh.base_vert_count; ++i) {
                        Vertex &cur = mesh.vertices[i];

                        top_vert.position.x = cur.position.x;
                        top_vert.position.y = height;
                        top_vert.position.z = cur.position.z;

                        top_vert.normal = {0.0f, 1.0f, 0.0f};

                        mesh.vertices.push_back(top_vert);
                    }

                    // Top face triangulation
                    mesh.triangulate_face(mesh.base_vert_count, mesh.vertices.size());

                    // Lateral faces
                    uint idx_lower, idx_upper, next_idx_lower, next_idx_upper;

                    for (uint i = 0; i < mesh.base_vert_count; ++i) {
                        idx_lower = i;
                        next_idx_lower = (i + 1) % mesh.base_vert_count;

                        idx_upper = idx_lower + mesh.base_vert_count;
                        next_idx_upper = next_idx_lower + mesh.base_vert_count;

                        mesh.indices.push_back(idx_upper);
                        mesh.indices.push_back(next_idx_upper);
                        mesh.indices.push_back(next_idx_lower);

                        mesh.indices.push_back(idx_upper);
                        mesh.indices.push_back(next_idx_lower);
                        mesh.indices.push_back(idx_lower);

                        const glm::vec3 &h = glm::normalize(mesh.vertices[next_idx_lower].position - mesh.vertices[idx_lower].position);
                        const glm::vec3 &v = glm::normalize(mesh.vertices[idx_upper].position - mesh.vertices[idx_lower].position);

                        const glm::vec3 &face_normal = glm::normalize(glm::cross(h, v));

                        mesh.vertices[idx_lower].normal += face_normal;
                        mesh.vertices[idx_upper].normal += face_normal;

                        mesh.vertices[next_idx_lower].normal += face_normal; 
                        mesh.vertices[next_idx_upper].normal += face_normal;
                    }

                    for (auto &vert : mesh.vertices) {
                        vert.normal = glm::normalize(vert.normal);
                    }
                }
            } else if (geometry_type == "MultiPolygon") {
                for (auto &polygon : geometry_coordinates) {
                    for (auto &coordinate : polygon) {
                        for (size_t i = 0, point_count = coordinate.size() - 1; i < point_count; ++i) {
                            cur_point = to_vec3(coordinate[i]);

                            if (prev_point != max_vec3) {
                                while (i + 1 <= point_count &&
                                       collinear(prev_point, cur_point, to_vec3(coordinate[(i + 1) % point_count]))) {
                                    ++i;
                                }
                            }

                            if (i == point_count) {
                                break;
                            }

                            bottom_vert = to_vertex(coordinate[i]);

                            mesh.vertices.push_back(bottom_vert);

                            prev_point = bottom_vert.position;
                        }
                    }
                }

                mesh.base_vert_count = mesh.vertices.size();

                mesh.triangulate_face(0, mesh.base_vert_count);
            } else {
                continue;
            }

            if (mesh_type == MESH_TYPE_BUILDING) {
                mesh.color[0] = randf();
                mesh.color[1] = randf();
                mesh.color[2] = randf();
                mesh.color[3] = 1.0f;
            } else if (mesh_type == MESH_TYPE_FLAT) {
                mesh.color = COLOR_GRAY;
            }

            if (!mesh.indices.empty()) {
                mesh.init(mesh_type);

                meshes.push_back(mesh);
            }
        }
    }

// --------------------------------------------------------------------------------

    void init_collada(const char *filepath) {
        tinyxml2::XMLDocument doc;
        doc.LoadFile(filepath);
        if (doc.LoadFile(filepath) != tinyxml2::XML_SUCCESS) {
            LOG_ERROR("Error loading COLLADA file: %s", filepath);
            return;
        }

        std::unordered_map<std::string, Geometry> geometries;
        std::unordered_map<std::string, std::vector<Geometry>> nodes;

        std::vector<glm::vec3> positions, normals;

        auto *geo_elem = doc.RootElement()->FirstChildElement("library_geometries")->FirstChildElement("geometry");
        Geometry geo = {};

        while (geo_elem != nullptr) {
            auto *mesh_elem = geo_elem->FirstChildElement("mesh");

            while (mesh_elem != nullptr) {
                auto *mesh_triangles_elem = mesh_elem->FirstChildElement("triangles");
                auto *mesh_input_elem = mesh_triangles_elem->FirstChildElement("input");

                uint num_attribs = 0;
                Collada_Source_Data source_data = {};

                while (mesh_input_elem != nullptr) {
                    ++num_attribs;

                    if (strncmp(mesh_input_elem->Attribute("semantic"), "VERTEX", 6) == 0) {
                        auto *mesh_verts_elem = mesh_elem->FirstChildElement("vertices");
                        auto *verts_input_elem = mesh_verts_elem->FirstChildElement("input");

                        while (verts_input_elem != nullptr) {
                            const char *source_name = verts_input_elem->Attribute("source");
                            if (source_name != nullptr) {
                                ++source_name;
                            }
                            uint source_name_len = strlen(source_name);

                            auto *mesh_source_elem = mesh_elem->FirstChildElement("source");

                            while (mesh_source_elem != nullptr) {
                                if (strncmp(mesh_source_elem->Attribute("id"), source_name, source_name_len) == 0) {
                                    read_collada_source(source_data, mesh_source_elem);

                                    float *float_data = reinterpret_cast<float *>(source_data.data);

                                    for (uint i = 0; i < source_data.size / sizeof(float); i += 3) {
                                        positions.emplace_back(float_data[i], float_data[i + 1], float_data[i + 2]);
                                    }

                                    break;
                                }

                                mesh_source_elem = mesh_source_elem->NextSiblingElement("source");
                            }

                            verts_input_elem = verts_input_elem->NextSiblingElement("input");
                        }
                    }

                    if (strncmp(mesh_input_elem->Attribute("semantic"), "NORMAL", 6) == 0) {
                        const char *source_name = mesh_input_elem->Attribute("source");
                        if (source_name != nullptr) {
                            ++source_name;
                        }
                        uint source_name_len = strlen(source_name);

                        auto *mesh_source = mesh_elem->FirstChildElement("source");

                        while (mesh_source != nullptr) {
                            auto tmp = mesh_source->Attribute("id");

                            if (strncmp(mesh_source->Attribute("id"), source_name, source_name_len) == 0) {
                                read_collada_source(source_data, mesh_source);

                                float *float_data = reinterpret_cast<float *>(source_data.data);

                                for (uint i = 0; i < source_data.size / sizeof(float); i += 3) {
                                    normals.emplace_back(float_data[i], float_data[i + 1], float_data[i + 2]);
                                }

                                break;
                            }

                            mesh_source = mesh_source->NextSiblingElement("source");
                        }
                    }

                    mesh_input_elem = mesh_input_elem->NextSiblingElement("input");
                }

                read_geometry(geo, mesh_triangles_elem, num_attribs, positions, normals);
                const char *geo_id = geo_elem->Attribute("id");
                geometries[geo_id] = geo;

                positions.clear();
                normals.clear();

                mesh_elem = mesh_elem->NextSiblingElement("mesh");
            }

            geo.vertices.clear();
            geo.indices.clear();

            geo_elem = geo_elem->NextSiblingElement("geometry");
        }

        auto *lib_node_elem = doc.RootElement()->FirstChildElement("library_nodes")->FirstChildElement("node");

        while (lib_node_elem != nullptr) {
            auto *lib_child_node_elem = lib_node_elem->FirstChildElement("node");

            while (lib_child_node_elem != nullptr) {
                
                // TRANSFORM 1 BEGIN
                auto *matrix_elem = lib_child_node_elem->FirstChildElement("matrix");
                glm::mat4 matrix = {};

                if (matrix_elem != nullptr) {
                    read_mat4(matrix, matrix_elem);

                    auto *instance_geo_elem = lib_child_node_elem->FirstChildElement("instance_geometry");

                    while (instance_geo_elem != nullptr) {
                        const char *url = instance_geo_elem->Attribute("url");
                        if (url != nullptr) {
                            ++url;
                        }

                        auto iter = geometries.find(url);
                        if (iter == geometries.end()) {
                            LOG_ERROR("Failed to get COLLADA geometry with id=%s", url);
                        } else {
                            Geometry geo = iter->second;

                            for (auto &vert : geo.vertices) {
                                glm::vec4 transformed_pos = glm::vec4(vert.position, 1.0f) * matrix;
                                vert.position = glm::vec3(transformed_pos);

                                //glm::vec4 transformed_normal = glm::vec4(vert.normal, 1.0f) * matrix;
                                //vert.normal = glm::normalize(glm::vec3(transformed_normal));
                            }

                            const char *lib_node_id = lib_node_elem->Attribute("id");
                            nodes[lib_node_id].push_back(geo);
                        }

                        instance_geo_elem = instance_geo_elem->NextSiblingElement("instance_geometry");
                    }
                }
                // TRANSFORM 1 END
                

                
                // TRANSFORM 2 BEGIN
                auto *lib_grandchild_node_elem = lib_child_node_elem->FirstChildElement("node");

                while (lib_grandchild_node_elem != nullptr) {
                    auto *matrix_elem = lib_grandchild_node_elem->FirstChildElement("matrix");
                    glm::mat4 matrix = {};

                    if (matrix_elem != nullptr) {
                        read_mat4(matrix, matrix_elem);

                        auto *instance_geo_elem = lib_grandchild_node_elem->FirstChildElement("instance_geometry");

                        while (instance_geo_elem != nullptr) {
                            const char *url = instance_geo_elem->Attribute("url");
                            if (url != nullptr) {
                                ++url;
                            }

                            auto iter = geometries.find(url);
                            if (iter == geometries.end()) {
                                LOG_ERROR("Failed to get COLLADA geometry with id=%s", url);
                            } else {
                                Geometry geo = iter->second;

                                for (auto &vert : geo.vertices) {
                                    glm::vec4 transformed_pos = glm::vec4(vert.position, 1.0f) * matrix;
                                    vert.position = glm::vec3(transformed_pos);

                                    //glm::vec4 transformed_normal = glm::vec4(vert.normal, 1.0f) * matrix;
                                    //vert.normal = glm::normalize(glm::vec3(transformed_normal));
                                }

                                const char *lib_node_id = lib_node_elem->Attribute("id");
                                nodes[lib_node_id].push_back(geo);
                            }

                            instance_geo_elem = instance_geo_elem->NextSiblingElement("instance_geometry");
                        }
                    }

                    lib_grandchild_node_elem = lib_grandchild_node_elem->NextSiblingElement("node");
                }
                // TRANSFORM 2 END
                

                lib_child_node_elem = lib_child_node_elem->NextSiblingElement("node");
            }

            lib_node_elem = lib_node_elem->NextSiblingElement("node");
        }

        auto *vis_node_elem = doc.RootElement()->FirstChildElement("library_visual_scenes")->FirstChildElement("visual_scene")->FirstChildElement("node");

        while (vis_node_elem != nullptr) {
            Mesh mesh = {};
            mesh.color = COLOR_WHITE;
            mesh.type = MESH_TYPE_MISC;

            auto *param_elem = vis_node_elem->FirstChildElement("extra")->FirstChildElement("technique")->FirstChildElement("param");

            while (param_elem != nullptr) {
                const char *name = param_elem->Attribute("name");
                const char *text = param_elem->GetText();

                if (strcmp(name, "entity:type") == 0) {
                    if (strcmp(text, "tree") == 0) {
                        mesh.color = COLOR_EMERALD;
                        mesh.type = MESH_TYPE_TREE;

                        break;
                    }
                }

                if (strcmp(name, "terrain") == 0) {
                    if (strcmp(text, "surface") == 0) {
                        mesh.color = COLOR_BRIGHT_GRAY;
                        mesh.type = MESH_TYPE_MISC;

                        break;
                    } else if (strcmp(text, "road") == 0) {
                        mesh.color = COLOR_DARK_GRAY;
                        mesh.type = MESH_TYPE_MISC;

                        break;
                    } else if (strcmp(text, "sidewalk") == 0) {
                        mesh.color = COLOR_GRAY;
                        mesh.type = MESH_TYPE_MISC;

                        break;
                    } else if (strcmp(text, "water") == 0) {
                        mesh.color = COLOR_BLUE;
                        mesh.type = MESH_TYPE_WATER;

                        break;
                    }
                }

                if (strcmp(name, "geopipe:identifier") == 0) {
                    mesh.color = COLOR_VISTA;
                    mesh.type = MESH_TYPE_BUILDING;
                }

                if (strcmp(name, "amenity") == 0) {
                    mesh.color = COLOR_TURQUOISE;
                    mesh.type = MESH_TYPE_AMENITY;

                    break;
                }

                if (strcmp(name, "name") == 0) {
                    mesh.color = COLOR_LAVENDER;
                    mesh.type = MESH_TYPE_LANDMARK;

                    break;
                }

                param_elem = param_elem->NextSiblingElement("param");
            }

            auto *translate_elem = vis_node_elem->FirstChildElement("translate");

            glm::vec3 translation = {};
            if (translate_elem != nullptr) {
                read_vec3(translation, translate_elem);
            }

            auto *instance_geo_elem = vis_node_elem->FirstChildElement("instance_geometry");

            while (instance_geo_elem != nullptr) {
                const char *url = instance_geo_elem->Attribute("url");
                if (url != nullptr) {
                    ++url;
                }

                auto iter = geometries.find(url);
                if (iter == geometries.end()) {
                    LOG_ERROR("Failed to get COLLADA geometry with id=%s", url);
                } else {
                    auto &verts = iter->second.vertices;
                    auto &idxs = iter->second.indices;

                    for (auto &vert : verts) {
                        vert.position += translation;
                    }

                    for (auto &idx : idxs) {
                        idx += mesh.vertices.size();
                    }

                    mesh.vertices.insert(mesh.vertices.end(), verts.begin(), verts.end());
                    mesh.indices.insert(mesh.indices.end(), idxs.begin(), idxs.end());
                }

                instance_geo_elem = instance_geo_elem->NextSiblingElement("instance_geometry");
            }

            meshes.push_back(mesh);

            mesh.vertices.clear();
            mesh.indices.clear();

            
            // TRANSFORM 3 BEGIN
            auto *matrix_elem = vis_node_elem->FirstChildElement("matrix");
            glm::mat4 matrix = {};

            if (matrix_elem != nullptr) {
                read_mat4(matrix, matrix_elem);

                auto *instance_node_elem = vis_node_elem->FirstChildElement("node")->FirstChildElement("instance_node");

                while (instance_node_elem != nullptr) {
                    const char *url = instance_node_elem->Attribute("url");
                    if (url != nullptr) {
                        ++url;
                    }

                    auto geos = nodes[url];

                    for (auto &geo : geos) {
                        auto &verts = geo.vertices;
                        auto &idxs = geo.indices;

                        for (auto &vert : verts) {
                            glm::vec4 transformed_pos = glm::vec4(vert.position, 1.0f) * matrix;
                            vert.position = glm::vec3(transformed_pos);

                            //glm::vec4 transformed_normal = glm::vec4(vert.normal, 1.0f) * matrix;
                            //vert.normal = glm::normalize(glm::vec3(transformed_normal));
                        }

                        mesh.vertices.insert(mesh.vertices.end(), verts.begin(), verts.end());
                        mesh.indices.insert(mesh.indices.end(), idxs.begin(), idxs.end());

                        meshes.push_back(mesh);

                        mesh.vertices.clear();
                        mesh.indices.clear();
                    }

                    instance_node_elem = instance_node_elem->NextSiblingElement("instance_node");
                }
            }
            // TRANSFORM 3 END
            

            vis_node_elem = vis_node_elem->NextSiblingElement("node");
        }

        std::vector<glm::vec3> vert_positions;
        std::vector<glm::vec3> trans_positions;

        for (auto &mesh : meshes) {
            vert_positions.reserve(mesh.vertices.size());
            for (auto &vert : mesh.vertices) {
                std::swap(vert.position.y, vert.position.z);
                std::swap(vert.normal.y, vert.normal.z);

                //mesh.aabb.extend(vert.position);
                vert_positions.push_back(vert.position);
            }

            if (MESH_TYPE_TREE < mesh.type && mesh.type < MESH_TYPE_MISC) {
                transform_points(trans_positions, vert_positions);

                for (auto &pos : trans_positions) {
                    mesh.aabb.extend(pos);
                }

                trans_positions.clear();
                vert_positions.clear();
            }

            mesh.init(MESH_TYPE_BUILDING);
        }
    }

// --------------------------------------------------------------------------------

    void init(const char *filepath, float x, float y, float z) {
        const char *last_dot = strrchr(filepath, '.');
        if (last_dot == nullptr) {
            LOG_ERROR("Failed to get last dot from path '%s'.", filepath);
            return;
        }

        if (strcmp(last_dot, ".geojson") == 0) {
            glm::vec2 tmp = to_meters(x, z);
            position.x = tmp.x;
            position.y = y;
            position.z = -tmp.y;

            init_geojson(filepath);
        } else if (strcmp(last_dot, ".dae") == 0) {
            position.x = x;
            position.y = y;
            position.z = z;

            init_collada(filepath);
        } else {
            LOG_ERROR("'%s' model format is not supported.", last_dot);
        }
    }
};

// --------------------------------------------------------------------------------

#endif // MODEL_HPP