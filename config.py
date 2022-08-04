
def can_build(env, platform):
    return True


def configure(env):
    from SCons.Script import BoolVariable, Variables, Help

    env_vars = Variables()

    env_vars.Add(BoolVariable("voxel_tests", 
        "Build with tests for the voxel module, which will run on startup of the engine", False))

    env_vars.Add(BoolVariable("voxel_fast_noise_2", "Build FastNoise2 support", True))

    env_vars.Update(env)
    Help(env_vars.GenerateHelpText(env))


def get_icons_path():
    return "editor/icons"


def get_doc_classes():
    return [
        "FastNoise2",
        "VoxelBlockSerializer",
        "VoxelBlockyLibrary",
        "VoxelBlockyModel",
        "VoxelBoxMover",
        "VoxelBuffer",
        "VoxelColorPalette",
        "VoxelDataBlockEnterInfo",
        "VoxelEngine",
        "VoxelGenerator",
        "VoxelGeneratorFlat",
        "VoxelGeneratorGraph",
        "VoxelGeneratorHeightmap",
        "VoxelGeneratorImage",
        "VoxelGeneratorNoise",
        "VoxelGeneratorNoise2D",
        "VoxelGeneratorScript",
        "VoxelGeneratorWaves",
        "VoxelInstanceComponent",
        "VoxelInstanceGenerator",
        "VoxelInstanceLibrary",
        "VoxelInstanceLibraryItem",
        "VoxelInstanceLibraryMultiMeshItem",
        "VoxelInstanceLibrarySceneItem",
        "VoxelInstancer",
        "VoxelLodTerrain",
        "VoxelMesher",
        "VoxelMesherBlocky",
        "VoxelMesherCubes",
        "VoxelMesherDMC",
        "VoxelMesherTransvoxel",
        "VoxelMeshSDF",
        "VoxelModifier",
        "VoxelModifierMesh",
        "VoxelModifierSphere",
        "VoxelNode",
        "VoxelRaycastResult",
        "VoxelStream",
        "VoxelStreamRegionFiles",
        "VoxelStreamScript",
        "VoxelStreamSQLite",
        "VoxelTerrain",
        "VoxelTool",
        "VoxelToolBuffer",
        "VoxelToolLodTerrain",
        "VoxelToolTerrain",
        "VoxelViewer",
        "VoxelVoxLoader",
        "ZN_FastNoiseLite",
        "ZN_FastNoiseLiteGradient",
        "ZN_ThreadedTask",
    ]


def get_doc_path():
    return "doc/classes"
