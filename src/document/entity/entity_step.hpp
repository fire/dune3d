#pragma once
#include "entity.hpp"
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include <filesystem>
#include "import_step/imported_step.hpp"

namespace dune3d {
class EntitySTEP : public Entity {
public:
    explicit EntitySTEP(const UUID &uu);
    explicit EntitySTEP(const UUID &uu, const json &j, const std::filesystem::path &containing_dir);
    static constexpr Type s_type = Type::STEP;
    Type get_type() const override
    {
        return s_type;
    }
    json serialize() const override;
    std::unique_ptr<Entity> clone() const override;

    double get_param(unsigned int point, unsigned int axis) const override;
    void set_param(unsigned int point, unsigned int axis, double value) override;

    glm::dvec3 get_point(unsigned int point, const Document &doc) const override;
    bool is_valid_point(unsigned int point) const override;

    void accept(EntityVisitor &visitor) const override;

    glm::dvec3 m_origin;
    glm::dquat m_normal;

    glm::dvec3 transform(glm::dvec3 p) const;
    glm::dvec3 untransform(glm::dvec3 p) const;

    std::filesystem::path m_path;
    std::filesystem::path get_path(const std::filesystem::path &containing_dir) const;

    std::map<unsigned int, glm::dvec3> m_anchors;
    std::map<unsigned int, glm::dvec3> m_anchors_transformed;

    void add_anchor(unsigned int i, const glm::dvec3 &pt);
    void remove_anchor(unsigned int i);

    bool m_show_points = false;

    std::shared_ptr<const ImportedSTEP> m_imported;
};

} // namespace dune3d
