#ifndef DIRECT_MULTIMESH_INSTANCE_H
#define DIRECT_MULTIMESH_INSTANCE_H

#include "../math/color8.h"
#include "../non_copyable.h"
#include "../span.h"

#include <core/templates/rid.h>
#include <scene/resources/multimesh.h>

class World3D;

// Thin wrapper around VisualServer multimesh instance API
class DirectMultiMeshInstance : public zylann::NonCopyable {
public:
	DirectMultiMeshInstance();
	~DirectMultiMeshInstance();

	void create();
	void destroy();
	bool is_valid() const;
	void set_world(World3D *world);
	void set_multimesh(Ref<MultiMesh> multimesh);
	Ref<MultiMesh> get_multimesh() const;
	void set_transform(Transform3D world_transform);
	void set_visible(bool visible);
	void set_material_override(Ref<Material> material);
	void set_cast_shadows_setting(RenderingServer::ShadowCastingSetting mode);

	static void make_transform_3d_bulk_array(Span<const Transform3D> transforms, PackedFloat32Array &bulk_array);

	struct TransformAndColor8 {
		Transform3D transform;
		Color8 color;
	};

	static void make_transform_and_color8_3d_bulk_array(
			Span<const TransformAndColor8> data, PackedFloat32Array &bulk_array);

	struct TransformAndColor32 {
		Transform3D transform;
		Color color;
	};

	static void make_transform_and_color32_3d_bulk_array(
			Span<const TransformAndColor32> data, PackedFloat32Array &bulk_array);

private:
	RID _multimesh_instance;
	Ref<MultiMesh> _multimesh;
};

#endif // DIRECT_MULTIMESH_INSTANCE_H
