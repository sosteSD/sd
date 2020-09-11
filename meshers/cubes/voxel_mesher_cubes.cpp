#include "voxel_mesher_cubes.h"

namespace {
// 2-----3
// |     |
// |     |
// 0-----1
// [axis][front/back][i]
const uint8_t g_indices_lut[3][2][6] = {
	// X
	{
			// Front
			{ 0, 3, 2, 0, 1, 3 },
			// Back
			{ 0, 2, 3, 0, 3, 1 },
	},
	// Y
	{
			// Front
			{ 0, 2, 3, 0, 3, 1 },
			// Back
			{ 0, 3, 2, 0, 1, 3 },
	},
	// Z
	{
			// Front
			{ 0, 3, 2, 0, 1, 3 },
			// Back
			{ 0, 2, 3, 0, 3, 1 },
	}
};

const uint8_t g_face_axes_lut[Vector3i::AXIS_COUNT][2] = {
	// X
	{ Vector3i::AXIS_Y, Vector3i::AXIS_Z },
	// Y
	{ Vector3i::AXIS_X, Vector3i::AXIS_Z },
	// Z
	{ Vector3i::AXIS_X, Vector3i::AXIS_Y }
};

enum Side {
	SIDE_FRONT = 0,
	SIDE_BACK,
	SIDE_NONE // Either means there is no face, or it was consumed
};

} // namespace

// Returns:
// 0 if alpha is zero,
// 1 if alpha is neither zero neither max,
// 2 if alpha is max
inline uint8_t get_alpha_index(uint8_t v) {
	const uint8_t a = v & 3;
	return (a == 3) + (a > 0);
}

inline uint8_t get_alpha_index(uint16_t v) {
	const uint16_t a = v & 0xf;
	return (a == 0xf) + (a > 0);
}

inline Color to_color(uint8_t v) {
	// rrggbbaa
	return Color(
			(v >> 6) / 3.f,
			((v >> 4) & 3) / 3.f,
			((v >> 2) & 3) / 3.f,
			(v & 3) / 3.f);
}

inline Color to_color(uint16_t v) {
	// rrrrgggg bbbbaaaa
	return Color(
			(v >> 12) / 15.f,
			((v >> 8) & 0xf) / 15.f,
			((v >> 4) & 0xf) / 15.f,
			(v & 0xf) / 15.f);
}

template <typename Voxel_T>
void build_voxel_mesh_as_simple_cubes(
		FixedArray<VoxelMesherCubes::Arrays, VoxelMesherCubes::MATERIAL_COUNT> &out_arrays_per_material,
		const ArraySlice<Voxel_T> voxel_buffer,
		const Vector3i block_size) {

	ERR_FAIL_COND(block_size.x < 2 * VoxelMesherCubes::PADDING ||
				  block_size.y < 2 * VoxelMesherCubes::PADDING ||
				  block_size.z < 2 * VoxelMesherCubes::PADDING);

	const Vector3i min_pos = Vector3i(VoxelMesherCubes::PADDING);
	const Vector3i max_pos = block_size - Vector3i(VoxelMesherCubes::PADDING);
	const unsigned int row_size = block_size.y;
	const unsigned int deck_size = block_size.x * row_size;

	// Note: voxel buffers are indexed in ZXY order
	FixedArray<uint32_t, Vector3i::AXIS_COUNT> neighbor_offset_d_lut;
	neighbor_offset_d_lut[Vector3i::AXIS_X] = block_size.y;
	neighbor_offset_d_lut[Vector3i::AXIS_Y] = 1;
	neighbor_offset_d_lut[Vector3i::AXIS_Z] = block_size.x * block_size.y;

	FixedArray<uint32_t, VoxelMesherCubes::MATERIAL_COUNT> index_offsets(0);

	// For each axis
	for (unsigned int za = 0; za < Vector3i::AXIS_COUNT; ++za) {
		const unsigned int xa = g_face_axes_lut[za][0];
		const unsigned int ya = g_face_axes_lut[za][1];

		// For each deck
		for (unsigned int d = min_pos[za] - VoxelMesherCubes::PADDING; d < (unsigned int)max_pos[za]; ++d) {
			// For each cell of the deck, gather face info
			for (unsigned int fy = min_pos[ya]; fy < (unsigned int)max_pos[ya]; ++fy) {
				for (unsigned int fx = min_pos[xa]; fx < (unsigned int)max_pos[xa]; ++fx) {
					FixedArray<unsigned int, Vector3i::AXIS_COUNT> pos;
					pos[xa] = fx;
					pos[ya] = fy;
					pos[za] = d;

					const unsigned int voxel_index = pos[Vector3i::AXIS_Y] +
													 pos[Vector3i::AXIS_X] * row_size +
													 pos[Vector3i::AXIS_Z] * deck_size;

					const Voxel_T color0 = voxel_buffer[voxel_index];
					const Voxel_T color1 = voxel_buffer[voxel_index + neighbor_offset_d_lut[za]];

					const uint8_t ai0 = get_alpha_index(color0);
					const uint8_t ai1 = get_alpha_index(color1);

					Voxel_T color_raw;
					Side side;
					if (ai0 == ai1) {
						continue;
					} else if (ai0 > ai1) {
						color_raw = color0;
						side = SIDE_BACK;
					} else {
						color_raw = color1;
						side = SIDE_FRONT;
					}

					// Commit face to the mesh

					const Color color = to_color(color_raw);
					const uint8_t material_index = color.a < 0.999f;
					VoxelMesherCubes::Arrays &arrays = out_arrays_per_material[material_index];

					const int vx0 = fx - VoxelMesherCubes::PADDING;
					const int vy0 = fy - VoxelMesherCubes::PADDING;
					const int vx1 = vx0 + 1;
					const int vy1 = vy0 + 1;

					Vector3 v0;
					v0[xa] = vx0;
					v0[ya] = vy0;
					v0[za] = d;

					Vector3 v1;
					v1[xa] = vx1;
					v1[ya] = vy0;
					v1[za] = d;

					Vector3 v2;
					v2[xa] = vx0;
					v2[ya] = vy1;
					v2[za] = d;

					Vector3 v3;
					v3[xa] = vx1;
					v3[ya] = vy1;
					v3[za] = d;

					Vector3 n;
					n[za] = side == SIDE_FRONT ? -1 : 1;

					// 2-----3
					// |     |
					// |     |
					// 0-----1

					arrays.positions.push_back(v0);
					arrays.positions.push_back(v1);
					arrays.positions.push_back(v2);
					arrays.positions.push_back(v3);

					arrays.colors.push_back(color);
					arrays.colors.push_back(color);
					arrays.colors.push_back(color);
					arrays.colors.push_back(color);

					arrays.normals.push_back(n);
					arrays.normals.push_back(n);
					arrays.normals.push_back(n);
					arrays.normals.push_back(n);

					const unsigned int index_offset = index_offsets[material_index];
					CRASH_COND(za >= 3 || side >= 2);
					const uint8_t *lut = g_indices_lut[za][side];
					for (unsigned int i = 0; i < 6; ++i) {
						arrays.indices.push_back(index_offset + lut[i]);
					}
					index_offsets[material_index] += 4;
				}
			}
		}
	}
}

template <typename Voxel_T>
void build_voxel_mesh_as_greedy_cubes(
		FixedArray<VoxelMesherCubes::Arrays, VoxelMesherCubes::MATERIAL_COUNT> &out_arrays_per_material,
		const ArraySlice<Voxel_T> voxel_buffer,
		const Vector3i block_size,
		std::vector<uint8_t> mask_memory_pool) {

	ERR_FAIL_COND(block_size.x < static_cast<int>(2 * VoxelMesherCubes::PADDING) ||
				  block_size.y < static_cast<int>(2 * VoxelMesherCubes::PADDING) ||
				  block_size.z < static_cast<int>(2 * VoxelMesherCubes::PADDING));

	struct MaskValue {
		Voxel_T color;
		uint8_t side;

		inline bool operator==(const MaskValue &other) const {
			return color == other.color && side == other.side;
		}

		inline bool operator!=(const MaskValue &other) const {
			return color != other.color || side != other.side;
		}
	};

	const Vector3i min_pos = Vector3i(VoxelMesherCubes::PADDING);
	const Vector3i max_pos = block_size - Vector3i(VoxelMesherCubes::PADDING);
	const unsigned int row_size = block_size.y;
	const unsigned int deck_size = block_size.x * row_size;

	// Note: voxel buffers are indexed in ZXY order
	FixedArray<uint32_t, Vector3i::AXIS_COUNT> neighbor_offset_d_lut;
	neighbor_offset_d_lut[Vector3i::AXIS_X] = block_size.y;
	neighbor_offset_d_lut[Vector3i::AXIS_Y] = 1;
	neighbor_offset_d_lut[Vector3i::AXIS_Z] = block_size.x * block_size.y;

	FixedArray<uint32_t, VoxelMesherCubes::MATERIAL_COUNT> index_offsets(0);

	// For each axis
	for (unsigned int za = 0; za < Vector3i::AXIS_COUNT; ++za) {
		const unsigned int xa = g_face_axes_lut[za][0];
		const unsigned int ya = g_face_axes_lut[za][1];

		const unsigned int mask_size_x = (max_pos[xa] - min_pos[xa]);
		const unsigned int mask_size_y = (max_pos[ya] - min_pos[ya]);
		const unsigned int mask_area = mask_size_x * mask_size_y;
		// Using the vector as memory pool
		mask_memory_pool.resize(mask_area * sizeof(MaskValue));
		ArraySlice<MaskValue> mask(reinterpret_cast<MaskValue *>(mask_memory_pool.data()), 0, mask_area);

		// For each deck
		for (unsigned int d = min_pos[za] - VoxelMesherCubes::PADDING; d < (unsigned int)max_pos[za]; ++d) {
			// For each cell of the deck, gather face info
			for (unsigned int fy = min_pos[ya]; fy < (unsigned int)max_pos[ya]; ++fy) {
				for (unsigned int fx = min_pos[xa]; fx < (unsigned int)max_pos[xa]; ++fx) {
					FixedArray<unsigned int, Vector3i::AXIS_COUNT> pos;
					pos[xa] = fx;
					pos[ya] = fy;
					pos[za] = d;

					const unsigned int voxel_index = pos[Vector3i::AXIS_Y] +
													 pos[Vector3i::AXIS_X] * row_size +
													 pos[Vector3i::AXIS_Z] * deck_size;

					const Voxel_T color0 = voxel_buffer[voxel_index];
					const Voxel_T color1 = voxel_buffer[voxel_index + neighbor_offset_d_lut[za]];

					const uint8_t ai0 = get_alpha_index(color0);
					const uint8_t ai1 = get_alpha_index(color1);

					MaskValue mv;
					if (ai0 == ai1) {
						mv.side = SIDE_NONE;
					} else if (ai0 > ai1) {
						mv.color = color0;
						mv.side = SIDE_BACK;
					} else {
						mv.color = color1;
						mv.side = SIDE_FRONT;
					}

					mask[(fx - VoxelMesherCubes::PADDING) + (fy - VoxelMesherCubes::PADDING) * mask_size_x] = mv;
				}
			}

			struct L {
				static inline bool is_range_equal(
						const ArraySlice<MaskValue> &mask, unsigned int xmin, unsigned int xmax, MaskValue v) {
					for (unsigned int x = xmin; x < xmax; ++x) {
						if (mask[x] != v) {
							return false;
						}
					}
					return true;
				}
			};

			// Greedy quads
			for (unsigned int fy = 0; fy < mask_size_y; ++fy) {
				for (unsigned int fx = 0; fx < mask_size_x; ++fx) {
					const unsigned int mask_index = fx + fy * mask_size_x;
					const MaskValue m = mask[mask_index];

					if (m.side == SIDE_NONE) {
						continue;
					}

					// Check if the next faces are the same along X
					unsigned int rx = fx + 1;
					while (rx < mask_size_x && mask[rx + fy * mask_size_x] == m) {
						++rx;
					}

					// Check if the next rows of faces are the same along Y
					unsigned int ry = fy + 1;
					while (ry < mask_size_y &&
							L::is_range_equal(mask,
									fx + ry * mask_size_x,
									rx + ry * mask_size_x, m)) {
						++ry;
					}

					// Commit face to the mesh

					const Color color = to_color(m.color);
					const uint8_t material_index = color.a < 0.999f;
					VoxelMesherCubes::Arrays &arrays = out_arrays_per_material[material_index];

					Vector3 v0;
					v0[xa] = fx;
					v0[ya] = fy;
					v0[za] = d;

					Vector3 v1;
					v1[xa] = rx;
					v1[ya] = fy;
					v1[za] = d;

					Vector3 v2;
					v2[xa] = fx;
					v2[ya] = ry;
					v2[za] = d;

					Vector3 v3;
					v3[xa] = rx;
					v3[ya] = ry;
					v3[za] = d;

					Vector3 n;
					n[za] = m.side == SIDE_FRONT ? -1 : 1;

					// 2-----3
					// |     |
					// |     |
					// 0-----1

					arrays.positions.push_back(v0);
					arrays.positions.push_back(v1);
					arrays.positions.push_back(v2);
					arrays.positions.push_back(v3);

					arrays.colors.push_back(color);
					arrays.colors.push_back(color);
					arrays.colors.push_back(color);
					arrays.colors.push_back(color);

					arrays.normals.push_back(n);
					arrays.normals.push_back(n);
					arrays.normals.push_back(n);
					arrays.normals.push_back(n);

					const unsigned int index_offset = index_offsets[material_index];
					CRASH_COND(za >= 3 || m.side >= 2);
					const uint8_t *lut = g_indices_lut[za][m.side];
					for (unsigned int i = 0; i < 6; ++i) {
						arrays.indices.push_back(index_offset + lut[i]);
					}
					index_offsets[material_index] += 4;

					for (unsigned int j = fy; j < ry; ++j) {
						for (unsigned int i = fx; i < rx; ++i) {
							mask[i + j * mask_size_x].side = SIDE_NONE;
						}
					}
				}
			}
		}
	}
}

VoxelMesherCubes::VoxelMesherCubes() {
	set_padding(PADDING, PADDING);
}

void VoxelMesherCubes::build(VoxelMesher::Output &output, const VoxelMesher::Input &input) {
	const int channel = VoxelBuffer::CHANNEL_COLOR;

	for (unsigned int i = 0; i < _arrays_per_material.size(); ++i) {
		Arrays &a = _arrays_per_material[i];
		a.positions.clear();
		a.normals.clear();
		a.colors.clear();
		a.indices.clear();
	}

	const VoxelBuffer &voxels = input.voxels;
#ifdef TOOLS_ENABLED
	if (input.lod != 0) {
		WARN_PRINT("VoxelMesherBlocky received lod != 0, it is not supported");
	}
#endif

	// Iterate 3D padded data to extract voxel faces.
	// This is the most intensive job in this class, so all required data should be as fit as possible.

	// The buffer we receive MUST be dense (i.e not compressed, and channels allocated).
	// That means we can use raw pointers to voxel data inside instead of using the higher-level getters,
	// and then save a lot of time.

	if (voxels.get_channel_compression(channel) == VoxelBuffer::COMPRESSION_UNIFORM) {
		// All voxels have the same type.
		// If it's all air, nothing to do. If it's all cubes, nothing to do either.
		return;

	} else if (voxels.get_channel_compression(channel) != VoxelBuffer::COMPRESSION_NONE) {
		// No other form of compression is allowed
		ERR_PRINT("VoxelMesherBlocky received unsupported voxel compression");
		return;
	}

	ArraySlice<uint8_t> raw_channel;
	if (!voxels.get_channel_raw(channel, raw_channel)) {
		// Case supposedly handled before...
		ERR_PRINT("Something wrong happened");
		return;
	}

	const Vector3i block_size = voxels.get_size();
	const VoxelBuffer::Depth channel_depth = voxels.get_channel_depth(channel);

	switch (channel_depth) {
		case VoxelBuffer::DEPTH_8_BIT:
			if (_greedy_meshing) {
				build_voxel_mesh_as_greedy_cubes(_arrays_per_material, raw_channel, block_size, _mask_memory_pool);
			} else {
				build_voxel_mesh_as_simple_cubes(_arrays_per_material, raw_channel, block_size);
			}
			break;

		case VoxelBuffer::DEPTH_16_BIT:
			if (_greedy_meshing) {
				build_voxel_mesh_as_greedy_cubes(_arrays_per_material, raw_channel.reinterpret_cast_to<uint16_t>(),
						block_size, _mask_memory_pool);
			} else {
				build_voxel_mesh_as_simple_cubes(_arrays_per_material, raw_channel.reinterpret_cast_to<uint16_t>(),
						block_size);
			}
			break;

		default:
			ERR_PRINT("Unsupported voxel depth");
			return;
	}

	// TODO We could return a single byte array and use Mesh::add_surface down the line?

	for (unsigned int i = 0; i < MATERIAL_COUNT; ++i) {
		const Arrays &arrays = _arrays_per_material[i];

		if (arrays.positions.size() != 0) {
			Array mesh_arrays;
			mesh_arrays.resize(Mesh::ARRAY_MAX);

			{
				PoolVector<Vector3> positions;
				PoolVector<Vector3> normals;
				PoolVector<Color> colors;
				PoolVector<int> indices;

				raw_copy_to(positions, arrays.positions);
				raw_copy_to(normals, arrays.normals);
				raw_copy_to(colors, arrays.colors);
				raw_copy_to(indices, arrays.indices);

				mesh_arrays[Mesh::ARRAY_VERTEX] = positions;
				mesh_arrays[Mesh::ARRAY_NORMAL] = normals;
				mesh_arrays[Mesh::ARRAY_COLOR] = colors;
				mesh_arrays[Mesh::ARRAY_INDEX] = indices;
			}

			output.surfaces.push_back(mesh_arrays);

		} else {
			// Empty
			output.surfaces.push_back(Array());
		}
	}

	output.primitive_type = Mesh::PRIMITIVE_TRIANGLES;
	//output.compression_flags = Mesh::ARRAY_COMPRESS_COLOR;
}

void VoxelMesherCubes::set_greedy_meshing_enabled(bool enable) {
	_greedy_meshing = enable;
}

bool VoxelMesherCubes::is_greedy_meshing_enabled() const {
	return _greedy_meshing;
}

VoxelMesher *VoxelMesherCubes::clone() {
	VoxelMesherCubes *d = memnew(VoxelMesherCubes);
	d->_greedy_meshing = _greedy_meshing;
	return d;
}

void VoxelMesherCubes::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_greedy_meshing_enabled", "enable"),
			&VoxelMesherCubes::set_greedy_meshing_enabled);
	ClassDB::bind_method(D_METHOD("is_greedy_meshing_enabled"), &VoxelMesherCubes::is_greedy_meshing_enabled);

	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "greedy_meshing_enabled"),
			"set_greedy_meshing_enabled", "is_greedy_meshing_enabled");
}
