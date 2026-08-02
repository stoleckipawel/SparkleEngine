#!/usr/bin/env python3
"""Deterministically convert the publisher's LPS Head OBJ geometry to glTF 2.0."""

from __future__ import annotations

import argparse
import hashlib
import json
import math
import os
import struct
from dataclasses import dataclass
from pathlib import Path


FLOAT_COMPONENT = 5126
UNSIGNED_INT_COMPONENT = 5125
TRIANGLES_MODE = 4
GLB_MAGIC = 0x46546C67
GLB_VERSION = 2
GLB_JSON_CHUNK = 0x4E4F534A
GLB_BINARY_CHUNK = 0x004E4942
EXPECTED_SOURCE_POSITIONS = 8844
EXPECTED_SOURCE_TEXCOORDS = 35368
EXPECTED_SOURCE_FACES = 8842
MINIMUM_NORMAL_LENGTH_SQUARED = 1.0e-16


@dataclass
class ConvertedMesh:
    positions: list[tuple[float, float, float]]
    normals: list[tuple[float, float, float]]
    texcoords: list[tuple[float, float]]
    indices: list[int]
    skipped_degenerate_faces: int


def parse_arguments() -> argparse.Namespace:
    script_directory = Path(__file__).resolve().parent
    asset_directory = script_directory.parent
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--source",
        type=Path,
        default=asset_directory / "Publisher" / "head.OBJ",
        help="Path to the publisher's head.OBJ file.",
    )
    parser.add_argument(
        "--output",
        type=Path,
        default=asset_directory / "LPSHead.glb",
        help="Path for the generated GLB artifact.",
    )
    return parser.parse_args()


def resolve_obj_index(value: str, count: int, line_number: int) -> int:
    index = int(value)
    resolved = index - 1 if index > 0 else count + index
    if resolved < 0 or resolved >= count:
        raise ValueError(f"OBJ index {index} is out of range at line {line_number}.")
    return resolved


def parse_face_vertex(token: str, counts: tuple[int, int], line_number: int) -> tuple[int, int]:
    fields = token.split("/")
    if len(fields) != 2 or any(not field for field in fields):
        raise ValueError(f"Face vertex '{token}' is missing position or texture-coordinate data at line {line_number}.")
    return tuple(resolve_obj_index(field, count, line_number) for field, count in zip(fields, counts, strict=True))


def normalized(value: tuple[float, float, float], context: str) -> tuple[float, float, float]:
    length = math.sqrt(sum(component * component for component in value))
    if not math.isfinite(length) or length <= 1.0e-8:
        raise ValueError(f"Normal has no usable direction for {context}.")
    return tuple(component / length for component in value)


def load_obj(source_path: Path) -> ConvertedMesh:
    source_positions: list[tuple[float, float, float]] = []
    source_texcoords: list[tuple[float, float]] = []
    positions: list[tuple[float, float, float]] = []
    texcoords: list[tuple[float, float]] = []
    indices: list[int] = []
    expanded_source_positions: list[int] = []
    vertex_indices: dict[tuple[int, float, float], int] = {}
    accumulated_normals: list[list[float]] = []
    active_material = ""
    source_face_count = 0
    skipped_degenerate_faces = 0

    for line_number, raw_line in enumerate(source_path.read_text(encoding="utf-8").splitlines(), start=1):
        line = raw_line.strip()
        if not line or line.startswith("#"):
            continue

        fields = line.split()
        keyword = fields[0]
        values = fields[1:]
        if keyword == "v":
            if len(values) < 3:
                raise ValueError(f"Position is incomplete at line {line_number}.")
            source_positions.append(tuple(float(value) for value in values[:3]))
        elif keyword == "vn":
            raise ValueError(f"Unexpected authored normal at line {line_number}; this conversion generates smooth normals from geometry.")
        elif keyword == "vt":
            if len(values) < 2:
                raise ValueError(f"Texture coordinate is incomplete at line {line_number}.")
            source_texcoords.append((float(values[0]), 1.0 - float(values[1])))
        elif keyword == "usemtl":
            if len(values) != 1:
                raise ValueError(f"Material declaration is malformed at line {line_number}.")
            active_material = values[0]
            if active_material != "defaultMat":
                raise ValueError(f"Unexpected material '{active_material}' at line {line_number}.")
        elif keyword == "f":
            source_face_count += 1
            if active_material != "defaultMat":
                raise ValueError(f"Face has no supported material at line {line_number}.")
            if len(values) < 3:
                raise ValueError(f"Face has fewer than three vertices at line {line_number}.")

            counts = (len(source_positions), len(source_texcoords))
            face_source_indices = [parse_face_vertex(token, counts, line_number) for token in values]
            face_position_indices = [source_indices[0] for source_indices in face_source_indices]

            if not accumulated_normals:
                accumulated_normals = [[0.0, 0.0, 0.0] for _ in source_positions]
            if len(accumulated_normals) != len(source_positions):
                raise ValueError("OBJ positions must be declared before faces.")
            polygon_normal = [0.0, 0.0, 0.0]
            for index, position_index in enumerate(face_position_indices):
                current = source_positions[position_index]
                following = source_positions[face_position_indices[(index + 1) % len(face_position_indices)]]
                polygon_normal[0] += (current[1] - following[1]) * (current[2] + following[2])
                polygon_normal[1] += (current[2] - following[2]) * (current[0] + following[0])
                polygon_normal[2] += (current[0] - following[0]) * (current[1] + following[1])
            if sum(component * component for component in polygon_normal) <= MINIMUM_NORMAL_LENGTH_SQUARED:
                skipped_degenerate_faces += 1
                continue

            face: list[int] = []
            for position_index, texcoord_index in face_source_indices:
                texcoord = source_texcoords[texcoord_index]
                vertex_key = (position_index, texcoord[0], texcoord[1])
                vertex_index = vertex_indices.get(vertex_key)
                if vertex_index is None:
                    vertex_index = len(positions)
                    vertex_indices[vertex_key] = vertex_index
                    positions.append(source_positions[position_index])
                    texcoords.append(texcoord)
                    expanded_source_positions.append(position_index)
                face.append(vertex_index)

            for position_index in face_position_indices:
                accumulator = accumulated_normals[position_index]
                for component in range(3):
                    accumulator[component] += polygon_normal[component]

            for offset in range(1, len(face) - 1):
                indices.extend((face[0], face[offset], face[offset + 1]))

    source_inventory = (len(source_positions), len(source_texcoords), source_face_count)
    expected_inventory = (EXPECTED_SOURCE_POSITIONS, EXPECTED_SOURCE_TEXCOORDS, EXPECTED_SOURCE_FACES)
    if source_inventory != expected_inventory:
        raise ValueError(f"Publisher OBJ inventory changed: expected {expected_inventory}, found {source_inventory}.")

    if not positions or len(positions) != len(texcoords):
        raise ValueError("OBJ conversion produced incomplete vertex streams.")
    if not indices or len(indices) % 3:
        raise ValueError("OBJ conversion produced incomplete triangle indices.")

    normals = [
        normalized(tuple(accumulated_normals[position_index]), f"source position {position_index + 1}")
        for position_index in expanded_source_positions
    ]
    return ConvertedMesh(positions, normals, texcoords, indices, skipped_degenerate_faces)


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


def build_glb(
    positions: list[tuple[float, float, float]],
    normals: list[tuple[float, float, float]],
    texcoords: list[tuple[float, float]],
    indices: list[int],
) -> bytes:
    binary = bytearray()
    buffer_views = [
        append_buffer_view(binary, pack_float_vectors(positions), 34962),
        append_buffer_view(binary, pack_float_vectors(normals), 34962),
        append_buffer_view(binary, pack_float_vectors(texcoords), 34962),
        append_buffer_view(binary, struct.pack(f"<{len(indices)}I", *indices), 34963),
    ]
    position_minimum, position_maximum = vector_bounds(positions)
    texcoord_minimum, texcoord_maximum = vector_bounds(texcoords)

    document = {
        "accessors": [
            {
                "bufferView": 0,
                "componentType": FLOAT_COMPONENT,
                "count": len(positions),
                "max": position_maximum,
                "min": position_minimum,
                "type": "VEC3",
            },
            {"bufferView": 1, "componentType": FLOAT_COMPONENT, "count": len(normals), "type": "VEC3"},
            {
                "bufferView": 2,
                "componentType": FLOAT_COMPONENT,
                "count": len(texcoords),
                "max": texcoord_maximum,
                "min": texcoord_minimum,
                "type": "VEC2",
            },
            {"bufferView": 3, "componentType": UNSIGNED_INT_COMPONENT, "count": len(indices), "type": "SCALAR"},
        ],
        "asset": {
            "copyright": "Infinite Realities / Lee Perry-Smith, CC BY 3.0",
            "generator": "SparkleEngine deterministic LPS Head OBJ-to-glTF conversion",
            "version": "2.0",
        },
        "bufferViews": buffer_views,
        "buffers": [{"byteLength": len(binary)}],
        "images": [{"uri": "Publisher/lambertian.jpg"}],
        "materials": [
            {
                "name": "LPS Head Skin Baseline",
                "pbrMetallicRoughness": {
                    "baseColorTexture": {"index": 0},
                    "metallicFactor": 0.0,
                    "roughnessFactor": math.sqrt(2.0 / 7.0),
                },
            }
        ],
        "meshes": [
            {
                "name": "LPS Head",
                "primitives": [
                    {
                        "attributes": {"NORMAL": 1, "POSITION": 0, "TEXCOORD_0": 2},
                        "indices": 3,
                        "material": 0,
                        "mode": TRIANGLES_MODE,
                    }
                ],
            }
        ],
        "nodes": [{"mesh": 0, "name": "LPS Head"}],
        "samplers": [{"magFilter": 9729, "minFilter": 9987, "wrapS": 10497, "wrapT": 10497}],
        "scene": 0,
        "scenes": [{"name": "LPS Head", "nodes": [0]}],
        "textures": [{"sampler": 0, "source": 0}],
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
    if not source_path.is_file():
        raise FileNotFoundError(f"Publisher OBJ was not found: {source_path}")
    if output_path.suffix.lower() != ".glb":
        raise ValueError(f"Output must be one transactionally published .glb artifact: {output_path}")

    mesh = load_obj(source_path)
    glb = build_glb(mesh.positions, mesh.normals, mesh.texcoords, mesh.indices)
    output_path.parent.mkdir(parents=True, exist_ok=True)
    publish_atomically(output_path, glb)

    print(
        f"vertices={len(mesh.positions)} triangles={len(mesh.indices) // 3} "
        f"skipped_degenerate_faces={mesh.skipped_degenerate_faces}"
    )
    print(f"{output_path.name} sha256={sha256(glb)}")


if __name__ == "__main__":
    main()
