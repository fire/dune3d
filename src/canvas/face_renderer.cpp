#include "face_renderer.hpp"
#include "gl_util.hpp"
#include "canvas.hpp"
#include <cmath>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>

namespace dune3d {
FaceRenderer::FaceRenderer(Canvas &c) : BaseRenderer(c, Canvas::VertexType::FACE_GROUP)
{
}

void FaceRenderer::create_vao()
{
    GLuint position_index = glGetAttribLocation(m_program, "position");
    GLuint normal_index = glGetAttribLocation(m_program, "normal");
    GLuint color_index = glGetAttribLocation(m_program, "color");


    /* we need to create a VAO to store the other buffers */
    glGenVertexArrays(1, &m_vao);
    glBindVertexArray(m_vao);

    /* this is the VBO that holds the vertex data */
    glGenBuffers(1, &m_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);

    glGenBuffers(1, &m_ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);

    std::vector<Canvas::FaceVertex> vertices = {
            {-1, -1, 0, 0, 0, 1, 255, 0, 0}, {1, .1, 0, 0, 0, 1, 255, 0, 0}, {0, -.1, 0, 0, 0, 1, 255, 0, 0}};
    glBufferData(GL_ARRAY_BUFFER, sizeof(Canvas::FaceVertex) * vertices.size(), vertices.data(), GL_STATIC_DRAW);

    uint32_t elements[] = {0, 1, 2};
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(elements), elements, GL_STATIC_DRAW);

    /* enable and set the position attribute */
    glEnableVertexAttribArray(position_index);
    glVertexAttribPointer(position_index, 3, GL_FLOAT, GL_FALSE, sizeof(Canvas::FaceVertex),
                          (void *)offsetof(Canvas::FaceVertex, x));
    glEnableVertexAttribArray(normal_index);
    glVertexAttribPointer(normal_index, 3, GL_FLOAT, GL_FALSE, sizeof(Canvas::FaceVertex),
                          (void *)offsetof(Canvas::FaceVertex, nx));
    glEnableVertexAttribArray(color_index);
    glVertexAttribPointer(color_index, 3, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(Canvas::FaceVertex),
                          (void *)offsetof(Canvas::FaceVertex, r));

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // glDeleteBuffers (1, &buffer);
}

void FaceRenderer::realize()
{
    m_program = gl_create_program_from_resource("/org/dune3d/dune3d/canvas/shaders/face-vertex.glsl",
                                                "/org/dune3d/dune3d/canvas/shaders/face-fragment.glsl", nullptr);
    create_vao();

    realize_base();

    GET_LOC(this, cam_normal);
    GET_LOC(this, flags);
    GET_LOC(this, origin);
    GET_LOC(this, normal_mat);
}

void FaceRenderer::push()
{

    auto n_vertices = m_ca.m_face_vertex_buffer.size();
    auto n_idx = m_ca.m_face_index_buffer.size();

    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(Canvas::FaceVertex) * n_vertices, m_ca.m_face_vertex_buffer.data(),
                 GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * n_idx, m_ca.m_face_index_buffer.data(),
                 GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void FaceRenderer::render()
{
    glUseProgram(m_program);
    glBindVertexArray(m_vao);
    // glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);

    load_uniforms();

    glUniform3fv(m_cam_normal_loc, 1, glm::value_ptr(m_ca.m_cam_normal));

    size_t group_idx = 0;
    for (const auto &group : m_ca.m_face_groups) {
        glUniform1ui(m_pick_base_loc, m_ca.m_pick_base + group_idx);
        glUniform1ui(m_flags_loc, static_cast<uint32_t>(group.flags));
        glUniform3fv(m_origin_loc, 1, glm::value_ptr(group.origin));
        glm::mat3 normal_mat = glm::transpose(glm::toMat3(group.normal));

        glUniformMatrix3fv(m_normal_mat_loc, 1, GL_FALSE, glm::value_ptr(normal_mat));
        glDrawElements(GL_TRIANGLES, group.length, GL_UNSIGNED_INT, (void *)(group.offset * sizeof(unsigned int)));
        group_idx++;
    }
    m_ca.m_vertex_type_picks[Canvas::VertexType::FACE_GROUP] = {.offset = m_ca.m_pick_base,
                                                                .count = m_ca.m_face_groups.size()};
    m_ca.m_pick_base += m_ca.m_face_groups.size();
}


size_t FaceRenderer::get_vertex_count() const
{
    return m_ca.m_n_glyphs;
}


} // namespace dune3d
