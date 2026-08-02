#!/usr/bin/env python3
"""Deterministically convert the publisher's original Cornell Box OBJ to glTF 2.0."""

from __future__ import annotations

import argparse
import hashlib
import json
import math
import os
import struct
from collections import OrderedDict
from dataclasses import dataclass, field
from pathlib import Path


FLOAT_COMPONENT = 5126
UNSIGNED_INT_COMPONENT = 5125
TRIANGLES_MODE = 4
GLB_MAGIC = 0x46546C67
GLB_VERSION = 2
GLB_JSON_CHUNK = 0x4E4F534A
GLB_BINARY_CHUNK = 0x004E4942
EXPECTED_MATERIAL_NAMES = (
    "leftWall",
    "rightWall",
    "floor",
    "ceiling",
    "backWall",
    "shortBox",
    "tallBox",
    "light",
)
EXPECTED_PRIMITIVE_NAMES = (
    "floor",
    "ceiling",
    "backWall",
    "rightWall",
    "leftWall",
    "shortBox",
    "tallBox",
    "light",
)
EXPECTED_SOURCE_POSITIONS = 72
EXPECTED_SOURCE_FACES = 18


@dataclass
class Material:
    name: str
    base_color: tuple[float, float, float] = (1.0, 1.0, 1.0)
    emissive_color: tuple[float, float, float] = (0.0, 0.0, 0.0)
    specular_exponent: float = 10.0


@dataclass
class Primitive:
    positions: list[tuple[float, float, float]] = field(default_factory=list)
    normals: list[tuple[float, float, float]] = field(default_factory=list)
    indices: list[int] = field(default_factory=list)


def parse_arguments() -> argparse.Namespace:
    script_directory = Path(__file__).resolve().parent
    asset_directory = script_directory.parent
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--source",
        type=Path,
        default=asset_directory / "Publisher" / "CornellBox-Original.obj",
        help="Path to the publisher's CornellBox-Original.obj file.",
    )
    parser.add_argument(
        "--output",
        type=Path,
        default=asset_directory / "CornellBox.glb",
        help="Path for the generated GLB artifact.",
    )
    return parser.parse_args()


def numeric_fields(line: str) -> list[float]:
    return [float(value) for value in line.split("#", 1)[0].split()[1:]]


def load_materials(material_path: Path) -> OrderedDict[str, Material]:
    materials: OrderedDict[str, Material] = OrderedDict()
    current: Material | None = None
    for line_number, raw_line in enumerate(material_path.read_text(encoding="utf-8").splitlines(), start=1):
        line = raw_line.strip()
        if not line or line.startswith("#"):
            continue
        keyword = line.split(maxsplit=1)[0]
        if keyword == "newmtl":
            fields = line.split()
            if len(fields) != 2 or fields[1] in materials:
                raise ValueError(f"Material declaration is malformed or duplicated at line {line_number}.")
            current = Material(fields[1])
            materials[current.name] = current
        elif current is not None and keyword == "Kd":
            values = numeric_fields(line)
            if len(values) != 3:
                raise ValueError(f"Diffuse color is malformed at line {line_number}.")
            current.base_color = tuple(values)
        elif current is not None and keyword == "Ke":
            values = numeric_fields(line)
            if len(values) != 3:
                raise ValueError(f"Emissive color is malformed at line {line_number}.")
            current.emissive_color = tuple(values)
        elif current is not None and keyword == "Ns":
            values = numeric_fields(line)
            if len(values) != 1:
                raise ValueError(f"Specular exponent is malformed at line {line_number}.")
            current.specular_exponent = values[0]
    if not materials:
        raise ValueError("MTL conversion found no materials.")
    if tuple(materials) != EXPECTED_MATERIAL_NAMES:
        raise ValueError(f"Publisher MTL inventory changed: found {tuple(materials)}.")
    return materials


def resolve_obj_index(value: str, count: int, line_number: int) -> int:
    index = int(value)
    resolved = index - 1 if index > 0 else count + index
    if resolved < 0 or resolved >= count:
        raise ValueError(f"OBJ index {index} is out of range at line {line_number}.")
    return resolved


def face_normal(vertices: list[tuple[float, float, float]], line_number: int) -> tuple[float, float, float]:
    normal = [0.0, 0.0, 0.0]
    for index, current in enumerate(vertices):
        following = vertices[(index + 1) % len(vertices)]
        normal[0] += (current[1] - following[1]) * (current[2] + following[2])
        normal[1] += (current[2] - following[2]) * (current[0] + following[0])
        normal[2] += (current[0] - following[0]) * (current[1] + following[1])
    length = math.sqrt(sum(component * component for component in normal))
    if not math.isfinite(length) or length <= 1.0e-8:
        raise ValueError(f"Face has no usable normal at line {line_number}.")
    return tuple(component / length for component in normal)


def load_obj(source_path: Path, materials: OrderedDict[str, Material]) -> OrderedDict[str, Primitive]:
    source_positions: list[tuple[float, float, float]] = []
    primitives: OrderedDict[str, Primitive] = OrderedDict()
    active_material = ""
    declared_material_library = ""
    source_face_count = 0

    for line_number, raw_line in enumerate(source_path.read_text(encoding="utf-8").splitlines(), start=1):
        line = raw_line.strip()
        if not line or line.startswith("#"):
            continue
        fields = line.split()
        keyword = fields[0]
        values = fields[1:]
        if keyword == "mtllib":
            if len(values) != 1:
                raise ValueError(f"Material library declaration is malformed at line {line_number}.")
            declared_material_library = values[0]
        elif keyword == "v":
            if len(values) != 3:
                raise ValueError(f"Position is incomplete at line {line_number}.")
            source_positions.append(tuple(float(value) for value in values))
        elif keyword == "usemtl":
            if len(values) != 1 or values[0] not in materials:
                raise ValueError(f"Material reference is malformed or unknown at line {line_number}.")
            active_material = values[0]
            primitives.setdefault(active_material, Primitive())
        elif keyword == "f":
            source_face_count += 1
            if active_material not in primitives or len(values) < 3:
                raise ValueError(f"Face has no supported material or too few vertices at line {line_number}.")
            source_indices = [resolve_obj_index(value, len(source_positions), line_number) for value in values]
            vertices = [source_positions[index] for index in source_indices]
            normal = face_normal(vertices, line_number)
            primitive = primitives[active_material]
            first_vertex = len(primitive.positions)
            primitive.positions.extend(vertices)
            primitive.normals.extend([normal] * len(vertices))
            for offset in range(1, len(vertices) - 1):
                primitive.indices.extend((first_vertex, first_vertex + offset, first_vertex + offset + 1))

    if declared_material_library != source_path.with_suffix(".mtl").name:
        raise ValueError("OBJ material-library identity does not match the selected source variant.")
    source_inventory = (len(source_positions), source_face_count)
    expected_inventory = (EXPECTED_SOURCE_POSITIONS, EXPECTED_SOURCE_FACES)
    if source_inventory != expected_inventory:
        raise ValueError(f"Publisher OBJ inventory changed: expected {expected_inventory}, found {source_inventory}.")
    if tuple(primitives) != EXPECTED_PRIMITIVE_NAMES:
        raise ValueError(f"Publisher OBJ material usage changed: found {tuple(primitives)}.")
    if not primitives or any(not primitive.indices for primitive in primitives.values()):
        raise ValueError("OBJ conversion produced incomplete material primitives.")
    return primitives


def pack_float_vectors(vectors: list[tuple[float, ...]]) -> bytes:
    flat_values = (component for vector in vectors for component in vector)
    return struct.pack(f"<{sum(len(vector) for vector in vectors)}f", *flat_values)


def align_buffer(buffer: bytearray) -> None:
    buffer.extend(b"\x00" * ((-len(buffer)) % 4))


def append_buffer_view(buffer: bytearray, payload: bytes, target: int) -> dict[str, int]:
    align_buffer(buffer)
    byte_offset = len(buffer)
    buffer.extend(payload)
    return {"buffer": 0, "byteLength": len(payload), "byteOffset": byte_offset, "target": target}


def vector_bounds(vectors: list[tuple[float, ...]]) -> tuple[list[float], list[float]]:
    component_count = len(vectors[0])
    minimum = [min(vector[component] for vector in vectors) for component in range(component_count)]
    maximum = [max(vector[component] for vector in vectors) for component in range(component_count)]
    return minimum, maximum


def build_material(material: Material) -> dict[str, object]:
    roughness = math.sqrt(2.0 / (max(material.specular_exponent, 0.0) + 2.0))
    document: dict[str, object] = {
        "name": material.name,
        "pbrMetallicRoughness": {
            "baseColorFactor": [*material.base_color, 1.0],
            "metallicFactor": 0.0,
            "roughnessFactor": roughness,
        },
    }
    emissive_strength = max(material.emissive_color)
    if emissive_strength > 0.0:
        document["emissiveFactor"] = [component / emissive_strength for component in material.emissive_color]
        if emissive_strength > 1.0:
            document["extensions"] = {"KHR_materials_emissive_strength": {"emissiveStrength": emissive_strength}}
    return document


def build_glb(
    materials: OrderedDict[str, Material],
    primitives: OrderedDict[str, Primitive],
) -> bytes:
    binary = bytearray()
    buffer_views: list[dict[str, int]] = []
    accessors: list[dict[str, object]] = []
    mesh_primitives: list[dict[str, object]] = []
    material_indices = {name: index for index, name in enumerate(materials)}

    for material_name, primitive in primitives.items():
        position_view = len(buffer_views)
        buffer_views.append(append_buffer_view(binary, pack_float_vectors(primitive.positions), 34962))
        normal_view = len(buffer_views)
        buffer_views.append(append_buffer_view(binary, pack_float_vectors(primitive.normals), 34962))
        index_view = len(buffer_views)
        buffer_views.append(append_buffer_view(binary, struct.pack(f"<{len(primitive.indices)}I", *primitive.indices), 34963))
        position_minimum, position_maximum = vector_bounds(primitive.positions)

        position_accessor = len(accessors)
        accessors.append(
            {
                "bufferView": position_view,
                "componentType": FLOAT_COMPONENT,
                "count": len(primitive.positions),
                "max": position_maximum,
                "min": position_minimum,
                "type": "VEC3",
            }
        )
        normal_accessor = len(accessors)
        accessors.append(
            {"bufferView": normal_view, "componentType": FLOAT_COMPONENT, "count": len(primitive.normals), "type": "VEC3"}
        )
        index_accessor = len(accessors)
        accessors.append(
            {"bufferView": index_view, "componentType": UNSIGNED_INT_COMPONENT, "count": len(primitive.indices), "type": "SCALAR"}
        )
        mesh_primitives.append(
            {
                "attributes": {"NORMAL": normal_accessor, "POSITION": position_accessor},
                "indices": index_accessor,
                "material": material_indices[material_name],
                "mode": TRIANGLES_MODE,
            }
        )

    document = {
        "accessors": accessors,
        "asset": {
            "copyright": "Guedis Cardenas and Morgan McGuire, CC BY 3.0 archive distribution",
            "generator": "SparkleEngine deterministic Cornell Box OBJ-to-glTF conversion",
            "version": "2.0",
        },
        "bufferViews": buffer_views,
        "buffers": [{"byteLength": len(binary)}],
        "extensionsUsed": ["KHR_materials_emissive_strength"],
        "materials": [build_material(material) for material in materials.values()],
        "meshes": [{"name": "Cornell Box Original", "primitives": mesh_primitives}],
        "nodes": [{"mesh": 0, "name": "Cornell Box Original"}],
        "scene": 0,
        "scenes": [{"name": "Cornell Box Original", "nodes": [0]}],
    }
    json_chunk = json.dumps(document, separators=(",", ":"), ensure_ascii=True).encode("utf-8")
    json_chunk += b" " * ((-len(json_chunk)) % 4)
    align_buffer(binary)
    total_length = 12 + 8 + len(json_chunk) + 8 + len(binary)
    return b"".join(
        (
            struct.pack("<III", GLB_MAGIC, GLB_VERSION, total_length),
            struct.pack("<II", len(json_chunk), GLB_JSON_CHUNK),
            json_chunk,
            struct.pack("<II", len(binary), GLB_BINARY_CHUNK),
            binary,
        )
    )


def sha256(payload: bytes) -> str:
    return hashlib.sha256(payload).hexdigest()


def publish_atomically(output_path: Path, payload: bytes) -> None:
    temporary_path = output_path.with_name(f".{output_path.name}.tmp")
    try:
        with temporary_path.open("wb") as output:
            output.write(payload)
            output.flush()
            os.fsync(output.fileno())
        temporary_path.replace(output_path)
    finally:
        temporary_path.unlink(missing_ok=True)


def main() -> None:
    arguments = parse_arguments()
    source_path = arguments.source.resolve()
    output_path = arguments.output.resolve()
    material_path = source_path.with_suffix(".mtl")
    if not source_path.is_file() or not material_path.is_file():
        raise FileNotFoundError(f"Publisher OBJ/MTL pair was not found: {source_path}")
    if output_path.suffix.lower() != ".glb":
        raise ValueError(f"Output must be one transactionally published .glb artifact: {output_path}")

    materials = load_materials(material_path)
    primitives = load_obj(source_path, materials)
    glb = build_glb(materials, primitives)
    output_path.parent.mkdir(parents=True, exist_ok=True)
    publish_atomically(output_path, glb)

    vertex_count = sum(len(primitive.positions) for primitive in primitives.values())
    triangle_count = sum(len(primitive.indices) // 3 for primitive in primitives.values())
    print(f"materials={len(materials)} primitives={len(primitives)} vertices={vertex_count} triangles={triangle_count}")
    print(f"{output_path.name} sha256={sha256(glb)}")


if __name__ == "__main__":
    main()
